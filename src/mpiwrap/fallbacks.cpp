#include "mpiwrap.h"
#if defined(__PLUMED_MPIWRAP) && !defined(__PLUMED_HAS_MPI)
// Kernel-side algorithms shared by embedded and injected providers.
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>

namespace PLMD {
namespace mpiwrap {
namespace {
// MPI cannot be reinitialized, but the embedded launcher explicitly supports
// sequential tMPI runtimes. Invalidate rank-local cached handles at each launch.
std::atomic<std::uint64_t> datatype_epoch{1};
}

void begin_fallback_datatype_epoch() {
  datatype_epoch.fetch_add(1, std::memory_order_relaxed);
}

std::size_t fallback_contiguous_size(int count, const MPI_Datatype &oldtype) {
  if(count<0 || (oldtype.fallback_size &&
                 static_cast<std::size_t>(count)>std::numeric_limits<std::size_t>::max()/oldtype.fallback_size)) {
    throw std::overflow_error("MPI fallback contiguous datatype size overflows size_t");
  }
  return static_cast<std::size_t>(count)*oldtype.fallback_size;
}

std::size_t fallback_indexed_size(int count,const int *blocks,const MPI_Datatype &oldtype) {
  if(count<0 || (count && !blocks)) {
    throw std::invalid_argument("MPI indexed datatype requires block lengths");
  }
  std::size_t size=0;
  for(int i=0; i<count; ++i) {
    const auto block=fallback_contiguous_size(blocks[i],oldtype);
    if(block>std::numeric_limits<std::size_t>::max()-size) {
      throw std::overflow_error("MPI indexed datatype size overflows size_t");
    }
    size+=block;
  }
  return size;
}

bool fallback_integer_type(const char *name, MPI_Datatype *value) {
  const bool is_unsigned=std::strcmp(name,"MPI_UNSIGNED_LONG_LONG")==0;
  const bool is_signed=std::strcmp(name,"MPI_LONG_LONG")==0 || std::strcmp(name,"MPI_LONG_LONG_INT")==0;
  if(!is_signed && !is_unsigned) {
    return false;
  }
  // LONG_LONG and LONG_LONG_INT are synonyms even if a provider advertises
  // just one of them. No emulation is needed in that case.
  if(is_signed) {
    const auto accessor=pointers::constant_LONG_LONG ? pointers::constant_LONG_LONG : pointers::constant_LONG_LONG_INT;
    if(accessor && accessor(&value->storage,sizeof(value->storage))==0) {
      return true;
    }
  }
  // Representation transport is appropriate for one homogeneous threaded
  // address space. Do not silently impose this on heterogeneous native MPI.
  if(!provider_ranks_are_threads() || !pointers::Type_contiguous ||
      !pointers::Type_commit || !pointers::constant_BYTE) {
    return false;
  }
  value->fallback_kind=is_unsigned ? 2 : 1;
  value->fallback_size=sizeof(unsigned long long);
  return true;
}

plumed_mpi_handle_storage datatype_storage(const MPI_Datatype &value) {
  if(value.fallback_kind!=1 && value.fallback_kind!=2) {
    return value.storage;
  }
  int initialized=0;
  if(Initialized(&initialized)!=SUCCESS() || !initialized) {
    throw std::runtime_error("MPI long-long fallback requires an initialized runtime");
  }
  struct Cache {
    std::uint64_t epoch=0;
    MPI_Datatype carrier{};
  };
  static thread_local Cache cache;
  const auto epoch=datatype_epoch.load(std::memory_order_relaxed);
  if(cache.epoch!=epoch) {
    MPI_Datatype carrier{};
    if(Type_contiguous(static_cast<int>(sizeof(unsigned long long)),BYTE(),&carrier)!=SUCCESS() ||
        Type_commit(&carrier)!=SUCCESS()) {
      throw std::runtime_error("MPI could not create the long-long transport datatype");
    }
    // tMPI owns and releases all committed datatypes on finalization. Keep
    // this predefined carrier alive for outstanding requests and derived types.
    cache.carrier=carrier;
    cache.epoch=epoch;
  }
  static_assert(sizeof(long long)==sizeof(unsigned long long),"signed/unsigned long-long size");
  return cache.carrier.storage;
}

int fallback_integer_count(const MPI_Status *status,const MPI_Datatype &datatype,int *count,bool elements) {
  if(!status || !count) {
    throw std::invalid_argument("MPI status query requires status and count");
  }
  int nbytes=0;
  const int rc=Get_count(status,BYTE(),&nbytes);
  if(rc!=SUCCESS()) {
    return rc;
  }
  const std::size_t extent=elements ? sizeof(unsigned long long) : datatype.fallback_size;
  if(!extent) {
    *count=nbytes==0 ? 0 : UNDEFINED();
    return SUCCESS();
  }
  // Query in transport elements too: a byte count may exceed int even though
  // the requested element count fits. Do not impose an extra INT_MAX/sizeof(T)
  // restriction on otherwise valid transfers.
  MPI_Datatype carrier{};
  if(elements) {
    MPI_Datatype basic{};
    basic.fallback_kind=2;
    carrier.storage=datatype_storage(basic);
  } else {
    carrier.storage=datatype_storage(datatype);
  }
  const int result=Get_count(status,carrier,count);
  // tMPI truncates partial elements. Compare the low byte-count bits too;
  // its byte inquiry can wrap while a valid long-long count still fits int.
  if(result==SUCCESS() && *count!=UNDEFINED()) {
    if(*count<0 || (nbytes!=UNDEFINED() && static_cast<std::uint32_t>(nbytes)!=
                    static_cast<std::uint32_t>(*count)*static_cast<std::uint32_t>(extent))) {
      *count=UNDEFINED();
    }
  }
  return result;
}

namespace mpiwrap_fallback_detail {

bool ready() {
  return pointers::ready.load(std::memory_order_acquire);
}

bool request_constants() {
  return pointers::constant_REQUEST_NULL && pointers::constant_UNDEFINED &&
         pointers::constant_SUCCESS && pointers::constant_ANY_SOURCE && pointers::constant_ANY_TAG;
}

bool collective_primitives() {
  return ready() && pointers::constant_SUCCESS &&
         (pointers::constant_INT || pointers::constant_CHAR || pointers::constant_FLOAT ||
          pointers::constant_DOUBLE || pointers::constant_BYTE || pointers::constant_SHORT ||
          pointers::constant_LONG || pointers::constant_LONG_LONG || pointers::constant_LONG_LONG_INT ||
          pointers::constant_SIGNED_CHAR || pointers::constant_UNSIGNED_CHAR ||
          pointers::constant_UNSIGNED_SHORT || pointers::constant_UNSIGNED || pointers::constant_UNSIGNED_LONG ||
          pointers::constant_UNSIGNED_LONG_LONG || pointers::constant_LONG_DOUBLE ||
          pointers::constant_C_BOOL || pointers::constant_INT64_T) &&
         pointers::Comm_rank && pointers::Comm_size && pointers::Bcast &&
         (pointers::Comm_test_inter ||
          provider_has_capability("capability:intracommunicators-only"));
}

// Basic C datatype extents are fixed by the MPI language binding. Compare the
// provider's handles; their numeric values and representations are immaterial.
// Compound pair types and custom typemaps are deliberately not approximated.
std::size_t basic_extent(const MPI_Datatype &datatype) {
  if(datatype.fallback_kind && datatype.fallback_kind!=4) {
    return datatype.fallback_size;
  }
#define PLUMED_MPIW_BASIC_EXTENT(name, type) \
  if(pointers::constant_ ## name && datatype==name()) return sizeof(type)
  PLUMED_MPIW_BASIC_EXTENT(CHAR,char);
  PLUMED_MPIW_BASIC_EXTENT(SIGNED_CHAR,signed char);
  PLUMED_MPIW_BASIC_EXTENT(UNSIGNED_CHAR,unsigned char);
  PLUMED_MPIW_BASIC_EXTENT(BYTE,unsigned char);
  PLUMED_MPIW_BASIC_EXTENT(SHORT,short);
  PLUMED_MPIW_BASIC_EXTENT(UNSIGNED_SHORT,unsigned short);
  PLUMED_MPIW_BASIC_EXTENT(INT,int);
  PLUMED_MPIW_BASIC_EXTENT(UNSIGNED,unsigned);
  PLUMED_MPIW_BASIC_EXTENT(LONG,long);
  PLUMED_MPIW_BASIC_EXTENT(UNSIGNED_LONG,unsigned long);
  PLUMED_MPIW_BASIC_EXTENT(LONG_LONG,long long);
  PLUMED_MPIW_BASIC_EXTENT(LONG_LONG_INT,long long);
  PLUMED_MPIW_BASIC_EXTENT(UNSIGNED_LONG_LONG,unsigned long long);
  PLUMED_MPIW_BASIC_EXTENT(FLOAT,float);
  PLUMED_MPIW_BASIC_EXTENT(DOUBLE,double);
  PLUMED_MPIW_BASIC_EXTENT(LONG_DOUBLE,long double);
  PLUMED_MPIW_BASIC_EXTENT(C_BOOL,bool);
  PLUMED_MPIW_BASIC_EXTENT(INT64_T,std::int64_t);
#undef PLUMED_MPIW_BASIC_EXTENT
  return 0;
}

std::size_t bytes(int count,std::size_t extent) {
  if(count<0 || (extent && static_cast<std::size_t>(count)>
                 std::numeric_limits<std::size_t>::max()/extent)) {
    throw std::invalid_argument("MPI fallback count or byte extent is invalid");
  }
  return static_cast<std::size_t>(count)*extent;
}

bool in_place(const void *buffer) {
  return pointers::constant_IN_PLACE && buffer==IN_PLACE();
}

void intracommunicator(MPI_Comm comm) {
  if(pointers::Comm_test_inter) {
    int inter=0;
    const int rc=Comm_test_inter(comm,&inter);
    if(rc!=SUCCESS() || inter) {
      throw std::runtime_error("MPI collective fallback requires an intracommunicator");
    }
  } else if(!provider_has_capability("capability:intracommunicators-only")) {
    throw std::runtime_error("MPI provider cannot establish the collective fallback communicator kind");
  }
}

void rank_size(MPI_Comm comm,int &rank,int &size) {
  intracommunicator(comm);
  if(Comm_rank(comm,&rank)!=SUCCESS() ||
      Comm_size(comm,&size)!=SUCCESS() || size<1 || rank<0 || rank>=size) {
    throw std::runtime_error("MPI collective fallback could not query the communicator");
  }
}

std::size_t collective_extent(const MPI_Datatype &type,const char *operation) {
  const std::size_t extent=basic_extent(type);
  if(!extent) {
    throw std::runtime_error(std::string(operation)+" fallback requires a supported basic C datatype (custom/compound types are unsupported)");
  }
  return extent;
}

int some(bool wait,int count,MPI_Request requests[],int *outcount,
         int indices[],MPI_Status statuses[]) {
  if(count<0 || !outcount || (count && (!requests || !indices))) {
    throw std::invalid_argument("MPI request completion fallback requires valid arrays");
  }
  if(requests_all_null(count,requests)) {
    *outcount=UNDEFINED();
    return SUCCESS();
  }
  std::vector<unsigned char> completed_indices(static_cast<std::size_t>(count),0);
  int completed=0,result=SUCCESS();
  while(completed<count) {
    int index=UNDEFINED(),flag=0;
    MPI_Status status{};
    const int rc=wait && completed==0 ?
                 Waitany(count,requests,&index,&status) :
                 Testany(count,requests,&index,&flag,&status);
    if(wait && completed==0) {
      flag=index!=UNDEFINED();
    }
    if(index==UNDEFINED() || !flag) {
      if(rc!=SUCCESS()) {
        result=rc;
      }
      break;
    }
    if(index<0 || index>=count || completed_indices[static_cast<std::size_t>(index)]) {
      throw std::runtime_error("MPI provider returned an invalid completion index");
    }
    completed_indices[static_cast<std::size_t>(index)]=1;
    indices[completed]=index;
    // Waitany/Testany report a single request's error directly. Some-functions
    // collect per-request errors and return MPI_ERR_IN_STATUS instead.
    if(rc!=SUCCESS()) {
      if(rc!=ERR_IN_STATUS()) {
        status.MPI_ERROR=rc;
      }
      result=ERR_IN_STATUS();
    } else {
      status.MPI_ERROR=SUCCESS();
    }
    if(statuses) {
      statuses[completed]=status;
    }
    ++completed;
  }
  *outcount=completed;
  return result;
}

}

bool can_fallback_Wait() {
  return mpiwrap_fallback_detail::ready() && pointers::Waitany &&
         mpiwrap_fallback_detail::request_constants();
}

bool can_fallback_Testsome() {
  return mpiwrap_fallback_detail::ready() && pointers::Testany &&
         pointers::constant_ERR_IN_STATUS && mpiwrap_fallback_detail::request_constants();
}

bool can_fallback_Waitsome() {
  return can_fallback_Testsome() && pointers::Waitany;
}

bool can_fallback_Allgather() {
  return mpiwrap_fallback_detail::collective_primitives();
}

bool can_fallback_Allgatherv() {
  return mpiwrap_fallback_detail::collective_primitives();
}

bool requests_all_null(int count,const MPI_Request requests[]) {
  if(count<0 || (count && !requests)) {
    throw std::invalid_argument("MPI request completion requires a nonnegative count and request array");
  }
  if(count==0) {
    return true;
  }
  const MPI_Request null_request=REQUEST_NULL();
  for(int i=0; i<count; ++i) {
    if(requests[i]!=null_request) {
      return false;
    }
  }
  return true;
}

void set_empty_status(MPI_Status *status) {
  if(status) {
    // No fabricated provider-private status layout: raw_size=0 marks a
    // synthetic empty status, interpreted by PLUMED's status-query wrappers.
    *status=MPI_Status{};
    status->MPI_SOURCE=ANY_SOURCE();
    status->MPI_TAG=ANY_TAG();
    status->MPI_ERROR=SUCCESS();
  }
}

bool is_empty_status(const MPI_Status *status) {
  return status && status->raw_size==0 && pointers::constant_ANY_SOURCE &&
         pointers::constant_ANY_TAG && pointers::constant_SUCCESS &&
         status->MPI_SOURCE==ANY_SOURCE() &&
         status->MPI_TAG==ANY_TAG() && status->MPI_ERROR==SUCCESS();
}

int fallback_Wait(MPI_Request *request,MPI_Status *status) {
  if(!request) {
    throw std::invalid_argument("MPI_Wait requires a request");
  }
  int index=UNDEFINED();
  const int rc=Waitany(1,request,&index,status);
  if(index==UNDEFINED() && rc==SUCCESS()) {
    set_empty_status(status);
  }
  return rc;
}

int fallback_Waitsome(int count,MPI_Request requests[],int *outcount,
                      int indices[],MPI_Status statuses[]) {
  return mpiwrap_fallback_detail::some(true,count,requests,outcount,indices,statuses);
}

int fallback_Testsome(int count,MPI_Request requests[],int *outcount,
                      int indices[],MPI_Status statuses[]) {
  return mpiwrap_fallback_detail::some(false,count,requests,outcount,indices,statuses);
}

int fallback_Allgather(const void *send,int sendcount,const MPI_Datatype &sendtype,
                       void *recv,int recvcount,const MPI_Datatype &recvtype,MPI_Comm comm) {
  int rank=0,size=0;
  mpiwrap_fallback_detail::rank_size(comm,rank,size);
  const std::size_t extent=mpiwrap_fallback_detail::collective_extent(recvtype,"MPI_Allgather");
  const std::size_t segment_bytes=mpiwrap_fallback_detail::bytes(recvcount,extent);
  if(segment_bytes && static_cast<std::size_t>(size)>
      std::numeric_limits<std::size_t>::max()/segment_bytes) {
    throw std::invalid_argument("MPI_Allgather fallback receive extent overflows");
  }
  if(segment_bytes && !recv) {
    throw std::invalid_argument("MPI_Allgather fallback requires a receive buffer");
  }
  if(!mpiwrap_fallback_detail::in_place(send)) {
    mpiwrap_fallback_detail::collective_extent(sendtype,"MPI_Allgather");
    if(sendcount!=recvcount || sendtype!=recvtype || (segment_bytes && !send)) {
      throw std::invalid_argument("MPI_Allgather fallback requires matching basic datatypes/counts");
    }
    if(segment_bytes) {
      std::memmove(static_cast<unsigned char *>(recv)+static_cast<std::size_t>(rank)*segment_bytes,
                   send,segment_bytes);
    }
  }
  for(int root=0; root<size; ++root) {
    void *segment=segment_bytes ? static_cast<unsigned char *>(recv)+
                  static_cast<std::size_t>(root)*segment_bytes : recv;
    const int rc=Bcast(segment,recvcount,recvtype,root,comm);
    if(rc!=SUCCESS()) {
      return rc;
    }
  }
  return SUCCESS();
}

int fallback_Allgatherv(const void *send,int sendcount,const MPI_Datatype &sendtype,
                        void *recv,const int counts[],const int displs[],
                        const MPI_Datatype &recvtype,MPI_Comm comm) {
  int rank=0,size=0;
  mpiwrap_fallback_detail::rank_size(comm,rank,size);
  const std::size_t extent=mpiwrap_fallback_detail::collective_extent(recvtype,"MPI_Allgatherv");
  if(!counts || !displs) {
    throw std::invalid_argument("MPI_Allgatherv fallback requires counts and displacements");
  }
  // Validate every segment before entering any collective. Zero-count gaps
  // must remain untouched, and pointer arithmetic must not overflow.
  for(int i=0; i<size; ++i) {
    const std::size_t length=mpiwrap_fallback_detail::bytes(counts[i],extent);
    const std::size_t offset=mpiwrap_fallback_detail::bytes(displs[i],extent);
    if(offset>std::numeric_limits<std::size_t>::max()-length || (length && !recv)) {
      throw std::invalid_argument("MPI_Allgatherv fallback receive extent is invalid");
    }
  }
  if(!mpiwrap_fallback_detail::in_place(send)) {
    mpiwrap_fallback_detail::collective_extent(sendtype,"MPI_Allgatherv");
    if(sendcount!=counts[rank] || sendtype!=recvtype || (sendcount && !send)) {
      throw std::invalid_argument("MPI_Allgatherv fallback requires matching basic datatypes/counts");
    }
    const std::size_t length=mpiwrap_fallback_detail::bytes(sendcount,extent);
    if(length) {
      std::memmove(static_cast<unsigned char *>(recv)+
                   mpiwrap_fallback_detail::bytes(displs[rank],extent),send,length);
    }
  }
  for(int root=0; root<size; ++root) {
    void *segment=counts[root] ? static_cast<unsigned char *>(recv)+
                  mpiwrap_fallback_detail::bytes(displs[root],extent) : recv;
    const int rc=Bcast(segment,counts[root],recvtype,root,comm);
    if(rc!=SUCCESS()) {
      return rc;
    }
  }
  return SUCCESS();
}

namespace mpiwrap_fallback_detail {
enum class IntegerOp { sum, product, minimum, maximum, land, lor, lxor, band, bor, bxor };

IntegerOp integer_op(MPI_Op op) {
#define PLUMED_MPIW_INTEGER_OP(name, operation) \
  if(pointers::constant_ ## name && op==name()) return IntegerOp::operation
  PLUMED_MPIW_INTEGER_OP(SUM,sum);
  PLUMED_MPIW_INTEGER_OP(PROD,product);
  PLUMED_MPIW_INTEGER_OP(MIN,minimum);
  PLUMED_MPIW_INTEGER_OP(MAX,maximum);
  PLUMED_MPIW_INTEGER_OP(LAND,land);
  PLUMED_MPIW_INTEGER_OP(LOR,lor);
  PLUMED_MPIW_INTEGER_OP(LXOR,lxor);
  PLUMED_MPIW_INTEGER_OP(BAND,band);
  PLUMED_MPIW_INTEGER_OP(BOR,bor);
  PLUMED_MPIW_INTEGER_OP(BXOR,bxor);
#undef PLUMED_MPIW_INTEGER_OP
  throw std::invalid_argument("MPI long-long fallback requires a supported predefined integer operation");
}

template<class T> T combine(T a,T b,IntegerOp op) {
  switch(op) {
  case IntegerOp::sum:
    return a+b;
  case IntegerOp::product:
    return a*b;
  case IntegerOp::minimum:
    return std::min(a,b);
  case IntegerOp::maximum:
    return std::max(a,b);
  case IntegerOp::land:
    return bool(a) && bool(b);
  case IntegerOp::lor:
    return bool(a) || bool(b);
  case IntegerOp::lxor:
    return bool(a)!=bool(b);
  case IntegerOp::band:
    return a&b;
  case IntegerOp::bor:
    return a|b;
  case IntegerOp::bxor:
    return a^b;
  }
  throw std::invalid_argument("Invalid MPI integer operation");
}

template<class T>
int integer_reduce(const char *operation,const void *send,void *recv,int count,
                   const MPI_Datatype &datatype,IntegerOp op,int root,const int *counts,MPI_Comm comm) {
  int rank=0,size=0;
  rank_size(comm,rank,size);
  const bool reduce=std::strcmp(operation,"Reduce")==0;
  const bool scan=std::strcmp(operation,"Scan")==0;
  const bool scatter=std::strcmp(operation,"Reduce_scatter")==0;
  if(reduce && (root<0 || root>=size)) {
    throw std::invalid_argument("MPI reduction fallback requires a valid root");
  }
  if(count<0) {
    throw std::invalid_argument("MPI reduction requires a nonnegative count");
  }
  std::size_t total=static_cast<std::size_t>(count),first=0,last=total;
  if(scatter) {
    if(!counts) {
      throw std::invalid_argument("MPI_Reduce_scatter requires receive counts");
    }
    total=0;
    for(int i=0; i<size; ++i) {
      if(counts[i]<0 || static_cast<std::size_t>(counts[i])>std::numeric_limits<std::size_t>::max()/sizeof(T)-total) {
        throw std::overflow_error("MPI_Reduce_scatter fallback size overflows size_t");
      }
      if(i==rank) {
        first=total;
        last=total+counts[i];
      }
      total+=counts[i];
    }
  }
  if(total>std::numeric_limits<std::size_t>::max()/sizeof(T)) {
    throw std::overflow_error("MPI integer reduction size overflows size_t");
  }
  const std::size_t length=total*sizeof(T);
  const bool inplace=in_place(send);
  if(inplace && reduce && rank!=root) {
    throw std::invalid_argument("MPI_IN_PLACE is valid only at the reduction root");
  }
  const void *input=inplace ? recv : send;
  if((length && !input) || ((!reduce || rank==root) && last>first && !recv)) {
    throw std::invalid_argument("MPI integer reduction requires valid buffers");
  }
  // Bounded scratch space; all ranks participate in the same broadcasts.
  // There are no internal message tags that could collide with host traffic.
  constexpr std::size_t chunk=1024;
  std::vector<T> incoming(std::min(total,chunk));
  std::vector<T> accumulated(incoming.size());
  for(std::size_t offset=0; offset<total;) {
    const int n=static_cast<int>(std::min(total-offset,chunk));
    for(int source=0; source<size; ++source) {
      if(source==rank) {
        std::memcpy(incoming.data(),static_cast<const unsigned char *>(input)+
                    static_cast<std::size_t>(offset)*sizeof(T),static_cast<std::size_t>(n)*sizeof(T));
      }
      const int rc=Bcast(incoming.data(),n,datatype,source,comm);
      if(rc!=SUCCESS()) {
        return rc;
      }
      if(source==0) {
        std::copy(incoming.begin(),incoming.begin()+n,accumulated.begin());
      } else {
        for(int i=0; i<n; ++i) {
          accumulated[i]=combine(accumulated[i],incoming[i],op);
        }
      }
      if(scan && source==rank) {
        std::memcpy(static_cast<unsigned char *>(recv)+static_cast<std::size_t>(offset)*sizeof(T),
                    accumulated.data(),static_cast<std::size_t>(n)*sizeof(T));
      }
    }
    if(!scan && (!reduce || rank==root)) {
      const auto begin=std::max(offset,first),end=std::min(offset+n,last);
      if(end>begin) {
        std::memcpy(static_cast<unsigned char *>(recv)+static_cast<std::size_t>(begin-first)*sizeof(T),
                    accumulated.data()+begin-offset,static_cast<std::size_t>(end-begin)*sizeof(T));
      }
    }
    offset+=n;
  }
  return SUCCESS();
}
}

int fallback_integer_reduce(const char *operation,const void *send,void *recv,int count,
                            const MPI_Datatype &datatype,MPI_Op op,int root,const int *counts,MPI_Comm comm) {
  const auto integer_op=mpiwrap_fallback_detail::integer_op(op);
  if(datatype.fallback_kind==1) {
    return mpiwrap_fallback_detail::integer_reduce<long long>(operation,send,recv,count,datatype,integer_op,root,counts,comm);
  }
  if(datatype.fallback_kind==2) {
    return mpiwrap_fallback_detail::integer_reduce<unsigned long long>(operation,send,recv,count,datatype,integer_op,root,counts,comm);
  }
  throw std::invalid_argument("Predefined MPI integer reductions require a basic datatype");
}

bool try_singleton_reduce(const void *send,void *recv,int count,
                          const MPI_Datatype &type,int root,MPI_Comm comm,int *result) {
  if(!pointers::Comm_size || root>0 || root< -1 || count<0) {
    return false;
  }
  // An intercommunicator reduction uses MPI_ROOT/MPI_PROC_NULL semantics;
  // a local group size of one does not make it a singleton collective.
  if(pointers::Comm_test_inter) {
    int inter=0;
    if(Comm_test_inter(comm,&inter)!=SUCCESS() || inter) {
      return false;
    }
  } else if(!provider_has_capability("capability:intracommunicators-only")) {
    return false;
  }
  const std::size_t extent=mpiwrap_fallback_detail::basic_extent(type);
  if(!extent) {
    return false;
  }
  int size=0;
  if(Comm_size(comm,&size)!=SUCCESS() || size!=1) {
    return false;
  }
  const std::size_t length=mpiwrap_fallback_detail::bytes(count,extent);
  if(length && !recv) {
    throw std::invalid_argument("MPI reduction requires a receive buffer");
  }
  if(!mpiwrap_fallback_detail::in_place(send)) {
    if(length && !send) {
      throw std::invalid_argument("MPI reduction requires a send buffer");
    }
    if(length) {
      std::memmove(recv,send,length);
    }
  }
  *result=SUCCESS();
  return true;
}

} // namespace mpiwrap
}

#endif
