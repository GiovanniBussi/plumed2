// PLUMED must retain provider-owned box storage across lambda trial calculations.
#include <cmath>

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gromacs/applied_forces/plumed/plumedOptions.h"
#include "gromacs/applied_forces/plumed/plumedforceprovider.h"
#include "gromacs/mdlib/gmx_omp_nthreads.h"
#include "gromacs/mdrunutility/handlerestart.h"
#include "gromacs/mdtypes/enerdata.h"
#include "gromacs/mdtypes/forceoutput.h"
#include "gromacs/utility/init.h"
#include "gromacs/utility/mpicomm.h"
#define __PLUMED_WRAPPER_CXX 1
#define __PLUMED_WRAPPER_LIBCXX11 1
#define __PLUMED_WRAPPER_EXTERN 0
#define __PLUMED_WRAPPER_FORTRAN 0
#define __PLUMED_WRAPPER_LINK_RUNTIME 1
#define __PLUMED_WRAPPER_IMPLEMENTATION 1
#define __PLUMED_HAS_DLOPEN
#include "Plumed.h"

int main(int argc, char** argv)
{
    gmx::init(&argc, &argv);
    gmx_omp_nthreads_set(ModuleMultiThread::Default, 1);
    try
    {
        std::ofstream("plumed.dat") << "d: DISTANCE ATOMS=1,2\n"
                                       "r: RESTRAINT ARG=d AT=0 KAPPA=2\n";
        gmx::MpiComm       comm(gmx::MpiComm::SingleRank{});
        gmx::PlumedOptions options;
        options.plumedFile_         = "plumed.dat";
        options.natoms_             = 2;
        options.mpiComm_            = &comm;
        options.simulationTimeStep_ = 0.001;
        options.startingBehavior_   = gmx::StartingBehavior::NewSimulation;
        gmx::PlumedForceProvider provider(options);
        std::vector<gmx::RVec>   x{ { 0, 0, 0 }, { real(0.45), 0, 0 } }, f(2);
        std::vector<real>        mass(2, 1), charge(2, 0);
        matrix                   box = { { 4, 0, 0 }, { 0, 4, 0 }, { 0, 0, 4 } };
        gmx::ForceProviderInput  input(x, 2, charge, mass, 0, 0, box, comm, nullptr);
        gmx::ForceWithVirial     force(f, true);
        gmx_enerdata_t           energy(1, nullptr);
        gmx::ForceProviderOutput output(&force, &energy);
        provider.calculateForces(input, &output);
        PLMD::Plumed plumed(provider.plumedContext()->ptr);
        real         before = 0, after = 0;
        plumed.cmd("getBias", &before);
        // Model reuse of the short-lived input storage without invoking undefined behavior.
        input.box_[0][0] = input.box_[1][1] = input.box_[2][2] = real(0.5);
        long int step                                          = 0;
        int      exchange                                      = 1;
        plumed.cmd("setExchangeStep", &exchange);
        plumed.cmd("setStepLong", &step);
        plumed.cmd("prepareCalc");
        plumed.cmd("performCalcNoForces");
        plumed.cmd("getBias", &after);
        std::cout << "before=" << before << " after=" << after << " expected=0.2025\n";
        if (std::abs(before - real(0.2025)) > 1e-6 || std::abs(after - before) > 1e-6)
            throw std::runtime_error("PLUMED retains the temporary force input box");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        gmx::finalize();
        return 1;
    }
    gmx::finalize();
}
