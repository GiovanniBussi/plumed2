#include "plumed/core/ActionSet.h"
#include "plumed/core/PlumedMain.h"
#include "plumed/ves/TargetDistribution.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace PLMD;

void require(bool condition, const std::string& message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}

int main() {
  PlumedMain plumed;
  int natoms=0;
  plumed.cmd("setNatoms",&natoms);
  plumed.cmd("setLogFile","test.log");
  plumed.cmd("init", {});
  auto distribution=[&plumed](const std::string& label, const std::string& parameters) {
    plumed.readInputLine(label+": TD_GENERALIZED_EXTREME_VALUE "+parameters);
    return plumed.getActionSet().selectWithLabel<ves::TargetDistribution*>(label);
  };
  const auto* negative=distribution("negative","LOCATION=30 SCALE=2 SHAPE=-0.2");
  const auto* exponential=distribution("exponential","LOCATION=0 SCALE=2 SHAPE=-1");
  const auto* singular=distribution("singular","LOCATION=0 SCALE=2 SHAPE=-2");
  const auto* positive=distribution("positive","LOCATION=0 SCALE=1 SHAPE=0.5");
  const auto* gumbel=distribution("gumbel","LOCATION=0 SCALE=1 SHAPE=0");
  const auto* small=distribution("small","LOCATION=0 SCALE=1 SHAPE=1e-12");
  std::ofstream output("output");
  require(negative->getValue({40.0})==0.0,"negative shape upper endpoint is not zero");
  const double near=negative->getValue({std::nextafter(40.0,0.0)});
  require(std::isfinite(near) && near>=0.0 && near<1e-50,"negative shape near endpoint is invalid");
  require(negative->getValue({40.1})==0.0,"negative shape outside support is not zero");
  require(positive->getValue({-2.0})==0.0,"positive shape lower endpoint is not zero");
  require(positive->getValue({-2.1})==0.0,"positive shape outside support is not zero");
  // The upper-endpoint limit depends on the shape: it is finite at -1
  // and diverges below -1. It must not be replaced by zero for all shapes.
  require(exponential->getValue({2.0})==0.5,"shape -1 upper endpoint has an incorrect limit");
  require(singular->getValue({1.0})==std::numeric_limits<double>::infinity(),"shape -2 upper endpoint has an incorrect limit");
  require(singular->getValue({1.1})==0.0,"singular distribution outside support is not zero");
  output << "Generalized extreme value support limits: OK\n";
  require(gumbel->getValue({-1000.0})==0.0,"Gumbel lower tail is not zero");
  require(gumbel->getValue({1000.0})==0.0,"Gumbel upper tail is not zero");
  require(positive->getValue({std::nextafter(-2.0,0.0)})==0.0,"positive shape lower tail is not zero");
  output << "Generalized extreme value tails: OK\n";
  const double expected=std::exp(-0.3-std::exp(-0.3));
  require(std::abs(gumbel->getValue({0.3})-expected)<1e-14,"Gumbel density is incorrect");
  require(std::abs(small->getValue({0.3})-expected)<1e-12,"small shape does not approach the Gumbel density");
  output << "Generalized extreme value small shape: OK\n";
}
