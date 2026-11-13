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
#include "ActionForExchangePattern.h"
#include "ActionSet.h"
#include "ExchangePatterns.h"
#include "PlumedMain.h"

namespace PLMD {

ActionForExchangePattern::ActionForExchangePattern(const ActionOptions& ao):
  Action(ao) {
  const auto patterns=plumed.getActionSet().select<ActionForExchangePattern*>();
  if(!patterns.empty()) {
    error("only one exchange-pattern action is allowed; already defined by "
          +patterns.front()->getLabel());
  }
}

void ActionForExchangePattern::registerKeywords(Keywords& keys) {
  Action::registerKeywords(keys);
}

void ActionForExchangePattern::setExchangeSeed(int) {}

bool ActionForExchangePattern::isExchangeAllowed(int, int) const {
  return true;
}

int ActionForExchangePattern::getExchangeSeed() const {
  return plumed.getExchangePatterns().getSeed();
}

}
