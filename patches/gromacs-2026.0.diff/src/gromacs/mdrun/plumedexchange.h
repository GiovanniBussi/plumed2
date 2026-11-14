#ifndef GMX_MDRUN_PLUMEDEXCHANGE_H
#define GMX_MDRUN_PLUMEDEXCHANGE_H

#include "config.h"

#if GMX_LIB_MPI
#    include <cstdint>
#    include <cstdio>

#    include "gromacs/utility/basedefinitions.h"
#    include "gromacs/utility/real.h"

struct t_commrec;
struct gmx_multisim_t;
struct gmx_repl_ex;
class t_state;

namespace gmx
{
// One non-owning context per MPI process. This bridge also serves single-replica
// alchemical MCMC. It must never be used with thread-MPI: its ranks share process
// globals, whereas each rank needs its own PLUMED instance and mutable buffers.
// Keep the definition and every access under #if GMX_LIB_MPI, even if a future
// GROMACS version permits replica exchange with thread-MPI.
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
extern PlumedContext plumedContext;
} // namespace gmx

int plumed_hrex_partner(FILE*, const t_commrec*, const gmx_multisim_t*, gmx_repl_ex*, int64_t);
void plumed_hrex_cache_foreign(real*);
void exchange_state(const gmx_multisim_t*, int, t_state*);
void copy_state_serial(const t_state*, t_state*);
#endif // GMX_LIB_MPI
#endif
