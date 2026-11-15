#ifndef GMX_MDRUN_PLUMEDEXCHANGE_H
#define GMX_MDRUN_PLUMEDEXCHANGE_H

#include "config.h"

#if GMX_LIB_MPI
#    include <cstdint>
#    include <cstdio>

#    include "gromacs/applied_forces/plumed/plumedforceprovider.h"
#    include "gromacs/utility/basedefinitions.h"
#    include "gromacs/utility/real.h"

struct t_commrec;
struct gmx_multisim_t;
struct gmx_repl_ex;
class t_state;

namespace gmx
{
// Temporary non-owning lookup for the one PLUMED provider on each real-MPI rank.
// Thread-MPI ranks share globals: neither this pointer nor accesses through it
// may be compiled there. Replace lookup with per-simulation access to enable tMPI MCMC.
extern PlumedForceProvider* plumedForceProvider;


//! Select this step's HREX partner and broadcast its replica index within each replica.
//! Called by every rank of every replica; returns the local replica index when unpaired.
int plumed_hrex_partner(FILE* fplog, const t_commrec* cr, const gmx_multisim_t* ms, gmx_repl_ex* re, int64_t step);

//! Select the exchange partner and native lambda state before any bias updates.
//! Called by all PP ranks; supplies provider-owned metadata for energy trials.
void plumed_prepare_exchange(FILE* fplog, const t_commrec* cr, const gmx_multisim_t* ms, gmx_repl_ex* re, int64_t step);

//! Exchange global coordinates, velocities, box and thermostat/barostat state with a partner.
//! Called on replica main ranks; domain redistribution is the caller's responsibility.
void exchange_state(const gmx_multisim_t* ms, int partner, t_state* state);

//! Native serial-state handoff: src and dest currently must alias; otherwise this asserts.
void copy_state_serial(const t_state* src, t_state* dest);
} // namespace gmx
#endif // GMX_LIB_MPI
#endif
