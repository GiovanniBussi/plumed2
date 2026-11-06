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
#include "TypesafePtr.h"
#include "core/PlumedMainInitializer.h"
#if defined(__PLUMED_MPIWRAP) && !defined(__PLUMED_HAS_MPI)
#include "../mpiwrap/mpi.h"
#endif

#include <iostream>

namespace PLMD {

#if defined(__PLUMED_HAS_MPI) || defined(__PLUMED_MPIWRAP)
const MPI_Comm* TypesafePtr::getMPIComm() const {
#if defined(__PLUMED_HAS_MPI)
  // A native MPI_Comm can alias int or void*. Only an explicit communicator
  // request may consume this flag; ordinary get<int*>() must ignore it.
  if(!typesafePtrSkipCheck() && ((myflags & 0x20000000u) || ((myflags>>16)&0xffu)==6)) {
    throw ExceptionTypeError()<<"This command expects a pointer to MPI_Comm"<<extra_msg();
  }
#endif
  return get<const MPI_Comm*>();
}
#endif

TypesafePtr TypesafePtr::fromSafePtr(void* safe) {
  auto s=(plumed_safeptr_x*)safe;
  return TypesafePtr(const_cast<void*>(s->ptr), s->nelem, s->shape, s->flags);
}

TypesafePtr TypesafePtr::copy() const {
  auto passbyvalue=((myflags>>25)&0x7)==1;
  if(passbyvalue) {
    throw ExceptionTypeError()<<"PLUMED is trying to copy the pointer of an object passed by value";
  }
  auto forbidstore=myflags&0x10000000;
  if(forbidstore) {
    throw ExceptionTypeError()<<"PLUMED is trying to copy the pointer of an object for which this was forbidden";
  }
  TypesafePtr ret;
  ret.myptr=myptr;
  ret.myflags=myflags;
  ret.mynelem=mynelem;
  ret.myshape=myshape;
  return ret;
}

std::string TypesafePtr::extra_msg() {
  const char *text = "\n"
                     "If you are sure your code is correct you can disable this check with export PLUMED_TYPESAFE_IGNORE=yes\n"
                     "In case this is necessary, please report an issue to developers of PLUMED and of the MD code\n"
                     "See also https://github.com/plumed/plumed2/pull/653";
  return std::string(text);
}

}


