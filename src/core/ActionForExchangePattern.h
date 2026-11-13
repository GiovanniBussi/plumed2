/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2023 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#ifndef __PLUMED_core_ActionForExchangePattern_h
#define __PLUMED_core_ActionForExchangePattern_h

#include "Action.h"
#include <vector>

namespace PLMD {

/**
\ingroup MULTIINHERIT
Base for actions providing replica-exchange patterns, including actions compiled
with LOAD. Only one such action may belong to a PlumedMain object.

The MD engine requests a permutation through getExchangesList and chooses pairs
from consecutive entries. This interface does not change the host's pairing
parity or capacity. See \ref AddingAnExchangePattern for the complete contract.
*/
class ActionForExchangePattern : public virtual Action {
public:
  explicit ActionForExchangePattern(const ActionOptions&);
  static void registerKeywords(Keywords&);
/// Fill a permutation of [0,list.size()). Called when the host requests a list,
/// normally only on rank zero of each replica. Do not perform collectives here.
/// The list is initialized to the identity. Its size is the host-supplied replica
/// count, which need not equal getNumberOfReplicas() in standalone tests.
  virtual void getExchangeList(std::vector<int>& list)=0;
/// Receive a seed from the MD engine, with the original sign. The default does
/// nothing. Constructors of seeded actions can obtain an earlier host seed with
/// getExchangeSeed(); a later host command takes precedence over input keywords.
  virtual void setExchangeSeed(int seed);
/// Veto a proposed pair, e.g. a padding attempt. Called on the GREX root rank,
/// including ranks that did not request a list. Must not perform collectives.
/// Define the same symmetric predicate in every replica. Self-pairs are idle
/// already and are not passed to this method. Default: allow every pair.
  virtual bool isExchangeAllowed(int replica, int partner) const;
/// Initial seed last supplied by the host, or zero if none was supplied.
  int getExchangeSeed() const;
  void calculate() override {}
  void apply() override {}
};

}

#endif
