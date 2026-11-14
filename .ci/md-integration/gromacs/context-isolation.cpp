// Exercise independent provider collections concurrently in a single process.
#include "gromacs/mdrun/plumedexchange.h"
#include "gromacs/mdtypes/iforceprovider.h"

#include <atomic>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <thread>

class Provider final : public gmx::IForceProvider
{
public:
    gmx::PlumedContext context;
    gmx::PlumedContext* plumedContext() override { return &context; }
    void calculateForces(const gmx::ForceProviderInput&, gmx::ForceProviderOutput*) override {}
};

class OtherProvider final : public gmx::IForceProvider
{
public:
    void calculateForces(const gmx::ForceProviderInput&, gmx::ForceProviderOutput*) override {}
};

static void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Provider context isolation failed");
    }
}

int main()
{
    require(gmx::findPlumedContext(nullptr) == nullptr);
    OtherProvider other;
    gmx::ForceProviders empty(nullptr);
    require(gmx::findPlumedContext(&empty) == nullptr);
    empty.addForceProvider(&other);
    require(gmx::findPlumedContext(&empty) == nullptr);

    std::atomic<int> ready{ 0 };
    std::exception_ptr errors[2];
    auto run = [&](int id)
    {
        try
        {
            Provider provider;
            OtherProvider additional;
            gmx::ForceProviders collection(nullptr);
            collection.addForceProvider(&additional);
            collection.addForceProvider(&provider);
            provider.context.ptr = &provider;
            provider.context.mainRank = (id == 0);
            ++ready;
            while (ready.load() < 2)
            {
                std::this_thread::yield();
            }
            for (int i = 0; i < 100000; ++i)
            {
                auto* context = gmx::findPlumedContext(&collection);
                require(context == &provider.context);
                require(context->ptr == &provider);
                require(context->mainRank == (id == 0));
                context->lambdaState = real(id * 100000 + i);
                context->trialEnergy = real(id * 100000 - i);
                context->hrexTrial = (id == 0);
                std::this_thread::yield();
                require(context->lambdaState == real(id * 100000 + i));
                require(context->trialEnergy == real(id * 100000 - i));
                require(context->hrexTrial == (id == 0));
            }
        }
        catch (...)
        {
            errors[id] = std::current_exception();
            ready = 2;
        }
    };
    std::thread first(run, 0), second(run, 1);
    first.join();
    second.join();
    for (const auto& error : errors)
    {
        if (error)
        {
            std::rethrow_exception(error);
        }
    }
    require(gmx::findPlumedContext(&empty) == nullptr);
    std::cout << "Null/non-PLUMED contexts: pass\n"
              << "Concurrent provider context isolation (200000 iterations): pass\n";
}
