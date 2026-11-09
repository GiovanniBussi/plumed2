/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2018-2023 The plumed team
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
#ifndef __PLUMED_tools_CmdArgument_h
#define __PLUMED_tools_CmdArgument_h
#include "TypesafePtr.h"
#include <string>

namespace PLMD {

/// Specialize for packed fixed arrays, providing value_type. The type must
/// consist entirely of contiguous value_type objects, without padding.
/// Unlike size()/data() containers, these can also be nested inside arrays.
template<class T> struct CmdArray : std::false_type {};
template<class T, std::size_t N>
struct CmdArray<T[N]> : std::true_type {
  using value_type=T;
};
template<class T, std::size_t N>
struct CmdArray<std::array<T,N>> : std::true_type {
  using value_type=T;
};

template<class T, unsigned N> class VectorTyped;
template<class T, unsigned N, unsigned M> class TensorTyped;
template<class T, unsigned N>
struct CmdArray<VectorTyped<T,N>> : std::true_type {
  using value_type=T;
};
template<class T, unsigned N, unsigned M>
struct CmdArray<TensorTyped<T,N,M>> : std::true_type {
  using value_type=T;
};

namespace cmd_detail {
template<class T, class=void> struct HasSizeData : std::false_type {};
template<class T> struct HasSizeData<T,std::void_t<
decltype(std::declval<T&>().size()),decltype(std::declval<T&>().data())>> : std::true_type {};

/// Generic custom arrays use the same explicit layout contract as the wrapper.
template<class T> struct FixedArray {
  using value_type=typename CmdArray<T>::value_type;
  static_assert(std::is_standard_layout<T>::value,"cmd custom arrays must have standard layout");
  static_assert(sizeof(T)%sizeof(value_type)==0,"cmd custom array has padding");
  static constexpr std::array<std::size_t,1> shape{sizeof(T)/sizeof(value_type)};
  template<class U> static auto data(U* ptr) {
    using V=std::conditional_t<std::is_const<U>::value,const value_type,value_type>;
    return reinterpret_cast<V*>(ptr);
  }
};
template<class T, std::size_t N> struct FixedArray<T[N]> {
  static constexpr std::array<std::size_t,1> shape{N};
  template<class U> static auto data(U* ptr) {
    return ptr ? *ptr : static_cast<std::remove_extent_t<U>*>(nullptr);
  }
};
template<class T, std::size_t N> struct FixedArray<std::array<T,N>> {
  static constexpr std::array<std::size_t,1> shape{N};
  template<class U> static auto data(U* ptr) {
    using P=decltype(ptr->data());
    static_assert(N==0 || sizeof(U)==N*sizeof(T),"cmd array has padding");
    return ptr ? ptr->data() : P(nullptr);
  }
};
template<class T, unsigned N> struct FixedArray<VectorTyped<T,N>> {
  static constexpr std::array<std::size_t,1> shape{N};
  template<class U> static auto data(U* ptr) {
    static_assert(std::is_standard_layout<U>::value && sizeof(U)==N*sizeof(T),"cmd vector has padding");
    return ptr ? ptr->data() : decltype(ptr->data())(nullptr);
  }
};
template<class T, unsigned N, unsigned M> struct FixedArray<TensorTyped<T,N,M>> {
  static constexpr std::array<std::size_t,2> shape{N,M};
  template<class U> static auto data(U* ptr) {
    static_assert(std::is_standard_layout<U>::value && sizeof(U)==N*M*sizeof(T),"cmd tensor has padding");
    return ptr ? ptr->data() : decltype(ptr->data())(nullptr);
  }
};

inline void append(std::array<std::size_t,TypesafePtr::maxrank+1>& shape,std::size_t dim) {
  unsigned i=0;
  while(i<TypesafePtr::maxrank && shape[i]) {
    ++i;
  }
  if(i==TypesafePtr::maxrank) {
    throw ExceptionTypeError()<<"Maximum shape size is "<<TypesafePtr::maxrank;
  }
  shape[i]=dim;
  shape[i+1]=0;
}

/// Peel only fixed arrays. A container of dynamic containers is not contiguous
/// scalar storage, even if both levels expose size()/data().
template<class T>
TypesafePtr pointer(T* ptr,std::array<std::size_t,TypesafePtr::maxrank+1> shape,bool nocopy=false) {
  using U=std::remove_cv_t<T>;
  if constexpr(CmdArray<U>::value) {
    for(auto dim: FixedArray<U>::shape) {
      append(shape,dim);
    }
    return pointer(FixedArray<U>::data(ptr),shape,nocopy);
  } else {
    static_assert(!HasSizeData<U>::value,"cmd cannot flatten nested dynamic containers");
    TypesafePtr result(ptr,0,shape.data());
    if(nocopy) {
      return TypesafePtr(result.getRaw(),result.getNelem(),result.getShape(),result.getFlags() | 0x10000000u);
    }
    return result;
  }
}

template<class T> TypesafePtr argument(T&& val) {
  using U=std::remove_reference_t<T>;
  using V=std::remove_cv_t<U>;
  constexpr bool temporary=!std::is_lvalue_reference<T>::value;
  if constexpr(CmdArray<V>::value) {
    return pointer(std::addressof(val), {},temporary);
  } else if constexpr(HasSizeData<U>::value) {
    std::size_t size=val.size();
    if constexpr(std::is_same<V,std::string>::value) {
      ++size;
    }
    return pointer(val.data(), {size,0},temporary);
  } else if constexpr(std::is_pointer<V>::value) {
    if constexpr(CmdArray<std::remove_cv_t<std::remove_pointer_t<V>>>::value) {
      return pointer(val, {val ? std::numeric_limits<std::size_t>::max() : 0,0});
    } else {
      return TypesafePtr(val);
    }
  } else {
    static_assert(std::is_constructible<TypesafePtr,T>::value,
                  "cmd expects a scalar, pointer, fixed array, or contiguous size()/data() container");
    return TypesafePtr(std::forward<T>(val));
  }
}
}
}
#endif
