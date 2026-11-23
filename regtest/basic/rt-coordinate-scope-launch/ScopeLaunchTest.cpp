#include "LaunchTest.cpp"
#include "core/ActionPilot.h"
#include "tools/AtomNumber.h"

namespace PLMD {
// Deliberately discover this mutation only in calculate(), to retain coverage
// of the scheduler's cancellation fallback for unknown/plugin modifiers.
class LateScopeMove : public ActionAtomistic, public ActionPilot {
public:
  static void registerKeywords(Keywords& keys) {
    Action::registerKeywords(keys);
    ActionAtomistic::registerKeywords(keys);
    keys.add("compulsory","STRIDE","1","execution stride");
  }
  explicit LateScopeMove(const ActionOptions& ao): Action(ao),ActionAtomistic(ao),ActionPilot(ao) {
    requestAtoms({AtomNumber::serial(1)});
  }
  void calculate() override {
    const auto atom=getValueIndices(AtomNumber::serial(1));
    setGlobalPosition(atom,getGlobalPosition(atom));
  }
  void apply() override {}
  bool actionHasForces() override {
    return false;
  }
};
PLUMED_REGISTER_ACTION(LateScopeMove,"LATE_SCOPE_MOVE")
}
