#include "plumed/core/ActionSet.h"
#include "plumed/core/PlumedMain.h"
#include "plumed/ves/BasisFunctions.h"
#include "plumed/ves/TargetDistribution.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace PLMD;

void require(bool condition, const std::string& message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}

std::string number(double value) {
  std::ostringstream stream;
  stream << std::setprecision(17) << value;
  return stream.str();
}

int main() {
  PlumedMain plumed;
  int natoms=0;
  plumed.cmd("setNatoms",&natoms);
  plumed.cmd("setLogFile","test.log");
  plumed.cmd("init", {});
  std::ofstream output("output");
  unsigned index=0;
  // Exercise translated, untransformed and custom basis sets at several scales.
  for(const auto& interval : std::vector<std::vector<double>> {{-4,4},{-6.5,1.5},{0,1},{1e6,1e6+8},{-1e-6,1e-6}}) {
    const std::string limits=" MINIMUM="+number(interval[0])+" MAXIMUM="+number(interval[1]);
    for(const std::string definition : {
          "BF_LEGENDRE ORDER=2", "BF_POWERS ORDER=2", "BF_CUSTOM FUNC1=x FUNC2=x^2"
        }) {
      const std::string label="bf"+std::to_string(index++);
      plumed.readInputLine(label+": "+definition+limits);
      const auto* bf=plumed.getActionSet().selectWithLabel<ves::BasisFunctions*>(label);
      const auto size=bf->getNumberOfBasisFunctions();
      for(unsigned side=0; side<2; ++side) {
        const double endpoint=interval[side];
        const double direction=side==0 ? -1.0 : 1.0;
        const double near=std::nextafter(endpoint,direction*std::numeric_limits<double>::infinity());
        const double far=endpoint+direction*1e-7*(interval[1]-interval[0]);
        std::vector<double> values(size),derivs(size),expected_values(size),expected_derivs(size);
        double transformed;
        bool inside;
        bf->getAllValues(endpoint,transformed,inside,expected_values,expected_derivs);
        require(inside,label+" excludes its endpoint");
        require(expected_derivs[1]!=0.0,label+" has a zero endpoint derivative");
        // A rounding-sized displacement must retain the endpoint derivative.
        bf->getAllValues(near,transformed,inside,values,derivs);
        require(inside,label+" excludes an endpoint displaced by one ULP");
        require(values==expected_values && derivs==expected_derivs,label+" changes at a displaced endpoint");
        // A real excursion must still clamp the values and zero the derivatives.
        bf->getAllValues(far,transformed,inside,values,derivs);
        require(!inside,label+" includes a genuinely outside argument");
        require(values==expected_values,label+" does not clamp outside values");
        require(std::all_of(derivs.begin(),derivs.end(),[](double x) {
          return x==0.0;
        }),label+" has nonzero outside derivatives");
      }
    }
  }
  output << "Basis interval endpoints: OK\n";
  plumed.readInputLine("td: TD_UNIFORM MINIMA=-4 MAXIMA=4");
  const auto* td=plumed.getActionSet().selectWithLabel<ves::TargetDistribution*>("td");
  for(double endpoint : {
        -4.0,4.0
        }) {
    const double direction=endpoint<0 ? -1.0 : 1.0;
    const double near=std::nextafter(endpoint,direction*std::numeric_limits<double>::infinity());
    require(td->getValue({endpoint})==1.0,"TD_UNIFORM excludes its endpoint");
    require(td->getValue({near})==1.0,"TD_UNIFORM loses the endpoint integration weight");
    require(td->getValue({endpoint+direction*1e-7})==0.0,"TD_UNIFORM includes a genuinely outside argument");
  }
  output << "Uniform target interval endpoints: OK\n";
}
