#include "plumed/tools/H5Table.h"
#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace PLMD::h5table;
Value integer(long long i) {
  Value value;
  value.type=Value::Int;
  value.i=i;
  return value;
}
Value real(double d) {
  Value value;
  value.type=Value::Double;
  value.d=d;
  return value;
}
Value text(const std::string& s) {
  Value value;
  value.s=s;
  return value;
}
void gate(const char* message) {
  std::puts(message);
  std::fflush(stdout);
  if(std::getchar()==EOF) {
    throw std::runtime_error("controller disappeared");
  }
}
void row(Writer& writer,unsigned k) {
  const long long counts[]= {9007199254740993LL,std::numeric_limits<long long>::max(),std::numeric_limits<long long>::min()};
  writer.addRow({{"time",real(k*0.125)},{"count",integer(counts[k%3])},
    {"label",text(k%2 ? "beta with spaces" : "\xce\xb2")}}, {{"origin",integer(-7)}});
}
int main(int argc,char** argv) {
  if(argc==1) {
    return 0;
  }
  if(argc!=3) {
    return 2;
  }
  const std::string mode=argv[1],path=argv[2];
  try {
    if(mode=="read" || mode=="hold") {
      Reader reader(path);
      if(mode=="hold") {
        gate("opened");
      }
      std::string line;
      while(reader.getline(line)) {
        std::puts(line.c_str());
      }
    } else if(mode=="rows") {
      for(const auto n:rows(path)) {
        std::printf("%llu ",n);
      }
      std::puts("");
    } else if(mode=="trim") {
      truncate(path, {5},path+".removed.h5.tar");
    } else if(mode=="trim-all") {
      truncate(path, {},path+".removed.h5.tar");
    } else if(mode=="append") {
      Writer writer(path,true,3);
      row(writer,8);
      writer.flush();
    } else if(mode=="typed" || mode=="many") {
      Writer writer(path,false,3);
      for(unsigned k=0; k<(mode=="many"?60:8); ++k) {
        row(writer,k);
      }
      writer.flush();
      writer.addRow({{"label",real(-1.5)},{"count",text("changed type")}},
      {{"origin",real(3.5)}});
      writer.flush();
    } else if(mode=="live") {
      auto writer=std::make_unique<Writer>(path,false,4096);
      row(*writer,0);
      writer->flush();
      gate("committed1");
      row(*writer,1);
      gate("buffered2");
      writer->flush();
      gate("committed2");
      writer.reset();
      gate("closed");
    } else if(mode=="timetrim") {
      bool has=false,increasing=false;
      const auto keep=keepBefore(path,0.6,has,increasing);
      if(!has || !increasing) {
        throw std::runtime_error("invalid time trim");
      }
      truncate(path,keep,path+".removed.h5.tar");
    } else {
      return 2;
    }
  } catch(const std::exception& error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
  }
}
