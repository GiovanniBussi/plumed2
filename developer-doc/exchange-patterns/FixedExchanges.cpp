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
#include "core/ActionForExchangePattern.h"
#include "core/ActionRegister.h"
#include <algorithm>

namespace PLMD {

// A deliberately small LOAD example: a fixed permutation and an optional pair
// to veto. Actual tree/grid actions can supply their own scheduling algorithms.
class FixedExchanges : public ActionForExchangePattern {
  std::vector<int> order;
  std::vector<int> rejected;
public:
  static void registerKeywords(Keywords& keys) {
    ActionForExchangePattern::registerKeywords(keys);
    keys.add("compulsory","ORDER","permutation of the zero-based replica indices");
    keys.add("optional","REJECT","two replica indices defining a dummy pair");
  }
  explicit FixedExchanges(const ActionOptions& ao):
    Action(ao), ActionForExchangePattern(ao) {
    parseVector("ORDER",order);
    parseVector("REJECT",rejected);
    checkRead();
    if(!rejected.empty() && (rejected.size()!=2 || rejected[0]==rejected[1])) {
      error("REJECT must contain two different replica indices");
    }
    for(int replica : rejected) {
      if(std::find(order.begin(),order.end(),replica)==order.end()) {
        error("REJECT indices must occur in ORDER");
      }
    }
  }
  void getExchangeList(std::vector<int>& list) override {
    // Core checks the size, bounds and uniqueness before exposing this to MD.
    list=order;
  }
  bool isExchangeAllowed(int replica, int partner) const override {
    return rejected.empty()
           || !((replica==rejected[0] && partner==rejected[1])
                || (replica==rejected[1] && partner==rejected[0]));
  }
};

PLUMED_REGISTER_ACTION(FixedExchanges,"FIXED_EXCHANGES")

}
