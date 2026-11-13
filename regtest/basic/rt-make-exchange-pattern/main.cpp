#include "plumed/wrapper/Plumed.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using PLMD::Plumed;

static void require(bool condition, const char* message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}
static void init(Plumed& p, const char* log) {
  p.cmd("setLogFile",log);
  p.cmd("init");
  p.cmd("setNumberOfReplicas",4);
}
static void line(Plumed& p, const std::string& input) {
  p.cmd("readInputLine",input.c_str());
}
static std::vector<int> list(Plumed& p) {
  std::vector<int> result(4,-7);
  p.cmd("getExchangesList",result.data(), {result.size()});
  return result;
}
static int flag(Plumed& p) {
  int result=-1;
  p.cmd("getExchangesFlag",&result);
  return result;
}
static void rejects(Plumed& p, const std::string& input) {
  bool caught=false;
  try {
    line(p,input);
  } catch(const std::exception& e) {
    caught=std::string(e.what()).find("only one exchange-pattern action")!=std::string::npos;
  }
  require(caught,"duplicate provider did not produce the expected diagnostic");
}
int main() {
  try {
    Plumed p,q;
    init(p,"first.log");
    init(q,"second.log");
    require(flag(p)==0 && list(p)==std::vector<int>(4,-7),"no-provider behavior changed");
    // Host seed before an action, input seed overriding it, and a later host seed.
    p.cmd("setExchangesSeed",17);
    line(p,"RANDOM_EXCHANGES");
    line(q,"RANDOM_EXCHANGES SEED=17");
    for(int i=0; i<8; ++i) {
      require(list(p)==list(q),"early host seed was not inherited");
    }
    p.cmd("clear");
    q.cmd("clear");
    require(flag(p)==0 && flag(q)==0,"clear retained a provider");
    p.cmd("setExchangesSeed",5);
    line(p,"RANDOM_EXCHANGES SEED=23");
    line(q,"RANDOM_EXCHANGES SEED=23");
    require(list(p)==list(q),"input seed did not override earlier host seed");
    p.cmd("setExchangesSeed",7);
    q.cmd("clear");
    line(q,"RANDOM_EXCHANGES SEED=7");
    require(list(p)==list(q),"later host seed did not take precedence");
    rejects(p,"duplicate: RANDOM_EXCHANGES");
    p.cmd("clear");
    // An unsuccessful constructor must not register a provider.
    bool failed=false;
    try {
      line(p,"RANDOM_EXCHANGES SEED=not-an-integer");
    } catch(const std::exception&) {
      failed=true;
    }
    require(failed && flag(p)==0,"failed construction retained a provider");
    // Compile and load an action that the kernel has never seen.
    line(p,"LOAD FILE=Pattern.cpp");
    line(p,"TEST_EXCHANGE_PATTERN ORDER=2,0,3,1 REJECT=2,0");
    require(flag(p)>0 && list(p)==std::vector<int>({2,0,3,1}),"LOAD provider was not dispatched");
    require(flag(q)>0,"another object lost its provider");
    rejects(p,"random: RANDOM_EXCHANGES");
    rejects(p,"other: TEST_EXCHANGE_PATTERN ORDER=0,1,2,3");
    p.cmd("clear");
    line(p,"RANDOM_EXCHANGES");
    require(flag(p)>0,"could not replace a cleared LOAD provider");
    // Per-object registration and both duplicate-orderings.
    line(q,"LOAD FILE=Pattern.cpp");
    rejects(q,"fixed: TEST_EXCHANGE_PATTERN ORDER=0,1,2,3");
    q.cmd("clear");
    line(q,"TEST_EXCHANGE_PATTERN ORDER=0,1,2,3");
    require(list(q)==std::vector<int>({0,1,2,3}),"clearing the first object affected the second");
    for(const char* order : {
          "0,1,2", "0,1,2,4", "0,1,2,-1", "0,1,1,3"
        }) {
      q.cmd("clear");
      line(q,std::string("TEST_EXCHANGE_PATTERN ORDER=")+order);
      bool invalid=false;
      try {
        list(q);
      } catch(const std::exception&) {
        invalid=true;
      }
      require(invalid,"invalid permutation reached the host");
    }
    std::ofstream("result") << "exchange providers, seeds, lifecycle, LOAD and validation: OK\n";
  } catch(const std::exception& e) {
    std::ofstream("result") << e.what() << '\n';
    return 1;
  }
}
