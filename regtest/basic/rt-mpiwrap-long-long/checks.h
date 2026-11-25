// Shared by the native, embedded tMPI and forced-missing provider probes.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include <atomic>

namespace PLMD {
namespace long_long_regtest {

inline void require(bool condition,int line) {
  if(!condition) {
    std::fprintf(stderr,"long-long MPI check failed at line %d\n",line);
    throw std::runtime_error("long-long MPI check failed");
  }
}
#define CHECK(x) require((x),__LINE__)
#define CALL(name, ...) CHECK(MPI_ ## name(__VA_ARGS__)==MPI_SUCCESS)
#if defined(__PLUMED_HAS_MPI)
#define HAS(name) true
#else
#define HAS(name) mpiwrap::has_ ## name()
#endif

template<class T> void collectives(MPI_Comm comm,int rank,int size,MPI_Datatype type) {
  const T base=static_cast<T>(1ULL<<40);
  const T own=base+rank;
  std::vector<T> gathered(size),send(size),receive(size);
  std::vector<int> counts(size,1),displs(size);
  for(int r=0; r<size; ++r) {
    displs[r]=r;
    send[r]=base+100*rank+r;
  }
  for(int root=0; root<size; ++root) {
    T value=rank==root ? std::numeric_limits<T>::max()-1 : 0;
    CALL(Bcast,&value,1,type,root,comm);
    CHECK(value==std::numeric_limits<T>::max()-1);
    CALL(Bcast,&value,0,type,root,comm);
    CALL(Gather,&own,1,type,gathered.data(),1,type,root,comm);
    if(rank==root) {
      for(int r=0; r<size; ++r) {
        CHECK(gathered[r]==base+r);
      }
    }
    CALL(Gatherv,&own,1,type,gathered.data(),counts.data(),displs.data(),type,root,comm);
    if(rank==root) {
      for(int r=0; r<size; ++r) {
        CHECK(gathered[r]==base+r);
      }
    }
    CALL(Scatter,send.data(),1,type,&value,1,type,root,comm);
    CHECK(value==base+100*root+rank);
    CALL(Scatterv,send.data(),counts.data(),displs.data(),type,&value,1,type,root,comm);
    CHECK(value==base+100*root+rank);
    T sum=rank==root ? own : 0;
    CALL(Reduce,rank==root ? MPI_IN_PLACE : &own,&sum,1,type,MPI_SUM,root,comm);
    if(rank==root) {
      CHECK(sum==base*size+size*(size-1)/2);
    }
  }
  CALL(Allgather,&own,1,type,gathered.data(),1,type,comm);
  for(int r=0; r<size; ++r) {
    CHECK(gathered[r]==base+r);
  }
  gathered[rank]=own;
  CALL(Allgather,MPI_IN_PLACE,0,MPI_Datatype{},gathered.data(),1,type,comm);
  for(int r=0; r<size; ++r) {
    CHECK(gathered[r]==base+r);
  }
  CALL(Allgatherv,&own,1,type,gathered.data(),counts.data(),displs.data(),type,comm);
  for(int r=0; r<size; ++r) {
    CHECK(gathered[r]==base+r);
  }
  gathered[rank]=own;
  CALL(Allgatherv,MPI_IN_PLACE,0,MPI_Datatype{},gathered.data(),counts.data(),displs.data(),type,comm);
  CALL(Alltoall,send.data(),1,type,receive.data(),1,type,comm);
  for(int r=0; r<size; ++r) {
    CHECK(receive[r]==base+100*r+rank);
  }
  CALL(Alltoallv,send.data(),counts.data(),displs.data(),type,receive.data(),counts.data(),displs.data(),type,comm);
  for(int r=0; r<size; ++r) {
    CHECK(receive[r]==base+100*r+rank);
  }
  // Uneven counts, a zero-count rank and holes in the receive buffer.
  std::vector<T> varying(size,own),gaps(2*size*size,0);
  for(int r=0; r<size; ++r) {
    counts[r]=r;
    displs[r]=2*size*r;
  }
  CALL(Allgatherv,varying.data(),rank,type,gaps.data(),counts.data(),displs.data(),type,comm);
  for(int r=0; r<size; ++r) {
    for(int i=0; i<r; ++i) {
      CHECK(gaps[displs[r]+i]==base+r);
    }
    CHECK(gaps[displs[r]+r]==0);
  }
  // Native transport construction must preserve element sizes and status counts.
  MPI_Datatype pair{};
  CALL(Type_contiguous,2,type,&pair);
  CALL(Type_commit,&pair);
  T input[2]= {own,own+1},output[2]= {};
  MPI_Status status{};
  CALL(Sendrecv,input,1,pair,(rank+1)%size,40,output,1,pair,(rank+size-1)%size,40,comm,&status);
  CHECK(output[0]==base+(rank+size-1)%size && output[1]==output[0]+1);
  int count=-1;
  CALL(Type_size,type,&count);
  CHECK(count==static_cast<int>(sizeof(T)));
  CALL(Get_count,&status,pair,&count);
  CHECK(count==1);
  CALL(Get_count,&status,type,&count);
  CHECK(count==2);
  if(HAS(Type_size)) {
    CALL(Type_size,pair,&count);
    CHECK(count==2*static_cast<int>(sizeof(T)));
  }
  CALL(Type_free,&pair);
  CHECK(pair==MPI_DATATYPE_NULL);
  CALL(Type_contiguous,0,type,&pair);
  CALL(Type_size,pair,&count);
  CHECK(count==0);
  CALL(Type_free,&pair);
  CALL(Type_contiguous,std::numeric_limits<int>::max(),type,&pair);
  CALL(Type_size,pair,&count);
  CHECK(count==MPI_UNDEFINED);
  CALL(Type_free,&pair);
  if(HAS(Type_vector)) {
    CALL(Type_vector,2,1,2,type,&pair);
    CALL(Type_commit,&pair);
    T data[3]= {own,0,own+1},result[3]= {};
    CALL(Sendrecv,data,1,pair,(rank+1)%size,41,result,1,pair,(rank+size-1)%size,41,comm,&status);
    CHECK(result[0]==base+(rank+size-1)%size && result[1]==0 && result[2]==result[0]+1);
    CALL(Get_count,&status,pair,&count);
    CHECK(count==1);
    CALL(Get_elements,&status,pair,&count);
    CHECK(count==2);
    CALL(Type_size,pair,&count);
    CHECK(count==2*static_cast<int>(sizeof(T)));
    CALL(Type_free,&pair);
  }
  if(HAS(Type_indexed)) {
    int blocks[2]= {1,1},offsets[2]= {0,2};
    CALL(Type_indexed,2,blocks,offsets,type,&pair);
    CALL(Type_commit,&pair);
    T data[3]= {own,0,own+1},result[3]= {};
    CALL(Sendrecv,data,1,pair,(rank+1)%size,42,result,1,pair,(rank+size-1)%size,42,comm,&status);
    CHECK(result[0]==base+(rank+size-1)%size && result[1]==0 && result[2]==result[0]+1);
    CALL(Get_elements,&status,pair,&count);
    CHECK(count==2);
    CALL(Type_free,&pair);
  }
  if(HAS(Pack)) {
    int length=0,position=0,consumed=0;
    CALL(Pack_size,2,type,comm,&length);
    std::vector<char> packed(length);
    CALL(Pack,input,2,type,packed.data(),length,&position,comm);
    CALL(Unpack,packed.data(),position,&consumed,output,2,type,comm);
    CHECK(input[0]==output[0] && input[1]==output[1] && consumed==position);
  }
}

template<class T> void reductions(MPI_Comm comm,int rank,int size,MPI_Datatype type) {
  const T base=static_cast<T>(1ULL<<40);
  const MPI_Op ops[]= {MPI_SUM,MPI_PROD,MPI_MIN,MPI_MAX,MPI_LAND,MPI_LOR,MPI_LXOR,MPI_BAND,MPI_BOR,MPI_BXOR};
  for(int operation=0; operation<10; ++operation) {
    const T value=operation==1 ? rank+1 : (rank==0 ? T(0) : base+rank);
    T expected=operation==1 ? 1 : 0;
    for(int r=operation==1 ? 0 : 1; r<size; ++r) {
      const T next=operation==1 ? r+1 : base+r;
      switch(operation) {
      case 0:
        expected+=next;
        break;
      case 1:
        expected*=next;
        break;
      case 2:
        expected=std::min(expected,next);
        break;
      case 3:
        expected=std::max(expected,next);
        break;
      case 4:
        expected=bool(expected)&&bool(next);
        break;
      case 5:
        expected=bool(expected)||bool(next);
        break;
      case 6:
        expected=bool(expected)!=bool(next);
        break;
      case 7:
        expected&=next;
        break;
      case 8:
        expected|=next;
        break;
      case 9:
        expected^=next;
        break;
      }
    }
    T result=0;
    CALL(Allreduce,&value,&result,1,type,ops[operation],comm);
    CHECK(result==expected);
    result=value;
    CALL(Allreduce,MPI_IN_PLACE,&result,1,type,ops[operation],comm);
    CHECK(result==expected);
  }
  T value=base+rank,result=0;
  CALL(Scan,&value,&result,1,type,MPI_SUM,comm);
  CHECK(result==base*(rank+1)+rank*(rank+1)/2);
  result=value;
  CALL(Scan,MPI_IN_PLACE,&result,1,type,MPI_SUM,comm);
  CHECK(result==base*(rank+1)+rank*(rank+1)/2);
  if(std::numeric_limits<T>::is_signed) {
    value=-base-rank;
    CALL(Allreduce,&value,&result,1,type,MPI_MIN,comm);
    CHECK(result==-base-(size-1));
    CALL(Allreduce,&value,&result,1,type,MPI_SUM,comm);
    CHECK(result==-base*size-size*(size-1)/2);
    value=std::numeric_limits<T>::lowest()+rank;
    CALL(Allreduce,&value,&result,1,type,MPI_MIN,comm);
    CHECK(result==std::numeric_limits<T>::lowest());
  } else {
    value=std::numeric_limits<T>::max()-rank;
    CALL(Allreduce,&value,&result,1,type,MPI_MAX,comm);
    CHECK(result==std::numeric_limits<T>::max());
    CALL(Allreduce,&value,&result,1,type,MPI_SUM,comm);
    T expected=0;
    for(int r=0; r<size; ++r) {
      expected+=std::numeric_limits<T>::max()-r;
    }
    CHECK(result==expected);
  }
  // Exercise chunk boundaries and keep scratch space independent of count.
  std::vector<T> many(2051,base+rank),summed(many.size());
  CALL(Allreduce,many.data(),summed.data(),static_cast<int>(many.size()),type,MPI_SUM,comm);
  for(T x:summed) {
    CHECK(x==base*size+size*(size-1)/2);
  }
  summed=many;
  CALL(Scan,MPI_IN_PLACE,summed.data(),static_cast<int>(summed.size()),type,MPI_SUM,comm);
  for(T x:summed) {
    CHECK(x==base*(rank+1)+rank*(rank+1)/2);
  }
  CALL(Reduce,many.data(),summed.data(),static_cast<int>(many.size()),type,MPI_SUM,size-1,comm);
  if(rank==size-1)
    for(T x:summed) {
      CHECK(x==base*size+size*(size-1)/2);
    }
  CALL(Allreduce,nullptr,nullptr,0,type,MPI_SUM,comm);
  if(HAS(Reduce_scatter)) {
    std::vector<int> counts(size,2);
    std::vector<T> input(2*size,base+rank),output(2);
    CALL(Reduce_scatter,input.data(),output.data(),counts.data(),type,MPI_SUM,comm);
    CHECK(output[0]==base*size+size*(size-1)/2 && output[1]==output[0]);
    CALL(Reduce_scatter,MPI_IN_PLACE,input.data(),counts.data(),type,MPI_SUM,comm);
    CHECK(input[0]==output[0] && input[1]==output[1]);
    int total=0,first=0;
    for(int r=0; r<size; ++r) {
      counts[r]=(r==0 && size>1) ? 0 : 1025+7*r;
      if(r<rank) {
        first+=counts[r];
      }
      total+=counts[r];
    }
    input.resize(total);
    output.resize(counts[rank]);
    for(int i=0; i<total; ++i) {
      input[i]=base+100*rank+i;
    }
    CALL(Reduce_scatter,input.data(),output.data(),counts.data(),type,MPI_SUM,comm);
    for(int i=0; i<counts[rank]; ++i) {
      CHECK(output[i]==base*size+100*size*(size-1)/2+(first+i)*size);
    }
    CALL(Reduce_scatter,MPI_IN_PLACE,input.data(),counts.data(),type,MPI_SUM,comm);
    for(int i=0; i<counts[rank]; ++i) {
      CHECK(input[i]==output[i]);
    }
  }
}

template<class T> void requests(MPI_Comm comm,int rank,int size,MPI_Datatype type) {
  const int next=(rank+1)%size,previous=(rank+size-1)%size;
  const T base=static_cast<T>(1ULL<<40);
  T send=base+rank,recv=0;
  MPI_Request pending[2] {};
  MPI_Status statuses[2] {};
  int count=-1;
  CALL(Irecv,&recv,1,type,previous,50,comm,&pending[0]);
  CALL(Isend,&send,1,type,next,50,comm,&pending[1]);
  CALL(Waitall,2,pending,statuses);
  CHECK(recv==base+previous);
  CALL(Get_count,&statuses[0],type,&count);
  CHECK(count==1);
  CHECK(statuses[0].MPI_SOURCE==previous && statuses[0].MPI_TAG==50);
  // The ready-send precondition is established by a barrier after every Irecv.
  CALL(Irecv,&recv,1,type,previous,51,comm,&pending[0]);
  CALL(Barrier,comm);
  CALL(Rsend,&send,1,type,next,51,comm);
  CALL(Wait,&pending[0],&statuses[0]);
  CHECK(recv==base+previous);
  CALL(Irecv,&recv,1,type,previous,52,comm,&pending[0]);
  CALL(Barrier,comm);
  CALL(Irsend,&send,1,type,next,52,comm,&pending[1]);
  for(int i=0; i<2; ++i) {
    int index=-1;
    CALL(Waitany,2,pending,&index,&statuses[i]);
    CHECK(index>=0 && index<2);
  }
  CHECK(recv==base+previous);
  CALL(Irecv,&recv,1,type,previous,53,comm,&pending[0]);
  CALL(Send,&send,1,type,next,53,comm);
  CALL(Wait,&pending[0],&statuses[0]);
  CHECK(recv==base+previous);
  CALL(Isend,&send,1,type,next,54,comm,&pending[1]);
  if(HAS(Probe)) {
    CALL(Probe,previous,54,comm,&statuses[0]);
    CALL(Get_count,&statuses[0],type,&count);
    CHECK(count==1);
  }
  if(HAS(Iprobe)) {
    int flag=0;
    while(!flag) {
      CALL(Iprobe,previous,54,comm,&flag,&statuses[0]);
    }
    CALL(Get_count,&statuses[0],type,&count);
    CHECK(count==1);
  }
  CALL(Recv,&recv,1,type,previous,54,comm,&statuses[0]);
  CALL(Wait,&pending[1],MPI_STATUS_IGNORE);
  CHECK(recv==base+previous);
  CALL(Sendrecv,&send,1,type,next,55,&recv,1,type,previous,55,comm,&statuses[0]);
  CHECK(recv==base+previous);
  if(HAS(Sendrecv_replace)) {
    recv=send;
    CALL(Sendrecv_replace,&recv,1,type,next,56,previous,56,comm,&statuses[0]);
    CHECK(recv==base+previous);
  }
  if(HAS(Get_elements)) {
    CALL(Get_elements,&statuses[0],type,&count);
    CHECK(count==1);
  }
  // Every completion family must retain the original element count.
  for(int mode=0; mode<5; ++mode) {
    if((mode==0 && !HAS(Test)) || (mode==1 && !HAS(Testany)) ||
        (mode==2 && !HAS(Testall)) || (mode==3 && !HAS(Waitsome)) ||
        (mode==4 && !HAS(Testsome))) {
      continue;
    }
    CALL(Irecv,&recv,1,type,previous,60+mode,comm,&pending[0]);
    CALL(Isend,&send,1,type,next,60+mode,comm,&pending[1]);
    if(mode==0) {
      for(int i=0; i<2; ++i) {
        int flag=0;
        while(!flag) {
          CALL(Test,&pending[i],&flag,&statuses[i]);
        }
      }
    } else if(mode==1) {
      for(int i=0; i<2; ++i) {
        int flag=0,index=-1;
        while(!flag) {
          CALL(Testany,2,pending,&index,&flag,&statuses[0]);
        }
        CHECK(index>=0 && index<2);
      }
    } else if(mode==2) {
      int flag=0;
      while(!flag) {
        CALL(Testall,2,pending,&flag,statuses);
      }
    } else {
      int completed=0;
      while(completed<2) {
        int n=0,indices[2]= {};
        if(mode==3) {
          CALL(Waitsome,2,pending,&n,indices,statuses);
        } else {
          CALL(Testsome,2,pending,&n,indices,statuses);
        }
        CHECK(n>=0 && n<=2-completed);
        for(int i=0; i<n; ++i)
          if(indices[i]==0) {
            CALL(Get_count,&statuses[i],type,&count);
            CHECK(count==1);
          }
        completed+=n;
      }
    }
    CHECK(recv==base+previous);
  }
  if(HAS(Ssend)) {
    CALL(Irecv,&recv,1,type,previous,65,comm,&pending[0]);
    CALL(Ssend,&send,1,type,next,65,comm);
    CALL(Wait,&pending[0],MPI_STATUS_IGNORE);
    CHECK(recv==base+previous);
  }
  if(HAS(Issend)) {
    CALL(Irecv,&recv,1,type,previous,66,comm,&pending[0]);
    CALL(Issend,&send,1,type,next,66,comm,&pending[1]);
    CALL(Waitall,2,pending,statuses);
    CHECK(recv==base+previous);
  }
  if(HAS(Bsend) && HAS(Buffer_attach)) {
    std::vector<char> buffer(65536);
    CALL(Buffer_attach,buffer.data(),static_cast<int>(buffer.size()));
    CALL(Irecv,&recv,1,type,previous,67,comm,&pending[0]);
    CALL(Bsend,&send,1,type,next,67,comm);
    CALL(Wait,&pending[0],MPI_STATUS_IGNORE);
    CHECK(recv==base+previous);
    CALL(Irecv,&recv,1,type,previous,68,comm,&pending[0]);
    CALL(Ibsend,&send,1,type,next,68,comm,&pending[1]);
    CALL(Waitall,2,pending,statuses);
    CHECK(recv==base+previous);
    void *detached=nullptr;
    int length=0;
    CALL(Buffer_detach,&detached,&length);
    CHECK(detached==buffer.data() && length==static_cast<int>(buffer.size()));
  }
  if(HAS(Cancel) && HAS(Test_cancelled)) {
    CALL(Irecv,&recv,1,type,previous,69,comm,&pending[0]);
    CALL(Cancel,&pending[0]);
    CALL(Wait,&pending[0],&statuses[0]);
    int cancelled=0;
    CALL(Test_cancelled,&statuses[0],&cancelled);
    CHECK(cancelled);
  }
  if(HAS(Send_init) && HAS(Recv_init)) {
    CALL(Recv_init,&recv,1,type,previous,57,comm,&pending[0]);
    CALL(Send_init,&send,1,type,next,57,comm,&pending[1]);
    for(int iteration=0; iteration<2; ++iteration) {
      if(iteration) {
        CALL(Startall,2,pending);
      } else {
        CALL(Start,&pending[0]);
        CALL(Start,&pending[1]);
      }
      CALL(Waitall,2,pending,statuses);
      CHECK(recv==base+previous);
    }
    CALL(Request_free,&pending[0]);
    CALL(Request_free,&pending[1]);
  }
  if(HAS(Ssend_init) && HAS(Rsend_init) && HAS(Bsend_init)) {
    for(int mode=0; mode<3; ++mode) {
      std::vector<char> buffer(65536);
      if(mode==2) {
        CALL(Buffer_attach,buffer.data(),static_cast<int>(buffer.size()));
      }
      CALL(Recv_init,&recv,1,type,previous,70+mode,comm,&pending[0]);
      if(mode==0) {
        CALL(Ssend_init,&send,1,type,next,70+mode,comm,&pending[1]);
      }
      if(mode==1) {
        CALL(Rsend_init,&send,1,type,next,70+mode,comm,&pending[1]);
      }
      if(mode==2) {
        CALL(Bsend_init,&send,1,type,next,70+mode,comm,&pending[1]);
      }
      CALL(Start,&pending[0]);
      CALL(Barrier,comm);
      CALL(Start,&pending[1]);
      CALL(Waitall,2,pending,statuses);
      CHECK(recv==base+previous);
      CALL(Request_free,&pending[0]);
      CALL(Request_free,&pending[1]);
      if(mode==2) {
        void *detached=nullptr;
        int n=0;
        CALL(Buffer_detach,&detached,&n);
      }
    }
  }
  // A status can be queried with another datatype. A partial logical element
  // must report MPI_UNDEFINED rather than tMPI's truncated integer division.
  char bytes[3]= {},received[3]= {};
  CALL(Sendrecv,bytes,3,MPI_BYTE,next,74,received,3,MPI_BYTE,previous,74,comm,&statuses[0]);
  CALL(Get_count,&statuses[0],type,&count);
  CHECK(count==MPI_UNDEFINED);
}

inline void checks(MPI_Comm comm,int rank,int size) {
  collectives<unsigned long long>(comm,rank,size,MPI_UNSIGNED_LONG_LONG);
  reductions<unsigned long long>(comm,rank,size,MPI_UNSIGNED_LONG_LONG);
  requests<unsigned long long>(comm,rank,size,MPI_UNSIGNED_LONG_LONG);
  CHECK(MPI_LONG_LONG==MPI_LONG_LONG_INT);
  collectives<long long>(comm,rank,size,MPI_LONG_LONG_INT);
  reductions<long long>(comm,rank,size,MPI_LONG_LONG_INT);
  requests<long long>(comm,rank,size,MPI_LONG_LONG_INT);
}

#if !defined(__PLUMED_HAS_MPI)
inline mpiwrap::plumed_mpi_MPI_Allreduce_pointer &original_allreduce() {
  static mpiwrap::plumed_mpi_MPI_Allreduce_pointer pointer=nullptr;
  return pointer;
}
inline std::atomic<int> &forwarded_allreduces() {
  static std::atomic<int> count{0};
  return count;
}
inline std::int32_t counted_allreduce(const void *send,void *recv,std::int32_t count,
                                      mpiwrap::plumed_mpi_handle_storage type,
                                      mpiwrap::plumed_mpi_handle_storage op,
                                      mpiwrap::plumed_mpi_handle_storage comm) {
  forwarded_allreduces().fetch_add(1);
  return original_allreduce()(send,recv,count,type,op,comm);
}

inline void fallback_checks(MPI_Comm comm,int rank,int size,bool shared_ranks,
                            void (*extra)(MPI_Comm,int,int)=nullptr) {
  // Only the private test process mutates the dispatch pointers. Barriers
  // keep thread ranks away from every change and from saved-value writes.
  static mpiwrap::plumed_mpi_provider_constant_pointer saved[5];
  const bool owner=!shared_ranks || rank==0;
  if(owner) {
    saved[0]=mpiwrap::pointers::constant_LONG_LONG;
    saved[1]=mpiwrap::pointers::constant_LONG_LONG_INT;
    saved[2]=mpiwrap::pointers::constant_UNSIGNED_LONG_LONG;
    saved[3]=mpiwrap::pointers::constant_LONG;
    saved[4]=mpiwrap::pointers::constant_UNSIGNED_LONG;
    mpiwrap::pointers::constant_LONG_LONG=nullptr;
    mpiwrap::pointers::constant_LONG_LONG_INT=nullptr;
    mpiwrap::pointers::constant_UNSIGNED_LONG_LONG=nullptr;
  }
  CALL(Barrier,comm);
  CHECK(MPI_LONG_LONG_INT.fallback_kind==1 && MPI_UNSIGNED_LONG_LONG.fallback_kind==2);
  for(bool force_generic: {
        false,true
      }) {
    if(owner) {
      mpiwrap::pointers::constant_LONG=force_generic ? nullptr : saved[3];
      mpiwrap::pointers::constant_UNSIGNED_LONG=force_generic ? nullptr : saved[4];
      original_allreduce()=mpiwrap::pointers::Allreduce;
      forwarded_allreduces().store(0);
      mpiwrap::pointers::Allreduce=&counted_allreduce;
    }
    CALL(Barrier,comm);
    checks(comm,rank,size);
    if(extra) {
      extra(comm,rank,size);
    }
    CALL(Barrier,comm);
    const bool fast=!force_generic && sizeof(long)==sizeof(long long) && saved[3] && saved[4];
    if(size>1) {
      CHECK((forwarded_allreduces().load()>0)==fast);
    }
    if(rank==0) {
      std::printf("long=%u long long=%u: %s reduction path passed\n",
                  static_cast<unsigned>(sizeof(long)),static_cast<unsigned>(sizeof(long long)),
                  fast ? "native-long" : "full-width");
    }
    if(owner) {
      mpiwrap::pointers::Allreduce=original_allreduce();
    }
    CALL(Barrier,comm);
  }
  if(owner) {
    mpiwrap::pointers::constant_LONG_LONG=saved[0];
    mpiwrap::pointers::constant_LONG_LONG_INT=saved[1];
    mpiwrap::pointers::constant_UNSIGNED_LONG_LONG=saved[2];
    mpiwrap::pointers::constant_LONG=saved[3];
    mpiwrap::pointers::constant_UNSIGNED_LONG=saved[4];
  }
  CALL(Barrier,comm);
}
#endif

#undef CHECK
#undef CALL
#undef HAS
}
}
