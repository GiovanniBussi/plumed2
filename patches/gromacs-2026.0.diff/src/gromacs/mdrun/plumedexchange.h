#ifndef GMX_MDRUN_PLUMEDEXCHANGE_H
#define GMX_MDRUN_PLUMEDEXCHANGE_H
#include <cstdint>
#include <cstdio>

#include "gromacs/utility/basedefinitions.h"
#include "gromacs/utility/real.h"
struct t_commrec;
struct gmx_multisim_t;
struct gmx_repl_ex;
struct t_state;
namespace gmx
{
extern void*    plumed_ptr;
extern bool     plumed_exchange_step;
extern gmx_bool plumed_hrex;
extern bool     plumed_hrex_trial;
extern real     plumed_lambda_state;
extern real     plumed_lambda_force;
extern real     plumed_trial_energy;
extern bool     plumed_main_rank;
} // namespace gmx
int plumed_hrex_partner(FILE*, const t_commrec*, const gmx_multisim_t*, gmx_repl_ex*, int64_t);
void plumed_hrex_cache_foreign(real*);
void exchange_state(const gmx_multisim_t*, int, t_state*);
void copy_state_serial(const t_state*, t_state*);
#endif
