/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2023 The plumed team
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
#ifndef __PLUMED_core_WithCmd_h
#define __PLUMED_core_WithCmd_h

#include "tools/CmdArgument.h"
#include <string>
#include <string_view>

namespace PLMD {

/// Base for classes with cmd() method.
/// This is an abstract base class for classes with
/// cmd() method.
class WithCmd {
  /// Small structure used to pass elements of a shape initializer_list
  struct SizeLike {
    std::size_t size;
    SizeLike(short unsigned dim): size(dim) {}
    SizeLike(unsigned dim): size(dim) {}
    SizeLike(long unsigned dim): size(dim) {}
    SizeLike(long long unsigned dim): size(dim) {}
    SizeLike(short dim): size(std::size_t(dim)) {}
    SizeLike(int dim): size(std::size_t(dim)) {}
    SizeLike(long int dim): size(std::size_t(dim)) {}
    SizeLike(long long int dim): size(std::size_t(dim)) {}
  };
public:
  /// Infer scalar type, shape and temporary lifetime before virtual dispatch.
  /// Explicit TypesafePtr arguments keep using the non-template entry points.
  template<class Key, class T,
           std::enable_if_t<std::is_convertible<Key,std::string_view>::value &&
                            !std::is_same<std::decay_t<T>,TypesafePtr>::value,int> = 0>
  void cmd(Key&& key,T&& val) {
    const auto arg=cmd_detail::argument(std::forward<T>(val));
    cmd(std::string_view(key),arg);
  }

  /// Explicit dimensions describe the outer array of fixed-size objects.
  template<class T, std::enable_if_t<CmdArray<std::remove_cv_t<T>>::value,int> = 0>
  void cmd(std::string_view key,T* val,const std::size_t* shape) {
    std::array<std::size_t,TypesafePtr::maxrank+1> dims{};
    if(shape) {
      unsigned i=0;
      for(; i<TypesafePtr::maxrank && shape[i]; ++i) {
        dims[i]=shape[i];
      }
      if(shape[i]) {
        throw ExceptionTypeError()<<"Maximum shape size is "<<TypesafePtr::maxrank;
      }
    }
    const auto arg=cmd_detail::pointer(val,dims);
    cmd(key,arg);
  }
  template<class T, std::enable_if_t<CmdArray<std::remove_cv_t<T>>::value,int> = 0>
  void cmd(std::string_view key,T* val,std::initializer_list<SizeLike> shape) {
    std::array<std::size_t,TypesafePtr::maxrank+1> dims{};
    if(shape.size()>TypesafePtr::maxrank) {
      throw ExceptionTypeError()<<"Maximum shape size is "<<TypesafePtr::maxrank;
    }
    unsigned i=0;
    for(auto dim: shape) {
      dims[i++]=dim.size;
    }
    cmd(key,val,dims.data());
  }
  template<class T, class I, std::enable_if_t<CmdArray<std::remove_cv_t<T>>::value &&
           std::is_integral<I>::value,int> = 0>
  void cmd(std::string_view key,T* val,I nelem) {
    const std::size_t dims[] {val ? std::size_t(nelem) : 0,0};
    cmd(key,val,dims);
  }

  /// This is the preferred method as it avoid allocations of temporaries.
  /// If this is not overridded, it will call the legacy method.
  virtual void cmd(std::string_view key,const TypesafePtr & val=nullptr) {
    cmd(std::string(key),val);
  }
  /// This is the legacy method we used in older plumed versions, so it is still possible.
  /// If this is not overridden, it will call the preferred method
  virtual void cmd(const std::string& key,const TypesafePtr & val=nullptr) {
    cmd(std::string_view(key),val);
  }
  void cmd(std::string_view key,const TypesafePtr & val,const std::size_t* shape) {
    cmd(key,TypesafePtr::setNelemAndShape(val,0,shape));
  }
  void cmd(std::string_view key,const TypesafePtr & val,std::initializer_list<SizeLike> shape) {
    if(shape.size()>4) {
      plumed_error() << "Maximum shape size is 4";
    }
    std::array<std::size_t,5> shape_;
    unsigned j=0;
    for(auto i : shape) {
      shape_[j]=i.size;
      j++;
    }
    shape_[j]=0;
    cmd(key,val,shape_.data());
  }
  template<typename I, typename std::enable_if<std::is_integral<I>::value, int>::type = 0>
  void cmd(std::string_view key,const TypesafePtr & val,I nelem, const std::size_t* shape=nullptr) {
    cmd(key,TypesafePtr::setNelemAndShape(val,nelem,shape));
  }
  /// This is needed to avoid ambiguities
  void cmd(const char* key,const TypesafePtr & val=nullptr) {
    cmd(std::string_view(key),val);
  }
  virtual ~WithCmd();
};

inline
WithCmd::~WithCmd() {
// do nothing
// here just to allow inheriting from this class properly
}

}

#endif
