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
#include "ExchangePatterns.h"
#include "ActionForExchangePattern.h"
#include "ActionSet.h"
#include "PlumedMain.h"
#include <algorithm>
#include <numeric>
#include <vector>

namespace PLMD {

ExchangePatterns::ExchangePatterns(PlumedMain& p): plumed(p) {}

ActionForExchangePattern* ExchangePatterns::getAction() const {
  const auto patterns=plumed.getActionSet().select<ActionForExchangePattern*>();
  plumed_massert(patterns.size()<=1,"only one exchange-pattern action is allowed");
  return patterns.empty() ? nullptr : patterns.front();
}

void ExchangePatterns::setNofR(int nrepl) {
  plumed_massert(nrepl>0,"the number of replicas must be positive");
  numberOfReplicas=nrepl;
}

void ExchangePatterns::getFlag(int& flag) const {
  flag=getAction() ? 1 : 0;
}

void ExchangePatterns::setSeed(int newSeed) {
  seed=newSeed;
  if(auto* action=getAction()) {
    action->setExchangeSeed(seed);
  }
}

int ExchangePatterns::getSeed() const {
  return seed;
}

void ExchangePatterns::getList(const TypesafePtr& ind) {
  auto* output=ind.get<int*>({numberOfReplicas});
  if(auto* action=getAction()) {
    std::vector<int> list(numberOfReplicas);
    std::iota(list.begin(),list.end(),0);
    action->getExchangeList(list);
    if(list.size()!=static_cast<std::size_t>(numberOfReplicas)) {
      action->error("exchange list must contain one entry per replica");
    }
    std::vector<bool> seen(numberOfReplicas,false);
    for(int replica : list) {
      if(replica<0 || replica>=numberOfReplicas || seen[replica]) {
        action->error("exchange list must be a permutation of the replica indices");
      }
      seen[replica]=true;
    }
    std::copy(list.begin(),list.end(),output);
  }
  // With no provider, retain the legacy behavior: leave the buffer unchanged.
}

bool ExchangePatterns::isExchangeAllowed(int replica, int partner) const {
  if(auto* action=getAction()) {
    return action->isExchangeAllowed(replica,partner);
  }
  return true;
}

}
