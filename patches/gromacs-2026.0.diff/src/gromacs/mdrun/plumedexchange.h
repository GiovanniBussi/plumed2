#ifndef GMX_MDRUN_PLUMEDEXCHANGE_H
#define GMX_MDRUN_PLUMEDEXCHANGE_H

#include "config.h"

#include <cstdint>
#include <cstdio>

#include "gromacs/utility/basedefinitions.h"
#include "gromacs/utility/real.h"

struct t_commrec;
struct gmx_multisim_t;
struct gmx_repl_ex;
class t_state;

namespace gmx
{
// Provider-owned host state. Each thread-MPI rank has its own provider and buffers.
// Kernel calculations and exchange state remain instance-specific through cmd().
// Pointers into these buffers are registered with the kernel: the provider must
// stay at a stable address until its kernel and all simulation callbacks finish.
struct PlumedContext
{
    void*    ptr          = nullptr;
    bool     exchangeStep = false;
    gmx_bool hrex         = FALSE;
    bool     hrexTrial    = false;
    real     lambdaState  = 0;
    real     lambdaForce  = 0;
    real     trialEnergy  = 0;
    bool     mainRank     = true;
};
class ForceProviders;
// Stateless lookup in the simulation/rank-owned provider collection.
PlumedContext* findPlumedContext(ForceProviders* providers);
} // namespace gmx

#if GMX_LIB_MPI
int  plumed_hrex_partner(FILE*, const t_commrec*, const gmx_multisim_t*, gmx_repl_ex*, int64_t);
void plumed_hrex_cache_foreign(gmx::PlumedContext&, real*);
void exchange_state(const gmx_multisim_t*, int, t_state*);
void copy_state_serial(const t_state*, t_state*);
#endif // GMX_LIB_MPI
#endif
