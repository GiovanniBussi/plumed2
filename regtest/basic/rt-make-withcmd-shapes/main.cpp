#include "plumed/core/WithCmd.h"
#include "plumed/core/PlumedMain.h"
#include "plumed/tools/Vector.h"
#include "plumed/tools/Tensor.h"
#include <fstream>
#include <vector>
#include <functional>

struct Custom {
  double x[3];
};
namespace PLMD {
template<> struct CmdArray<Custom> : std::true_type {
  using value_type=double;
};
}
using namespace PLMD;

void require(bool ok,int line) {
  if(!ok) {
    throw std::runtime_error("WithCmd check failed at line "+std::to_string(line));
  }
}
#define check(ok) require(bool(ok),__LINE__)
template<class F> void expectFailure(int line,F&& f) {
  bool caught=false;
  try {
    f();
  } catch(const ExceptionTypeError&) {
    caught=true;
  }
  require(caught,line);
}
#define fails(...) expectFailure(__LINE__,__VA_ARGS__)
struct Sink : WithCmd {
  using WithCmd::cmd;
  std::function<void(const TypesafePtr&)> test=[](const auto&) {};
  unsigned calls=0;
  void cmd(std::string_view key,const TypesafePtr& arg=nullptr) override {
    check(key=="check");
    ++calls;
    test(arg);
  }
};
struct Legacy : WithCmd {
  using WithCmd::cmd;
  unsigned calls=0;
  void cmd(const std::string& key,const TypesafePtr& arg=nullptr) override {
    check(key=="check");
    ++calls;
    check(arg.get<int>()==7);
  }
};
void shape(Sink& s,std::initializer_list<std::size_t> dims,bool constant=false,bool nocopy=false) {
  s.test=[dims=std::vector<std::size_t>(dims),constant,nocopy](const TypesafePtr& p) {
    std::size_t n=1;
    unsigned i=0;
    for(auto d:dims) {
      check(p.getShape()[i++]==d);
      n*=d;
    }
    check(p.getShape()[i]==0);
    check(p.getNelem()==n);
    check((p.getFlags()&0xffff)==sizeof(double));
    check(((p.getFlags()>>16)&0xff)==TypesafePtr::is_floating_point);
    check(((p.getFlags()>>25)&7)==(constant?3:2));
    check(bool(p.getFlags()&0x10000000)==nocopy);
    if(nocopy)
      fails([&] { p.copy(); });
    else {
      check(p.copy().getRaw()==p.getRaw());
    }
  };
}
void wrapperMatrix();
int main() {
  wrapperMatrix();
  Sink s;
  double a[2][3] {};
  shape(s, {2,3});
  s.cmd("check",a);
  const double ca[2][3] {};
  shape(s, {2,3},true);
  s.cmd("check",ca);
  std::array<std::array<double,3>,2> aa{};
  shape(s, {2,3});
  s.cmd("check",aa);
  shape(s, {2,3},true);
  s.cmd("check",std::as_const(aa));
  shape(s, {2,3},false,true);
  s.cmd("check",decltype(aa) {});
  std::vector<std::array<double,3>> positions(2);
  shape(s, {2,3});
  s.cmd("check",positions);
  shape(s, {2,3},true);
  s.cmd("check",std::as_const(positions));
  shape(s, {2,3},false,true);
  s.cmd("check",decltype(positions)(2));
  std::vector<Custom> custom(2);
  shape(s, {2,3});
  s.cmd("check",custom);
  shape(s, {2,3},true);
  s.cmd("check",std::as_const(custom));
  std::vector<Vector> native(2);
  shape(s, {2,3});
  s.cmd("check",native);
  const Vector v{};
  shape(s, {3},true);
  s.cmd("check",v);
  Tensor tensor;
  shape(s, {3,3});
  s.cmd("check",tensor);
  shape(s, {3,3},true);
  s.cmd("check",std::as_const(tensor));
  std::vector<Tensor> tensors(2);
  shape(s, {2,3,3});
  s.cmd("check",tensors);
  shape(s, {2,3});
  s.cmd("check",positions.data(), {2});
  std::size_t outer[] {2,0};
  s.cmd("check",positions.data(),outer);
  s.cmd("check",positions.data(),2);
  s.cmd("check",positions.data(), {2,0,99});
  fails([&] {s.cmd("check",positions.data(),{0,0,0,0,0});});
  shape(s, {std::numeric_limits<std::size_t>::max(),3});
  s.cmd("check",positions.data());
  double rank4[1][2][3][4] {};
  shape(s, {1,2,3,4});
  s.cmd("check",rank4);
  double rank5[1][1][1][1][1] {};
  fails([&] { s.cmd("check",rank5); });
  fails([&] { s.cmd("check",a,{1,2,3,4}); });
  s.test=[](const TypesafePtr& p) {
    check(p.getShape()[0]==0);
    check(p.getNelem()==0);
  };
  std::vector<double> empty;
  s.cmd("check",empty);
  std::array<double,0> zero;
  s.cmd("check",zero);
  // Reference's zero-terminated shape representation drops a zero outer axis.
  shape(s, {3});
  std::vector<std::array<double,3>> emptyNested;
  s.cmd("check",emptyNested);
  s.test=[](const TypesafePtr& p) {
    check(!p);
  };
  s.cmd("check");
  s.cmd("check",nullptr);
  double* null=nullptr;
  s.cmd("check",null);
  s.test=[&](const TypesafePtr& p) {
    check(p.getRaw()==a[0]);
    check(p.getShape()[0]==0);
    check(p.getNelem()==0);
  };
  s.cmd("check",&a[0][0]);
  s.test=[](const TypesafePtr& p) {
    check(p.getNelem()==6);
    check(p.getShape()[0]==0);
  };
  s.cmd("check",&a[0][0],6);
  shape(s, {2,3});
  s.cmd("check",&a[0][0], {2,3});
  s.test=[](const TypesafePtr& p) {
    check(p.getCString()==std::string("hello"));
    check(p.getNelem()==6);
    fails([&] {p.get<char*>();});
  };
  s.cmd("check","hello");
  const std::string str="hello";
  s.cmd("check",str);
  s.test=[](const TypesafePtr& p) {
    check(p.getCString()==std::string("hello"));
    check(p.getNelem()==6);
    check(p.get<char*>()[5]==0);
    fails([&] {p.copy();});
  };
  s.cmd("check",std::string("hello"));
  s.test=[](const TypesafePtr& p) {
    check(p.getNelem()==6);
    check(p.get<char*>()[5]==0);
  };
  std::string mutableString="hello";
  s.cmd("check",mutableString);
  s.test=[](const TypesafePtr& p) {
    check(p.get<int>()==7);
    fails([&] {p.copy();});
    fails([&] {p.get<int*>();});
  };
  s.cmd("check",7);
  int value=7;
  s.cmd("check",value);
  const TypesafePtr scalar(7);
  s.cmd("check",scalar);
  s.cmd("check",TypesafePtr(7));
  auto reshaped=TypesafePtr::setNelemAndShape(TypesafePtr(7),1,nullptr);
  s.cmd("check",reshaped);
  s.test=[](const TypesafePtr& p) {
    check(p.get<int>()==7);
  };
  s.cmd("check",&value);
  WithCmd& base=s;
  base.cmd(std::string("check"),&value);
  base.cmd(std::string_view("check"),&value);
  s.cmd(std::string("check"),&value);
  s.cmd(std::string_view("check"),&value);
  Legacy legacy;
  WithCmd& old=legacy;
  legacy.cmd("check",7);
  legacy.cmd(std::string_view("check"),7);
  legacy.cmd(std::string("check"),7);
  old.cmd("check",7);
  old.cmd(std::string_view("check"),7);
  old.cmd(std::string("check"),7);
  check(legacy.calls==6);
  s.test=[](const TypesafePtr& p) {
    fails([&] {p.get<const double*>({2,4});});
    fails([&] {p.get<const double*>({6});});
    fails([&] {p.get<const double*>(7);});
  };
  s.cmd("check",positions);
  s.test=[](const TypesafePtr& p) {
    fails([&] {p.get<double*>();});
    check(p.get<const double*>()!=nullptr);
  };
  s.cmd("check",std::as_const(positions));

  // Real consumer: encoded (natoms,3), retention of lvalues, wrong shape,
  // const writable output and temporary retention checks.
  PlumedMain p;
  p.cmd("setLogFile","test.log");
  p.cmd("setNatoms",2);
  p.cmd("init");
  p.cmd("readInputLines",std::string("d: DISTANCE ATOMS=1,2\nRESTRAINT ARG=d AT=0 KAPPA=1\n"));
  positions[1][0]=1;
  std::vector<std::array<double,3>> forces(2);
  p.cmd("setStep",0);
  p.cmd("setPositions",positions);
  p.cmd("setForces",forces);
  p.cmd("setBox",tensor);
  Tensor virial;
  p.cmd("setVirial",virial);
  p.cmd("calc");
  double bias=0;
  p.cmd("getBias",&bias);
  check(bias==0.5);
  p.cmd("setStep",1);
  fails([&] {p.cmd("setPositions",decltype(positions)(2));});
  std::vector<std::array<double,2>> wrong(2);
  fails([&] {p.cmd("setPositions",wrong); p.cmd("calc");});
  p.cmd("setStep",2);
  p.cmd("setPositions",positions);
  fails([&] {p.cmd("getBias",std::as_const(bias));});
  const auto constForces=forces;
  fails([&] {p.cmd("setForces",constForces); p.cmd("calc");});
  std::ofstream("output")<<"WithCmd metadata, dispatch, lifetime and real commands: OK\n";
}
