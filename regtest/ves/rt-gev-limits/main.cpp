#include "plumed/core/ActionSet.h"
#include "plumed/core/PlumedMain.h"
#include "plumed/ves/TargetDistribution.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

using namespace PLMD;

int main() {
  unsigned failures=0;
  auto require=[&failures](bool condition, const std::string& message) {
    if(!condition) {
      std::cerr << message << '\n';
      ++failures;
    }
  };
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
  const auto* translated_positive=distribution("translated_positive","LOCATION=1 SCALE=1 SHAPE=0.5");
  const auto* translated_exponential=distribution("translated_exponential","LOCATION=-1 SCALE=1 SHAPE=-1");
  const auto* translated_singular=distribution("translated_singular","LOCATION=-1 SCALE=2 SHAPE=-2");
  const auto* narrow_gumbel=distribution("narrow_gumbel","LOCATION=0 SCALE=0.5 SHAPE=0");
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
  // Coordinate translation can round an interior point onto the support
  // boundary. Preserve its density rather than feeding zero to log1p.
  require(translated_positive->getValue({std::nextafter(-1.0,0.0)})==0.0,"translated positive shape near endpoint is invalid");
  const double interior=std::nextafter(0.0,-1.0);
  require(translated_exponential->getValue({interior})==1.0,"translated shape -1 near endpoint has an incorrect limit");
  const double large=translated_singular->getValue({interior});
  const double expected_large=0.5/std::sqrt(-interior);
  require(std::isfinite(large) && std::abs(large/expected_large-1.0)<1e-12,"translated shape -2 near endpoint has an incorrect finite density");
  output << "Generalized extreme value translated endpoints: OK\n";
  require(gumbel->getValue({-1000.0})==0.0,"Gumbel lower tail is not zero");
  require(gumbel->getValue({1000.0})==0.0,"Gumbel upper tail is not zero");
  require(positive->getValue({std::nextafter(-2.0,0.0)})==0.0,"positive shape lower tail is not zero");
  require(narrow_gumbel->getValue({-std::numeric_limits<double>::max()})==0.0,"Gumbel overflowed lower-tail coordinate is not zero");
  output << "Generalized extreme value tails: OK\n";
  const double expected=std::exp(-0.3-std::exp(-0.3));
  require(std::abs(gumbel->getValue({0.3})-expected)<1e-14,"Gumbel density is incorrect");
  require(std::abs(small->getValue({0.3})-expected)<1e-12,"small shape does not approach the Gumbel density");
  output << "Generalized extreme value small shape: OK\n";
  return failures==0 ? 0 : 1;
}
