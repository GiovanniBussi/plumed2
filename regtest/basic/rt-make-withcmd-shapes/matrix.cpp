#include "plumed/core/PlumedMain.h"
#include <sstream>
#include <vector>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <type_traits>

// First we define custom data types
struct vec3d {
  double x;
  double y;
  double z;
};

struct tens3d3d {
  double xx;
  double xy;
  double xz;
  double yx;
  double yy;
  double yz;
  double zx;
  double zy;
  double zz;
  double & operator[](unsigned i) {
    return xx;
  }
  const double & operator[](unsigned i) const {
    return xx;
  }
};

namespace PLMD {


// then we forward declare is_custom_array


// then we provide the needed specializations
template<>
struct CmdArray<vec3d> : std::true_type {
  using value_type = double;
};

template<>
struct CmdArray<tens3d3d> : std::true_type {
  using value_type = vec3d;
};

}

using namespace PLMD;
void wrapperMatrix() {
  std::ostringstream output;
#define P_CREATE PlumedMain p
#include "../../rt-make-10/main.inc"
  std::istringstream lines(output.str());
  std::ofstream normalized("matrix");
  std::string line;
  while(std::getline(lines,line)) {
    auto caught=line.find(" - caught:");
    if(caught!=std::string::npos) {
      line.resize(caught+9);
    }
    normalized<<line<<"\n";
  }
}
