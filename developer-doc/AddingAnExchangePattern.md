\page AddingAnExchangePattern How to add a replica-exchange pattern

PLUMED 2.11 provides PLMD::ActionForExchangePattern for exchange-pattern actions,
including actions compiled after installation with LOAD. The action owns the
algorithm and any random or schedule state. Core dispatches the existing MD-engine
commands to the action and validates its output. Only one exchange-pattern action
can belong to each PLUMED object; this includes RANDOM_EXCHANGES and loaded actions.

## Minimal action

Derive from ActionForExchangePattern and initialize both Action and
ActionForExchangePattern in the most-derived constructor. Call the base
registerKeywords(), parse your keywords and checkRead(). Override
getExchangeList(std::vector<int>& list). The vector initially contains the identity
permutation of the host-supplied replica count. Return a permutation of exactly
those zero-based indices. Core rejects wrong sizes, duplicate indices and indices
outside the valid range before the list reaches the MD engine.

An external example is distributed in
`developer-doc/exchange-patterns/FixedExchanges.cpp`. It registers FIXED_EXCHANGES,
returns the permutation supplied with ORDER, and optionally vetoes the pair given
with REJECT. Copy the file to the simulation directory, then use:

\verbatim
LOAD FILE=FixedExchanges.cpp
FIXED_EXCHANGES ORDER=0,1,2,3 REJECT=2,3
\endverbatim

Alternatively, compile the file with `plumed mklib FixedExchanges.cpp` and LOAD
the resulting shared library. Use the installed PLUMED executable and its headers,
and a compatible compiler/toolchain. No core edit, action-registration edit or
kernel rebuild is required. This is a C++ extension interface for the matching
PLUMED version, not a promise of binary compatibility across arbitrary versions
or compilers. ActionForExchangePattern.h is installed with the other core headers.

## Contract with the existing GROMACS patches

The returned list is a permutation, not a partner array. The GROMACS patch pairs
consecutive list positions according to its exchange parity. In the 2024.3 patch,
with `m=(step/replex)%2`, an index `i` from 1 to R-1 defines a proposed pair
`list[i-1],list[i]` when `i%2==m`.

For odd R there is one unpaired replica per round. For even R, the minimum number
of unpaired replicas alternates between zero and two. A permutation may choose
any pairing with this required number of pairs, but cannot increase host capacity.
The existing restrictions on custom patterns, including the GROMACS 2024.3
HREX and multiple-exchange paths, still apply.

An action can supply fewer real pairs by filling surplus slots with dummy pairs
of otherwise idle replicas. Override `isExchangeAllowed(int replica,int partner)`
with a symmetric predicate returning false for dummy pairs. GREX substitutes
positive infinity for the local exchange correction when a pair is vetoed. For
finite host energy contributions and finite positive inverse temperatures, this
forces Metropolis rejection, including GROMACS 2024.3's explicit cutoff. This
penalty never enters the physical bias or MD forces. Both endpoints should define
the same predicate. GROMACS will still report the dummy attempts as rejected
exchanges. Dummy pairs add two effectively idle replicas each.

The veto is evaluated on the GREX root of each replica, broadcast to its other
ranks, exchanged with the partner, and included in the shared corrections. A
self-partner is already idle and is never vetoed. This interface deliberately
supports a boolean veto rather than arbitrary changes to physical energies.

## Invocation, phase and restart

These callbacks are exchange callbacks, independent of normal calculate()/apply()
activation. They must not perform MPI collectives: existing hosts request the
list and evaluate the predicate on replica roots only. Use list.size() as the
host's replica count in getExchangeList(); it can differ from
Action::getNumberOfReplicas() in standalone command-interface tests. The veto may
also be queried without a preceding list request, so define its behavior for that
case. Every replica must use compatible inputs, schedule phase and random seed.

The current command interface does not pass the host's exchange interval or
pairing parity. A phase-dependent action should therefore require an explicit
exchange interval matching the host, and use getStep() to derive the phase. For
GROMACS 2024.3 this must reproduce `(getStep()/interval)%2`, including on restart.
Do not infer absolute phase simply by counting requests: the first request may
occur after a restart. Repeated requests are not automatically cached; the legacy
RANDOM_EXCHANGES generator advances on each request, as before this refactor.

For stateful actions, override hasCheckpointState(), writeCheckpoint() and
readCheckpoint() as for other actions. The action owns its RNG and counters;
GREX does not advance or checkpoint them. RANDOM_EXCHANGES retains the existing
`random` checkpoint field and its seeded permutation sequence.

The MD engine may set a seed through setExchangesSeed. Constructors of seeded
actions can read the last host seed with getExchangeSeed() (zero by default).
An explicit input seed may override it during construction. Override
setExchangeSeed(int) to handle a later host command, whose seed then takes
precedence. The default implementation ignores seeds for deterministic patterns.

## Tests

`regtest/basic/rt-make-exchange-pattern` compiles and LOADs the external example,
checks seed ordering, provider uniqueness/lifecycle, and invalid permutations.
`regtest/basic/rt-exchange-veto` checks real and vetoed pairs plus idle self-pairs
with four replicas and two MPI ranks per replica. The existing checkpoint-exchange
regression checks exact restart of RANDOM_EXCHANGES.
