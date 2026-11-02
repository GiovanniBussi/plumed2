/* Native process-MPI adapter body. C-compatible implementation. */

#define PLUMED_MPIW_JOIN_INNER(a, b) a##b
#define PLUMED_MPIW_JOIN(a, b) PLUMED_MPIW_JOIN_INNER(a, b)
#define PLUMED_MPIW_STATIC_ASSERT(c, m) \
  typedef char PLUMED_MPIW_JOIN(plumed_mpiw_static_assert_, __LINE__)[(c) ? 1 : -1]

PLUMED_MPIW_STATIC_ASSERT(sizeof(int) == sizeof(int32_t), "MPI C int transport requires int32_t");
PLUMED_MPIW_STATIC_ASSERT(((int)-1) < 0, "MPI C int transport requires signed int");
#if defined(__cplusplus)
extern "C" {
#endif
/* These compatible redeclarations enforce int32_t == int without a helper call. */
void plumed_mpiw_verify_native_int(int *value);
void plumed_mpiw_verify_native_int(int32_t *value);
#if defined(__cplusplus)
}
#endif
PLUMED_MPIW_STATIC_ASSERT(sizeof(plumed_mpiw_handle_storage) >= sizeof(MPI_Comm), "Comm storage too small");
PLUMED_MPIW_STATIC_ASSERT(sizeof(plumed_mpiw_handle_storage) >= sizeof(MPI_Datatype), "Datatype storage too small");
PLUMED_MPIW_STATIC_ASSERT(sizeof(plumed_mpiw_handle_storage) >= sizeof(MPI_Request), "Request storage too small");
PLUMED_MPIW_STATIC_ASSERT(sizeof(plumed_mpiw_handle_storage) >= sizeof(MPI_Op), "Op storage too small");
PLUMED_MPIW_STATIC_ASSERT(sizeof(MPI_Status) <= PLUMED_MPIW_STATUS_CAPACITY, "Status storage too small");
PLUMED_MPIW_STATIC_ASSERT(sizeof(MPI_Fint) == sizeof(int32_t), "MPI_Fint transport requires int32_t");
PLUMED_MPIW_STATIC_ASSERT(((MPI_Fint)-1) < 0, "MPI_Fint must be signed");

static MPI_Comm plumed_mpiw_import_comm(plumed_mpiw_handle_storage value) {
  MPI_Comm result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static MPI_Datatype plumed_mpiw_import_datatype(plumed_mpiw_handle_storage value) {
  MPI_Datatype result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static MPI_Op plumed_mpiw_import_op(plumed_mpiw_handle_storage value) {
  MPI_Op result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static MPI_Group plumed_mpiw_import_group(plumed_mpiw_handle_storage value) {
  MPI_Group result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static MPI_Errhandler plumed_mpiw_import_errhandler(plumed_mpiw_handle_storage value) {
  MPI_Errhandler result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static MPI_Request plumed_mpiw_import_request(plumed_mpiw_handle_storage value) {
  MPI_Request result;
  memset(&result, 0, sizeof(result));
  memcpy(&result, &value, sizeof(result));
  return result;
}
static plumed_mpiw_handle_storage plumed_mpiw_export_bytes(const void *value, size_t size) {
  plumed_mpiw_handle_storage result = (plumed_mpiw_handle_storage)0;
  memcpy(&result, value, size);
  return result;
}
static void plumed_mpiw_export_status(const MPI_Status *source, plumed_mpiw_status *target) {
  if (!target) {
    return;
  }
  memset(target, 0, sizeof(*target));
  target->source = (int32_t)source->MPI_SOURCE;
  target->tag = (int32_t)source->MPI_TAG;
  target->error = (int32_t)source->MPI_ERROR;
  target->raw_size = (uint32_t)sizeof(*source);
  memcpy(target->raw, source, sizeof(*source));
}
static int plumed_mpiw_import_status(const plumed_mpiw_status *source, MPI_Status *target) {
  if (!source || source->raw_size != sizeof(*target)) {
    return 0;
  }
  memcpy(target, source->raw, sizeof(*target));
  return 1;
}

static int32_t plumed_mpiw_MPI_Initialized(int32_t *flag) {
  int native_flag = 0;
  int result;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  result = MPI_Initialized(&native_flag);
  if (result == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Comm_dup(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage *newcomm) {
  MPI_Comm native_result;
  int result;
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  memset(&native_result, 0, sizeof(native_result));
  result = MPI_Comm_dup(plumed_mpiw_import_comm(comm), &native_result);
  if (result == MPI_SUCCESS) {
    *newcomm = plumed_mpiw_export_bytes(&native_result, sizeof(native_result));
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Comm_free(plumed_mpiw_handle_storage *comm) {
  MPI_Comm native_comm;
  int result;
  if (!comm) {
    return MPI_ERR_ARG;
  }
  native_comm = plumed_mpiw_import_comm(*comm);
  result = MPI_Comm_free(&native_comm);
  *comm = plumed_mpiw_export_bytes(&native_comm, sizeof(native_comm));
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Comm_rank(plumed_mpiw_handle_storage comm, int32_t *rank) {
  int native_rank = 0;
  int result;
  if (!rank) {
    return MPI_ERR_ARG;
  }
  result = MPI_Comm_rank(plumed_mpiw_import_comm(comm), &native_rank);
  if (result == MPI_SUCCESS) {
    *rank = (int32_t)native_rank;
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Comm_size(plumed_mpiw_handle_storage comm, int32_t *size) {
  int native_size = 0;
  int result;
  if (!size) {
    return MPI_ERR_ARG;
  }
  result = MPI_Comm_size(plumed_mpiw_import_comm(comm), &native_size);
  if (result == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Comm_split(plumed_mpiw_handle_storage comm, int32_t color, int32_t key, plumed_mpiw_handle_storage *newcomm) {
  MPI_Comm native_result;
  int result;
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  memset(&native_result, 0, sizeof(native_result));
  result = MPI_Comm_split(plumed_mpiw_import_comm(comm), (int)color, (int)key, &native_result);
  if (result == MPI_SUCCESS) {
    *newcomm = plumed_mpiw_export_bytes(&native_result, sizeof(native_result));
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Abort(plumed_mpiw_handle_storage comm, int32_t errorcode) {
  return (int32_t)MPI_Abort(plumed_mpiw_import_comm(comm), (int)errorcode);
}
static int32_t plumed_mpiw_MPI_Bcast(void *buffer, int32_t count, plumed_mpiw_handle_storage datatype, int32_t root, plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Bcast(buffer, (int)count, plumed_mpiw_import_datatype(datatype), (int)root, plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Scatter(const void *sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void *recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, int32_t root, plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Scatter((void *)sendbuf, (int)sendcount, plumed_mpiw_import_datatype(sendtype), recvbuf, (int)recvcount, plumed_mpiw_import_datatype(recvtype), (int)root, plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Gather(const void *sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void *recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, int32_t root, plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Gather((void *)sendbuf, (int)sendcount, plumed_mpiw_import_datatype(sendtype), recvbuf, (int)recvcount, plumed_mpiw_import_datatype(recvtype), (int)root, plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Allreduce(const void *sendbuf, void *recvbuf, int32_t count, plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage op, plumed_mpiw_handle_storage comm) {
  /* Old MPI headers may lack MPI-5 input const qualifiers; this is input-only. */
  return (int32_t)MPI_Allreduce((void *)sendbuf, recvbuf, (int)count, plumed_mpiw_import_datatype(datatype), plumed_mpiw_import_op(op), plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Isend(const void *buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage *request) {
  MPI_Request native_request;
  int result;
  if (!request) {
    return MPI_ERR_ARG;
  }
  memset(&native_request, 0, sizeof(native_request));
  result = MPI_Isend((void *)buf, (int)count, plumed_mpiw_import_datatype(datatype), (int)dest, (int)tag, plumed_mpiw_import_comm(comm), &native_request);
  if (result == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Wait(plumed_mpiw_handle_storage *request, plumed_mpiw_status *status) {
  MPI_Request native_request;
  MPI_Status native_status;
  int result;
  if (!request) {
    return MPI_ERR_ARG;
  }
  native_request = plumed_mpiw_import_request(*request);
  memset(&native_status, 0, sizeof(native_status));
  result = MPI_Wait(&native_request, status ? &native_status : MPI_STATUS_IGNORE);
  *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  if (result == MPI_SUCCESS && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Allgather(const void *sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void *recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Allgather((void *)sendbuf, (int)sendcount, plumed_mpiw_import_datatype(sendtype), recvbuf, (int)recvcount, plumed_mpiw_import_datatype(recvtype), plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Allgatherv(const void *sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void *recvbuf, const int32_t recvcounts[], const int32_t displs[], plumed_mpiw_handle_storage recvtype, plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Allgatherv((void *)sendbuf, (int)sendcount, plumed_mpiw_import_datatype(sendtype), recvbuf, (const int *)recvcounts, (const int *)displs, plumed_mpiw_import_datatype(recvtype), plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Recv(void *buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t source, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_status *status) {
  MPI_Status native_status;
  int result;
  memset(&native_status, 0, sizeof(native_status));
  result = MPI_Recv(buf, (int)count, plumed_mpiw_import_datatype(datatype), (int)source, (int)tag, plumed_mpiw_import_comm(comm), status ? &native_status : MPI_STATUS_IGNORE);
  if (result == MPI_SUCCESS && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Barrier(plumed_mpiw_handle_storage comm) {
  return (int32_t)MPI_Barrier(plumed_mpiw_import_comm(comm));
}
static int32_t plumed_mpiw_MPI_Get_count(const plumed_mpiw_status *status, plumed_mpiw_handle_storage datatype, int32_t *count) {
  MPI_Status native_status;
  int native_count = 0;
  int result;
  if (!status || !count) {
    return MPI_ERR_ARG;
  }
  if (!plumed_mpiw_import_status(status, &native_status)) {
    return MPI_ERR_ARG;
  }
  result = MPI_Get_count(&native_status, plumed_mpiw_import_datatype(datatype), &native_count);
  if (result == MPI_SUCCESS) {
    *count = (int32_t)native_count;
  }
  return (int32_t)result;
}
static int32_t plumed_mpiw_MPI_Sendrecv(const void *sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, int32_t dest, int32_t sendtag, void *recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, int32_t source, int32_t recvtag, plumed_mpiw_handle_storage comm, plumed_mpiw_status *status) {
  MPI_Status native_status;
  int result;
  memset(&native_status, 0, sizeof(native_status));
  result = MPI_Sendrecv((void *)sendbuf, (int)sendcount, plumed_mpiw_import_datatype(sendtype), (int)dest, (int)sendtag, recvbuf, (int)recvcount, plumed_mpiw_import_datatype(recvtype), (int)source, (int)recvtag, plumed_mpiw_import_comm(comm), status ? &native_status : MPI_STATUS_IGNORE);
  if (result == MPI_SUCCESS && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)result;
}
static plumed_mpiw_handle_storage plumed_mpiw_MPI_Comm_f2c(int32_t comm) {
  MPI_Comm native_comm = MPI_Comm_f2c((MPI_Fint)comm);
  return plumed_mpiw_export_bytes(&native_comm, sizeof(native_comm));
}


static int32_t plumed_mpiw_MPI_Error_string(int32_t errorcode, char * string, int32_t * resultlen) {
  if (!resultlen) {
    return MPI_ERR_ARG;
  }
  int native_errorcode = (int)errorcode;
  int native_resultlen = 0;
  if (!resultlen) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Error_string(native_errorcode, string, &native_resultlen);
  if (native_return_value == MPI_SUCCESS) {
    *resultlen = (int32_t)native_resultlen;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Send(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Send((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Bsend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Bsend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Ssend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Ssend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Rsend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Rsend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Buffer_attach(void * buffer, int32_t size) {
  int native_size = (int)size;
  int native_return_value = MPI_Buffer_attach(buffer, native_size);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Buffer_detach(void * buffer_addr, int32_t * size) {
  if (!buffer_addr) {
    return MPI_ERR_ARG;
  }
  if (!size) {
    return MPI_ERR_ARG;
  }
  if (!buffer_addr) {
    return MPI_ERR_ARG;
  }
  int native_size = 0;
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Buffer_detach(buffer_addr, &native_size);
  if (native_return_value == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Ibsend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Ibsend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Issend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Issend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Irsend(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Irsend((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Irecv(void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t source, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_source = (int)source;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Irecv(buf, native_count, native_datatype, native_source, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Test(plumed_mpiw_handle_storage * request, int32_t * flag, plumed_mpiw_status * status) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  native_request = plumed_mpiw_import_request(*request);
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Test(&native_request, &native_flag, status ? &native_status : MPI_STATUS_IGNORE);
  if (1) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  if ((native_return_value == MPI_SUCCESS && native_flag) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Request_free(plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  native_request = plumed_mpiw_import_request(*request);
  int native_return_value = MPI_Request_free(&native_request);
  if (1) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Waitany(int32_t count, plumed_mpiw_handle_storage array_of_requests[], int32_t * indx, plumed_mpiw_status * status) {
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)count * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (!indx) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_indx = 0;
  if (!indx) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Waitany(native_count, native_array_of_requests, &native_indx, status ? &native_status : MPI_STATUS_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if (native_return_value == MPI_SUCCESS) {
    *indx = (int32_t)native_indx;
  }
  if ((native_return_value == MPI_SUCCESS && native_indx != MPI_UNDEFINED) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Testany(int32_t count, plumed_mpiw_handle_storage array_of_requests[], int32_t * indx, int32_t * flag, plumed_mpiw_status * status) {
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)count * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (!indx) {
    return MPI_ERR_ARG;
  }
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_indx = 0;
  if (!indx) {
    return MPI_ERR_ARG;
  }
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Testany(native_count, native_array_of_requests, &native_indx, &native_flag, status ? &native_status : MPI_STATUS_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if (native_return_value == MPI_SUCCESS) {
    *indx = (int32_t)native_indx;
  }
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  if ((native_return_value == MPI_SUCCESS && native_flag && native_indx != MPI_UNDEFINED) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Waitall(int32_t count, plumed_mpiw_handle_storage array_of_requests[], plumed_mpiw_status * array_of_statuses) {
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)count * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_statuses = (int)count * 1;
  if ((size_t)extent_array_of_statuses > SIZE_MAX / sizeof(MPI_Status)) {
    return MPI_ERR_COUNT;
  }
  int native_count = (int)count;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  MPI_Status *native_array_of_statuses = NULL;
  if (extent_array_of_statuses > 0 && array_of_statuses) {
    native_array_of_statuses = (MPI_Status *)calloc((size_t)extent_array_of_statuses, sizeof(*native_array_of_statuses));
    if (!native_array_of_statuses) {
      free(native_array_of_requests);
      return MPI_ERR_NO_MEM;
    }
  }
  int native_return_value = MPI_Waitall(native_count, native_array_of_requests, array_of_statuses ? native_array_of_statuses : MPI_STATUSES_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if (array_of_statuses && (native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS))
    for (int i = 0; i < extent_array_of_statuses; ++i) {
      plumed_mpiw_export_status(&native_array_of_statuses[i], &array_of_statuses[i]);
    }
  free(native_array_of_statuses);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Testall(int32_t count, plumed_mpiw_handle_storage array_of_requests[], int32_t * flag, plumed_mpiw_status * array_of_statuses) {
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)count * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (!flag) {
    return MPI_ERR_ARG;
  }
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_statuses = (int)count * 1;
  if ((size_t)extent_array_of_statuses > SIZE_MAX / sizeof(MPI_Status)) {
    return MPI_ERR_COUNT;
  }
  int native_count = (int)count;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Status *native_array_of_statuses = NULL;
  if (extent_array_of_statuses > 0 && array_of_statuses) {
    native_array_of_statuses = (MPI_Status *)calloc((size_t)extent_array_of_statuses, sizeof(*native_array_of_statuses));
    if (!native_array_of_statuses) {
      free(native_array_of_requests);
      return MPI_ERR_NO_MEM;
    }
  }
  int native_return_value = MPI_Testall(native_count, native_array_of_requests, &native_flag, array_of_statuses ? native_array_of_statuses : MPI_STATUSES_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if ((native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS)) {
    *flag = (int32_t)native_flag;
  }
  if (array_of_statuses && (native_return_value == MPI_ERR_IN_STATUS || (native_return_value == MPI_SUCCESS && native_flag)))
    for (int i = 0; i < extent_array_of_statuses; ++i) {
      plumed_mpiw_export_status(&native_array_of_statuses[i], &array_of_statuses[i]);
    }
  free(native_array_of_statuses);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Waitsome(int32_t incount, plumed_mpiw_handle_storage array_of_requests[], int32_t * outcount, int32_t array_of_indices[], plumed_mpiw_status * array_of_statuses) {
  if (incount < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)incount * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (!outcount) {
    return MPI_ERR_ARG;
  }
  if (incount < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_statuses = (int)incount * 1;
  if ((size_t)extent_array_of_statuses > SIZE_MAX / sizeof(MPI_Status)) {
    return MPI_ERR_COUNT;
  }
  int native_incount = (int)incount;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_outcount = 0;
  if (!outcount) {
    return MPI_ERR_ARG;
  }
  MPI_Status *native_array_of_statuses = NULL;
  if (extent_array_of_statuses > 0 && array_of_statuses) {
    native_array_of_statuses = (MPI_Status *)calloc((size_t)extent_array_of_statuses, sizeof(*native_array_of_statuses));
    if (!native_array_of_statuses) {
      free(native_array_of_requests);
      return MPI_ERR_NO_MEM;
    }
  }
  int native_return_value = MPI_Waitsome(native_incount, native_array_of_requests, &native_outcount, array_of_indices, array_of_statuses ? native_array_of_statuses : MPI_STATUSES_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if ((native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS)) {
    *outcount = (int32_t)native_outcount;
  }
  if (array_of_statuses && ((native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS) && native_outcount > 0 && native_outcount != MPI_UNDEFINED))
    for (int i = 0; i < native_outcount; ++i) {
      plumed_mpiw_export_status(&native_array_of_statuses[i], &array_of_statuses[i]);
    }
  free(native_array_of_statuses);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Testsome(int32_t incount, plumed_mpiw_handle_storage array_of_requests[], int32_t * outcount, int32_t array_of_indices[], plumed_mpiw_status * array_of_statuses) {
  if (incount < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)incount * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  if (!outcount) {
    return MPI_ERR_ARG;
  }
  if (incount < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_statuses = (int)incount * 1;
  if ((size_t)extent_array_of_statuses > SIZE_MAX / sizeof(MPI_Status)) {
    return MPI_ERR_COUNT;
  }
  int native_incount = (int)incount;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_outcount = 0;
  if (!outcount) {
    return MPI_ERR_ARG;
  }
  MPI_Status *native_array_of_statuses = NULL;
  if (extent_array_of_statuses > 0 && array_of_statuses) {
    native_array_of_statuses = (MPI_Status *)calloc((size_t)extent_array_of_statuses, sizeof(*native_array_of_statuses));
    if (!native_array_of_statuses) {
      free(native_array_of_requests);
      return MPI_ERR_NO_MEM;
    }
  }
  int native_return_value = MPI_Testsome(native_incount, native_array_of_requests, &native_outcount, array_of_indices, array_of_statuses ? native_array_of_statuses : MPI_STATUSES_IGNORE);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  if ((native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS)) {
    *outcount = (int32_t)native_outcount;
  }
  if (array_of_statuses && ((native_return_value == MPI_SUCCESS || native_return_value == MPI_ERR_IN_STATUS) && native_outcount > 0 && native_outcount != MPI_UNDEFINED))
    for (int i = 0; i < native_outcount; ++i) {
      plumed_mpiw_export_status(&native_array_of_statuses[i], &array_of_statuses[i]);
    }
  free(native_array_of_statuses);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Iprobe(int32_t source, int32_t tag, plumed_mpiw_handle_storage comm, int32_t * flag, plumed_mpiw_status * status) {
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_source = (int)source;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Iprobe(native_source, native_tag, native_comm, &native_flag, status ? &native_status : MPI_STATUS_IGNORE);
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  if ((native_return_value == MPI_SUCCESS && native_flag) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Probe(int32_t source, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_status * status) {
  int native_source = (int)source;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Probe(native_source, native_tag, native_comm, status ? &native_status : MPI_STATUS_IGNORE);
  if ((native_return_value == MPI_SUCCESS) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cancel(plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  native_request = plumed_mpiw_import_request(*request);
  int native_return_value = MPI_Cancel(&native_request);
  if (1) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Test_cancelled(const plumed_mpiw_status * status, int32_t * flag) {
  if (!status) {
    return MPI_ERR_ARG;
  }
  if (status->raw_size != sizeof(MPI_Status)) {
    return MPI_ERR_ARG;
  }
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  if (!plumed_mpiw_import_status(status, &native_status)) {
    return MPI_ERR_ARG;
  }
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Test_cancelled(&native_status, &native_flag);
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Send_init(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Send_init((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Bsend_init(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Bsend_init((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Ssend_init(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Ssend_init((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Rsend_init(const void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Rsend_init((void *)buf, native_count, native_datatype, native_dest, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Recv_init(void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t source, int32_t tag, plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_source = (int)source;
  int native_tag = (int)tag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Recv_init(buf, native_count, native_datatype, native_source, native_tag, native_comm, &native_request);
  if (native_return_value == MPI_SUCCESS) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Start(plumed_mpiw_handle_storage * request) {
  if (!request) {
    return MPI_ERR_ARG;
  }
  MPI_Request native_request;
  memset(&native_request, 0, sizeof(native_request));
  if (!request) {
    return MPI_ERR_ARG;
  }
  native_request = plumed_mpiw_import_request(*request);
  int native_return_value = MPI_Start(&native_request);
  if (1) {
    *request = plumed_mpiw_export_bytes(&native_request, sizeof(native_request));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Startall(int32_t count, plumed_mpiw_handle_storage array_of_requests[]) {
  if (count < 0) {
    return MPI_ERR_COUNT;
  }
  int extent_array_of_requests = (int)count * 1;
  if (extent_array_of_requests > 0 && !array_of_requests) {
    return MPI_ERR_ARG;
  }
  if ((size_t)extent_array_of_requests > SIZE_MAX / sizeof(MPI_Request)) {
    return MPI_ERR_COUNT;
  }
  int native_count = (int)count;
  MPI_Request *native_array_of_requests = NULL;
  if (extent_array_of_requests > 0) {
    native_array_of_requests = (MPI_Request *)calloc((size_t)extent_array_of_requests, sizeof(*native_array_of_requests));
    if (!native_array_of_requests) {
      return MPI_ERR_NO_MEM;
    }
  }
  for (int i = 0; i < extent_array_of_requests; ++i) {
    native_array_of_requests[i] = plumed_mpiw_import_request(array_of_requests[i]);
  }
  int native_return_value = MPI_Startall(native_count, native_array_of_requests);
  if (1)
    for (int i = 0; i < extent_array_of_requests; ++i) {
      array_of_requests[i] = plumed_mpiw_export_bytes(&native_array_of_requests[i], sizeof(native_array_of_requests[i]));
    }
  free(native_array_of_requests);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Sendrecv_replace(void * buf, int32_t count, plumed_mpiw_handle_storage datatype, int32_t dest, int32_t sendtag, int32_t source, int32_t recvtag, plumed_mpiw_handle_storage comm, plumed_mpiw_status * status) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_dest = (int)dest;
  int native_sendtag = (int)sendtag;
  int native_source = (int)source;
  int native_recvtag = (int)recvtag;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  int native_return_value = MPI_Sendrecv_replace(buf, native_count, native_datatype, native_dest, native_sendtag, native_source, native_recvtag, native_comm, status ? &native_status : MPI_STATUS_IGNORE);
  if ((native_return_value == MPI_SUCCESS) && status) {
    plumed_mpiw_export_status(&native_status, status);
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_contiguous(int32_t count, plumed_mpiw_handle_storage oldtype, plumed_mpiw_handle_storage * newtype) {
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_oldtype = plumed_mpiw_import_datatype(oldtype);
  MPI_Datatype native_newtype;
  memset(&native_newtype, 0, sizeof(native_newtype));
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Type_contiguous(native_count, native_oldtype, &native_newtype);
  if (native_return_value == MPI_SUCCESS) {
    *newtype = plumed_mpiw_export_bytes(&native_newtype, sizeof(native_newtype));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_vector(int32_t count, int32_t blocklength, int32_t stride, plumed_mpiw_handle_storage oldtype, plumed_mpiw_handle_storage * newtype) {
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  int native_blocklength = (int)blocklength;
  int native_stride = (int)stride;
  MPI_Datatype native_oldtype = plumed_mpiw_import_datatype(oldtype);
  MPI_Datatype native_newtype;
  memset(&native_newtype, 0, sizeof(native_newtype));
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Type_vector(native_count, native_blocklength, native_stride, native_oldtype, &native_newtype);
  if (native_return_value == MPI_SUCCESS) {
    *newtype = plumed_mpiw_export_bytes(&native_newtype, sizeof(native_newtype));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_indexed(int32_t count, const int32_t array_of_blocklengths[], const int32_t array_of_displacements[], plumed_mpiw_handle_storage oldtype, plumed_mpiw_handle_storage * newtype) {
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_count = (int)count;
  MPI_Datatype native_oldtype = plumed_mpiw_import_datatype(oldtype);
  MPI_Datatype native_newtype;
  memset(&native_newtype, 0, sizeof(native_newtype));
  if (!newtype) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Type_indexed(native_count, array_of_blocklengths, array_of_displacements, native_oldtype, &native_newtype);
  if (native_return_value == MPI_SUCCESS) {
    *newtype = plumed_mpiw_export_bytes(&native_newtype, sizeof(native_newtype));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_commit(plumed_mpiw_handle_storage * datatype) {
  if (!datatype) {
    return MPI_ERR_ARG;
  }
  MPI_Datatype native_datatype;
  memset(&native_datatype, 0, sizeof(native_datatype));
  if (!datatype) {
    return MPI_ERR_ARG;
  }
  native_datatype = plumed_mpiw_import_datatype(*datatype);
  int native_return_value = MPI_Type_commit(&native_datatype);
  if (1) {
    *datatype = plumed_mpiw_export_bytes(&native_datatype, sizeof(native_datatype));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_free(plumed_mpiw_handle_storage * datatype) {
  if (!datatype) {
    return MPI_ERR_ARG;
  }
  MPI_Datatype native_datatype;
  memset(&native_datatype, 0, sizeof(native_datatype));
  if (!datatype) {
    return MPI_ERR_ARG;
  }
  native_datatype = plumed_mpiw_import_datatype(*datatype);
  int native_return_value = MPI_Type_free(&native_datatype);
  if (1) {
    *datatype = plumed_mpiw_export_bytes(&native_datatype, sizeof(native_datatype));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Type_size(plumed_mpiw_handle_storage datatype, int32_t * size) {
  if (!size) {
    return MPI_ERR_ARG;
  }
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_size = 0;
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Type_size(native_datatype, &native_size);
  if (native_return_value == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Get_elements(const plumed_mpiw_status * status, plumed_mpiw_handle_storage datatype, int32_t * count) {
  if (!status) {
    return MPI_ERR_ARG;
  }
  if (status->raw_size != sizeof(MPI_Status)) {
    return MPI_ERR_ARG;
  }
  if (!count) {
    return MPI_ERR_ARG;
  }
  MPI_Status native_status;
  if (status) {
    memset(&native_status, 0, sizeof(native_status));
  }
  if (!plumed_mpiw_import_status(status, &native_status)) {
    return MPI_ERR_ARG;
  }
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_count = 0;
  if (!count) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Get_elements(&native_status, native_datatype, &native_count);
  if (native_return_value == MPI_SUCCESS) {
    *count = (int32_t)native_count;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Pack(const void * inbuf, int32_t incount, plumed_mpiw_handle_storage datatype, void * outbuf, int32_t outsize, int32_t * position, plumed_mpiw_handle_storage comm) {
  if (!position) {
    return MPI_ERR_ARG;
  }
  int native_incount = (int)incount;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  int native_outsize = (int)outsize;
  int native_position = 0;
  if (!position) {
    return MPI_ERR_ARG;
  }
  native_position = (int)*position;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Pack((void *)inbuf, native_incount, native_datatype, outbuf, native_outsize, &native_position, native_comm);
  *position = (int32_t)native_position;
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Unpack(const void * inbuf, int32_t insize, int32_t * position, void * outbuf, int32_t outcount, plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage comm) {
  if (!position) {
    return MPI_ERR_ARG;
  }
  int native_insize = (int)insize;
  int native_position = 0;
  if (!position) {
    return MPI_ERR_ARG;
  }
  native_position = (int)*position;
  int native_outcount = (int)outcount;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Unpack((void *)inbuf, native_insize, &native_position, outbuf, native_outcount, native_datatype, native_comm);
  *position = (int32_t)native_position;
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Pack_size(int32_t incount, plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage comm, int32_t * size) {
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_incount = (int)incount;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_size = 0;
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Pack_size(native_incount, native_datatype, native_comm, &native_size);
  if (native_return_value == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Gatherv(const void * sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void * recvbuf, const int32_t recvcounts[], const int32_t displs[], plumed_mpiw_handle_storage recvtype, int32_t root, plumed_mpiw_handle_storage comm) {
  int native_sendcount = (int)sendcount;
  MPI_Datatype native_sendtype = plumed_mpiw_import_datatype(sendtype);
  MPI_Datatype native_recvtype = plumed_mpiw_import_datatype(recvtype);
  int native_root = (int)root;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Gatherv((void *)sendbuf, native_sendcount, native_sendtype, recvbuf, recvcounts, displs, native_recvtype, native_root, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Scatterv(const void * sendbuf, const int32_t sendcounts[], const int32_t displs[], plumed_mpiw_handle_storage sendtype, void * recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, int32_t root, plumed_mpiw_handle_storage comm) {
  MPI_Datatype native_sendtype = plumed_mpiw_import_datatype(sendtype);
  int native_recvcount = (int)recvcount;
  MPI_Datatype native_recvtype = plumed_mpiw_import_datatype(recvtype);
  int native_root = (int)root;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Scatterv((void *)sendbuf, sendcounts, displs, native_sendtype, recvbuf, native_recvcount, native_recvtype, native_root, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Alltoall(const void * sendbuf, int32_t sendcount, plumed_mpiw_handle_storage sendtype, void * recvbuf, int32_t recvcount, plumed_mpiw_handle_storage recvtype, plumed_mpiw_handle_storage comm) {
  int native_sendcount = (int)sendcount;
  MPI_Datatype native_sendtype = plumed_mpiw_import_datatype(sendtype);
  int native_recvcount = (int)recvcount;
  MPI_Datatype native_recvtype = plumed_mpiw_import_datatype(recvtype);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Alltoall((void *)sendbuf, native_sendcount, native_sendtype, recvbuf, native_recvcount, native_recvtype, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Alltoallv(const void * sendbuf, const int32_t sendcounts[], const int32_t sdispls[], plumed_mpiw_handle_storage sendtype, void * recvbuf, const int32_t recvcounts[], const int32_t rdispls[], plumed_mpiw_handle_storage recvtype, plumed_mpiw_handle_storage comm) {
  MPI_Datatype native_sendtype = plumed_mpiw_import_datatype(sendtype);
  MPI_Datatype native_recvtype = plumed_mpiw_import_datatype(recvtype);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Alltoallv((void *)sendbuf, sendcounts, sdispls, native_sendtype, recvbuf, recvcounts, rdispls, native_recvtype, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Reduce(const void * sendbuf, void * recvbuf, int32_t count, plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage op, int32_t root, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  MPI_Op native_op = plumed_mpiw_import_op(op);
  int native_root = (int)root;
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Reduce((void *)sendbuf, recvbuf, native_count, native_datatype, native_op, native_root, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Op_free(plumed_mpiw_handle_storage * op) {
  if (!op) {
    return MPI_ERR_ARG;
  }
  MPI_Op native_op;
  memset(&native_op, 0, sizeof(native_op));
  if (!op) {
    return MPI_ERR_ARG;
  }
  native_op = plumed_mpiw_import_op(*op);
  int native_return_value = MPI_Op_free(&native_op);
  if (1) {
    *op = plumed_mpiw_export_bytes(&native_op, sizeof(native_op));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Reduce_scatter(const void * sendbuf, void * recvbuf, const int32_t recvcounts[], plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage op, plumed_mpiw_handle_storage comm) {
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  MPI_Op native_op = plumed_mpiw_import_op(op);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Reduce_scatter((void *)sendbuf, recvbuf, recvcounts, native_datatype, native_op, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Scan(const void * sendbuf, void * recvbuf, int32_t count, plumed_mpiw_handle_storage datatype, plumed_mpiw_handle_storage op, plumed_mpiw_handle_storage comm) {
  int native_count = (int)count;
  MPI_Datatype native_datatype = plumed_mpiw_import_datatype(datatype);
  MPI_Op native_op = plumed_mpiw_import_op(op);
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_return_value = MPI_Scan((void *)sendbuf, recvbuf, native_count, native_datatype, native_op, native_comm);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_size(plumed_mpiw_handle_storage group, int32_t * size) {
  if (!size) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_size = 0;
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_size(native_group, &native_size);
  if (native_return_value == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_rank(plumed_mpiw_handle_storage group, int32_t * rank) {
  if (!rank) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_rank = 0;
  if (!rank) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_rank(native_group, &native_rank);
  if (native_return_value == MPI_SUCCESS) {
    *rank = (int32_t)native_rank;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_translate_ranks(plumed_mpiw_handle_storage group1, int32_t n, const int32_t ranks1[], plumed_mpiw_handle_storage group2, int32_t ranks2[]) {
  MPI_Group native_group1 = plumed_mpiw_import_group(group1);
  int native_n = (int)n;
  MPI_Group native_group2 = plumed_mpiw_import_group(group2);
  int native_return_value = MPI_Group_translate_ranks(native_group1, native_n, ranks1, native_group2, ranks2);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_compare(plumed_mpiw_handle_storage group1, plumed_mpiw_handle_storage group2, int32_t * result) {
  if (!result) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group1 = plumed_mpiw_import_group(group1);
  MPI_Group native_group2 = plumed_mpiw_import_group(group2);
  int native_result = 0;
  if (!result) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_compare(native_group1, native_group2, &native_result);
  if (native_return_value == MPI_SUCCESS) {
    *result = (int32_t)native_result;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_group(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * group) {
  if (!group) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Group native_group;
  memset(&native_group, 0, sizeof(native_group));
  if (!group) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_group(native_comm, &native_group);
  if (native_return_value == MPI_SUCCESS) {
    *group = plumed_mpiw_export_bytes(&native_group, sizeof(native_group));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_union(plumed_mpiw_handle_storage group1, plumed_mpiw_handle_storage group2, plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group1 = plumed_mpiw_import_group(group1);
  MPI_Group native_group2 = plumed_mpiw_import_group(group2);
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_union(native_group1, native_group2, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_intersection(plumed_mpiw_handle_storage group1, plumed_mpiw_handle_storage group2, plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group1 = plumed_mpiw_import_group(group1);
  MPI_Group native_group2 = plumed_mpiw_import_group(group2);
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_intersection(native_group1, native_group2, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_difference(plumed_mpiw_handle_storage group1, plumed_mpiw_handle_storage group2, plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group1 = plumed_mpiw_import_group(group1);
  MPI_Group native_group2 = plumed_mpiw_import_group(group2);
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_difference(native_group1, native_group2, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_incl(plumed_mpiw_handle_storage group, int32_t n, const int32_t ranks[], plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_n = (int)n;
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_incl(native_group, native_n, ranks, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_excl(plumed_mpiw_handle_storage group, int32_t n, const int32_t ranks[], plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_n = (int)n;
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_excl(native_group, native_n, ranks, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_range_incl(plumed_mpiw_handle_storage group, int32_t n, int32_t ranges[][3], plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_n = (int)n;
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_range_incl(native_group, native_n, ranges, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_range_excl(plumed_mpiw_handle_storage group, int32_t n, int32_t ranges[][3], plumed_mpiw_handle_storage * newgroup) {
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group = plumed_mpiw_import_group(group);
  int native_n = (int)n;
  MPI_Group native_newgroup;
  memset(&native_newgroup, 0, sizeof(native_newgroup));
  if (!newgroup) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Group_range_excl(native_group, native_n, ranges, &native_newgroup);
  if (native_return_value == MPI_SUCCESS) {
    *newgroup = plumed_mpiw_export_bytes(&native_newgroup, sizeof(native_newgroup));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Group_free(plumed_mpiw_handle_storage * group) {
  if (!group) {
    return MPI_ERR_ARG;
  }
  MPI_Group native_group;
  memset(&native_group, 0, sizeof(native_group));
  if (!group) {
    return MPI_ERR_ARG;
  }
  native_group = plumed_mpiw_import_group(*group);
  int native_return_value = MPI_Group_free(&native_group);
  if (1) {
    *group = plumed_mpiw_export_bytes(&native_group, sizeof(native_group));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_compare(plumed_mpiw_handle_storage comm1, plumed_mpiw_handle_storage comm2, int32_t * result) {
  if (!result) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm1 = plumed_mpiw_import_comm(comm1);
  MPI_Comm native_comm2 = plumed_mpiw_import_comm(comm2);
  int native_result = 0;
  if (!result) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_compare(native_comm1, native_comm2, &native_result);
  if (native_return_value == MPI_SUCCESS) {
    *result = (int32_t)native_result;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_create(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage group, plumed_mpiw_handle_storage * newcomm) {
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Group native_group = plumed_mpiw_import_group(group);
  MPI_Comm native_newcomm;
  memset(&native_newcomm, 0, sizeof(native_newcomm));
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_create(native_comm, native_group, &native_newcomm);
  if (native_return_value == MPI_SUCCESS) {
    *newcomm = plumed_mpiw_export_bytes(&native_newcomm, sizeof(native_newcomm));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_test_inter(plumed_mpiw_handle_storage comm, int32_t * flag) {
  if (!flag) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_test_inter(native_comm, &native_flag);
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_remote_size(plumed_mpiw_handle_storage comm, int32_t * size) {
  if (!size) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_size = 0;
  if (!size) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_remote_size(native_comm, &native_size);
  if (native_return_value == MPI_SUCCESS) {
    *size = (int32_t)native_size;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_remote_group(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * group) {
  if (!group) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Group native_group;
  memset(&native_group, 0, sizeof(native_group));
  if (!group) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_remote_group(native_comm, &native_group);
  if (native_return_value == MPI_SUCCESS) {
    *group = plumed_mpiw_export_bytes(&native_group, sizeof(native_group));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Intercomm_create(plumed_mpiw_handle_storage local_comm, int32_t local_leader, plumed_mpiw_handle_storage peer_comm, int32_t remote_leader, int32_t tag, plumed_mpiw_handle_storage * newintercomm) {
  if (!newintercomm) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_local_comm = plumed_mpiw_import_comm(local_comm);
  int native_local_leader = (int)local_leader;
  MPI_Comm native_peer_comm = plumed_mpiw_import_comm(peer_comm);
  int native_remote_leader = (int)remote_leader;
  int native_tag = (int)tag;
  MPI_Comm native_newintercomm;
  memset(&native_newintercomm, 0, sizeof(native_newintercomm));
  if (!newintercomm) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Intercomm_create(native_local_comm, native_local_leader, native_peer_comm, native_remote_leader, native_tag, &native_newintercomm);
  if (native_return_value == MPI_SUCCESS) {
    *newintercomm = plumed_mpiw_export_bytes(&native_newintercomm, sizeof(native_newintercomm));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Intercomm_merge(plumed_mpiw_handle_storage intercomm, int32_t high, plumed_mpiw_handle_storage * newintracomm) {
  if (!newintracomm) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_intercomm = plumed_mpiw_import_comm(intercomm);
  int native_high = (int)high;
  MPI_Comm native_newintracomm;
  memset(&native_newintracomm, 0, sizeof(native_newintracomm));
  if (!newintracomm) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Intercomm_merge(native_intercomm, native_high, &native_newintracomm);
  if (native_return_value == MPI_SUCCESS) {
    *newintracomm = plumed_mpiw_export_bytes(&native_newintracomm, sizeof(native_newintracomm));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_create(plumed_mpiw_handle_storage comm_old, int32_t ndims, const int32_t dims[], const int32_t periods[], int32_t reorder, plumed_mpiw_handle_storage * comm_cart) {
  if (!comm_cart) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm_old = plumed_mpiw_import_comm(comm_old);
  int native_ndims = (int)ndims;
  int native_reorder = (int)reorder;
  MPI_Comm native_comm_cart;
  memset(&native_comm_cart, 0, sizeof(native_comm_cart));
  if (!comm_cart) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cart_create(native_comm_old, native_ndims, dims, periods, native_reorder, &native_comm_cart);
  if (native_return_value == MPI_SUCCESS) {
    *comm_cart = plumed_mpiw_export_bytes(&native_comm_cart, sizeof(native_comm_cart));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Dims_create(int32_t nnodes, int32_t ndims, int32_t dims[]) {
  int native_nnodes = (int)nnodes;
  int native_ndims = (int)ndims;
  int native_return_value = MPI_Dims_create(native_nnodes, native_ndims, dims);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graph_create(plumed_mpiw_handle_storage comm_old, int32_t nnodes, const int32_t indx[], const int32_t edges[], int32_t reorder, plumed_mpiw_handle_storage * comm_graph) {
  if (!comm_graph) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm_old = plumed_mpiw_import_comm(comm_old);
  int native_nnodes = (int)nnodes;
  int native_reorder = (int)reorder;
  MPI_Comm native_comm_graph;
  memset(&native_comm_graph, 0, sizeof(native_comm_graph));
  if (!comm_graph) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Graph_create(native_comm_old, native_nnodes, indx, edges, native_reorder, &native_comm_graph);
  if (native_return_value == MPI_SUCCESS) {
    *comm_graph = plumed_mpiw_export_bytes(&native_comm_graph, sizeof(native_comm_graph));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Topo_test(plumed_mpiw_handle_storage comm, int32_t * status) {
  if (!status) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_status = 0;
  if (!status) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Topo_test(native_comm, &native_status);
  if (native_return_value == MPI_SUCCESS) {
    *status = (int32_t)native_status;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graphdims_get(plumed_mpiw_handle_storage comm, int32_t * nnodes, int32_t * nedges) {
  if (!nnodes) {
    return MPI_ERR_ARG;
  }
  if (!nedges) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_nnodes = 0;
  if (!nnodes) {
    return MPI_ERR_ARG;
  }
  int native_nedges = 0;
  if (!nedges) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Graphdims_get(native_comm, &native_nnodes, &native_nedges);
  if (native_return_value == MPI_SUCCESS) {
    *nnodes = (int32_t)native_nnodes;
  }
  if (native_return_value == MPI_SUCCESS) {
    *nedges = (int32_t)native_nedges;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graph_get(plumed_mpiw_handle_storage comm, int32_t maxindex, int32_t maxedges, int32_t indx[], int32_t edges[]) {
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_maxindex = (int)maxindex;
  int native_maxedges = (int)maxedges;
  int native_return_value = MPI_Graph_get(native_comm, native_maxindex, native_maxedges, indx, edges);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cartdim_get(plumed_mpiw_handle_storage comm, int32_t * ndims) {
  if (!ndims) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_ndims = 0;
  if (!ndims) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cartdim_get(native_comm, &native_ndims);
  if (native_return_value == MPI_SUCCESS) {
    *ndims = (int32_t)native_ndims;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_get(plumed_mpiw_handle_storage comm, int32_t maxdims, int32_t dims[], int32_t periods[], int32_t coords[]) {
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_maxdims = (int)maxdims;
  int native_return_value = MPI_Cart_get(native_comm, native_maxdims, dims, periods, coords);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_rank(plumed_mpiw_handle_storage comm, const int32_t coords[], int32_t * rank) {
  if (!rank) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_rank = 0;
  if (!rank) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cart_rank(native_comm, coords, &native_rank);
  if (native_return_value == MPI_SUCCESS) {
    *rank = (int32_t)native_rank;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_coords(plumed_mpiw_handle_storage comm, int32_t rank, int32_t maxdims, int32_t coords[]) {
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_rank = (int)rank;
  int native_maxdims = (int)maxdims;
  int native_return_value = MPI_Cart_coords(native_comm, native_rank, native_maxdims, coords);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graph_neighbors_count(plumed_mpiw_handle_storage comm, int32_t rank, int32_t * nneighbors) {
  if (!nneighbors) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_rank = (int)rank;
  int native_nneighbors = 0;
  if (!nneighbors) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Graph_neighbors_count(native_comm, native_rank, &native_nneighbors);
  if (native_return_value == MPI_SUCCESS) {
    *nneighbors = (int32_t)native_nneighbors;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graph_neighbors(plumed_mpiw_handle_storage comm, int32_t rank, int32_t maxneighbors, int32_t neighbors[]) {
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_rank = (int)rank;
  int native_maxneighbors = (int)maxneighbors;
  int native_return_value = MPI_Graph_neighbors(native_comm, native_rank, native_maxneighbors, neighbors);
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_shift(plumed_mpiw_handle_storage comm, int32_t direction, int32_t disp, int32_t * rank_source, int32_t * rank_dest) {
  if (!rank_source) {
    return MPI_ERR_ARG;
  }
  if (!rank_dest) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_direction = (int)direction;
  int native_disp = (int)disp;
  int native_rank_source = 0;
  if (!rank_source) {
    return MPI_ERR_ARG;
  }
  int native_rank_dest = 0;
  if (!rank_dest) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cart_shift(native_comm, native_direction, native_disp, &native_rank_source, &native_rank_dest);
  if (native_return_value == MPI_SUCCESS) {
    *rank_source = (int32_t)native_rank_source;
  }
  if (native_return_value == MPI_SUCCESS) {
    *rank_dest = (int32_t)native_rank_dest;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_sub(plumed_mpiw_handle_storage comm, const int32_t remain_dims[], plumed_mpiw_handle_storage * newcomm) {
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Comm native_newcomm;
  memset(&native_newcomm, 0, sizeof(native_newcomm));
  if (!newcomm) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cart_sub(native_comm, remain_dims, &native_newcomm);
  if (native_return_value == MPI_SUCCESS) {
    *newcomm = plumed_mpiw_export_bytes(&native_newcomm, sizeof(native_newcomm));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Cart_map(plumed_mpiw_handle_storage comm, int32_t ndims, const int32_t dims[], const int32_t periods[], int32_t * newrank) {
  if (!newrank) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_ndims = (int)ndims;
  int native_newrank = 0;
  if (!newrank) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Cart_map(native_comm, native_ndims, dims, periods, &native_newrank);
  if (native_return_value == MPI_SUCCESS) {
    *newrank = (int32_t)native_newrank;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Graph_map(plumed_mpiw_handle_storage comm, int32_t nnodes, const int32_t indx[], const int32_t edges[], int32_t * newrank) {
  if (!newrank) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  int native_nnodes = (int)nnodes;
  int native_newrank = 0;
  if (!newrank) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Graph_map(native_comm, native_nnodes, indx, edges, &native_newrank);
  if (native_return_value == MPI_SUCCESS) {
    *newrank = (int32_t)native_newrank;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Get_processor_name(char * name, int32_t * resultlen) {
  if (!resultlen) {
    return MPI_ERR_ARG;
  }
  int native_resultlen = 0;
  if (!resultlen) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Get_processor_name(name, &native_resultlen);
  if (native_return_value == MPI_SUCCESS) {
    *resultlen = (int32_t)native_resultlen;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Errhandler_free(plumed_mpiw_handle_storage * errhandler) {
  if (!errhandler) {
    return MPI_ERR_ARG;
  }
  MPI_Errhandler native_errhandler;
  memset(&native_errhandler, 0, sizeof(native_errhandler));
  if (!errhandler) {
    return MPI_ERR_ARG;
  }
  native_errhandler = plumed_mpiw_import_errhandler(*errhandler);
  int native_return_value = MPI_Errhandler_free(&native_errhandler);
  if (1) {
    *errhandler = plumed_mpiw_export_bytes(&native_errhandler, sizeof(native_errhandler));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Error_class(int32_t errorcode, int32_t * errorclass) {
  if (!errorclass) {
    return MPI_ERR_ARG;
  }
  int native_errorcode = (int)errorcode;
  int native_errorclass = 0;
  if (!errorclass) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Error_class(native_errorcode, &native_errorclass);
  if (native_return_value == MPI_SUCCESS) {
    *errorclass = (int32_t)native_errorclass;
  }
  return (int32_t)native_return_value;
}

static double plumed_mpiw_MPI_Wtime(void) {
  double native_return_value = MPI_Wtime();
  return native_return_value;
}

static double plumed_mpiw_MPI_Wtick(void) {
  double native_return_value = MPI_Wtick();
  return native_return_value;
}

static int32_t plumed_mpiw_MPI_Finalized(int32_t * flag) {
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_flag = 0;
  if (!flag) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Finalized(&native_flag);
  if (native_return_value == MPI_SUCCESS) {
    *flag = (int32_t)native_flag;
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_get_errhandler(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage * errhandler) {
  if (!errhandler) {
    return MPI_ERR_ARG;
  }
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Errhandler native_errhandler;
  memset(&native_errhandler, 0, sizeof(native_errhandler));
  if (!errhandler) {
    return MPI_ERR_ARG;
  }
  int native_return_value = MPI_Comm_get_errhandler(native_comm, &native_errhandler);
  if (native_return_value == MPI_SUCCESS) {
    *errhandler = plumed_mpiw_export_bytes(&native_errhandler, sizeof(native_errhandler));
  }
  return (int32_t)native_return_value;
}

static int32_t plumed_mpiw_MPI_Comm_set_errhandler(plumed_mpiw_handle_storage comm, plumed_mpiw_handle_storage errhandler) {
  MPI_Comm native_comm = plumed_mpiw_import_comm(comm);
  MPI_Errhandler native_errhandler = plumed_mpiw_import_errhandler(errhandler);
  int native_return_value = MPI_Comm_set_errhandler(native_comm, native_errhandler);
  return (int32_t)native_return_value;
}
static void plumed_mpiw_verify_typed_functions(void) {
  plumed_mpiw_MPI_Initialized_fn typed_MPI_Initialized = &plumed_mpiw_MPI_Initialized;
  (void)typed_MPI_Initialized;
  plumed_mpiw_MPI_Comm_dup_fn typed_MPI_Comm_dup = &plumed_mpiw_MPI_Comm_dup;
  (void)typed_MPI_Comm_dup;
  plumed_mpiw_MPI_Comm_free_fn typed_MPI_Comm_free = &plumed_mpiw_MPI_Comm_free;
  (void)typed_MPI_Comm_free;
  plumed_mpiw_MPI_Comm_rank_fn typed_MPI_Comm_rank = &plumed_mpiw_MPI_Comm_rank;
  (void)typed_MPI_Comm_rank;
  plumed_mpiw_MPI_Comm_size_fn typed_MPI_Comm_size = &plumed_mpiw_MPI_Comm_size;
  (void)typed_MPI_Comm_size;
  plumed_mpiw_MPI_Comm_split_fn typed_MPI_Comm_split = &plumed_mpiw_MPI_Comm_split;
  (void)typed_MPI_Comm_split;
  plumed_mpiw_MPI_Abort_fn typed_MPI_Abort = &plumed_mpiw_MPI_Abort;
  (void)typed_MPI_Abort;
  plumed_mpiw_MPI_Bcast_fn typed_MPI_Bcast = &plumed_mpiw_MPI_Bcast;
  (void)typed_MPI_Bcast;
  plumed_mpiw_MPI_Scatter_fn typed_MPI_Scatter = &plumed_mpiw_MPI_Scatter;
  (void)typed_MPI_Scatter;
  plumed_mpiw_MPI_Gather_fn typed_MPI_Gather = &plumed_mpiw_MPI_Gather;
  (void)typed_MPI_Gather;
  plumed_mpiw_MPI_Allreduce_fn typed_MPI_Allreduce = &plumed_mpiw_MPI_Allreduce;
  (void)typed_MPI_Allreduce;
  plumed_mpiw_MPI_Isend_fn typed_MPI_Isend = &plumed_mpiw_MPI_Isend;
  (void)typed_MPI_Isend;
  plumed_mpiw_MPI_Wait_fn typed_MPI_Wait = &plumed_mpiw_MPI_Wait;
  (void)typed_MPI_Wait;
  plumed_mpiw_MPI_Allgather_fn typed_MPI_Allgather = &plumed_mpiw_MPI_Allgather;
  (void)typed_MPI_Allgather;
  plumed_mpiw_MPI_Allgatherv_fn typed_MPI_Allgatherv = &plumed_mpiw_MPI_Allgatherv;
  (void)typed_MPI_Allgatherv;
  plumed_mpiw_MPI_Recv_fn typed_MPI_Recv = &plumed_mpiw_MPI_Recv;
  (void)typed_MPI_Recv;
  plumed_mpiw_MPI_Barrier_fn typed_MPI_Barrier = &plumed_mpiw_MPI_Barrier;
  (void)typed_MPI_Barrier;
  plumed_mpiw_MPI_Get_count_fn typed_MPI_Get_count = &plumed_mpiw_MPI_Get_count;
  (void)typed_MPI_Get_count;
  plumed_mpiw_MPI_Sendrecv_fn typed_MPI_Sendrecv = &plumed_mpiw_MPI_Sendrecv;
  (void)typed_MPI_Sendrecv;
  plumed_mpiw_MPI_Error_string_fn typed_MPI_Error_string = &plumed_mpiw_MPI_Error_string;
  (void)typed_MPI_Error_string;
  plumed_mpiw_MPI_Comm_f2c_fn typed_MPI_Comm_f2c = &plumed_mpiw_MPI_Comm_f2c;
  (void)typed_MPI_Comm_f2c;
  plumed_mpiw_MPI_Send_fn typed_MPI_Send = &plumed_mpiw_MPI_Send;
  (void)typed_MPI_Send;
  plumed_mpiw_MPI_Bsend_fn typed_MPI_Bsend = &plumed_mpiw_MPI_Bsend;
  (void)typed_MPI_Bsend;
  plumed_mpiw_MPI_Ssend_fn typed_MPI_Ssend = &plumed_mpiw_MPI_Ssend;
  (void)typed_MPI_Ssend;
  plumed_mpiw_MPI_Rsend_fn typed_MPI_Rsend = &plumed_mpiw_MPI_Rsend;
  (void)typed_MPI_Rsend;
  plumed_mpiw_MPI_Buffer_attach_fn typed_MPI_Buffer_attach = &plumed_mpiw_MPI_Buffer_attach;
  (void)typed_MPI_Buffer_attach;
  plumed_mpiw_MPI_Buffer_detach_fn typed_MPI_Buffer_detach = &plumed_mpiw_MPI_Buffer_detach;
  (void)typed_MPI_Buffer_detach;
  plumed_mpiw_MPI_Ibsend_fn typed_MPI_Ibsend = &plumed_mpiw_MPI_Ibsend;
  (void)typed_MPI_Ibsend;
  plumed_mpiw_MPI_Issend_fn typed_MPI_Issend = &plumed_mpiw_MPI_Issend;
  (void)typed_MPI_Issend;
  plumed_mpiw_MPI_Irsend_fn typed_MPI_Irsend = &plumed_mpiw_MPI_Irsend;
  (void)typed_MPI_Irsend;
  plumed_mpiw_MPI_Irecv_fn typed_MPI_Irecv = &plumed_mpiw_MPI_Irecv;
  (void)typed_MPI_Irecv;
  plumed_mpiw_MPI_Test_fn typed_MPI_Test = &plumed_mpiw_MPI_Test;
  (void)typed_MPI_Test;
  plumed_mpiw_MPI_Request_free_fn typed_MPI_Request_free = &plumed_mpiw_MPI_Request_free;
  (void)typed_MPI_Request_free;
  plumed_mpiw_MPI_Waitany_fn typed_MPI_Waitany = &plumed_mpiw_MPI_Waitany;
  (void)typed_MPI_Waitany;
  plumed_mpiw_MPI_Testany_fn typed_MPI_Testany = &plumed_mpiw_MPI_Testany;
  (void)typed_MPI_Testany;
  plumed_mpiw_MPI_Waitall_fn typed_MPI_Waitall = &plumed_mpiw_MPI_Waitall;
  (void)typed_MPI_Waitall;
  plumed_mpiw_MPI_Testall_fn typed_MPI_Testall = &plumed_mpiw_MPI_Testall;
  (void)typed_MPI_Testall;
  plumed_mpiw_MPI_Waitsome_fn typed_MPI_Waitsome = &plumed_mpiw_MPI_Waitsome;
  (void)typed_MPI_Waitsome;
  plumed_mpiw_MPI_Testsome_fn typed_MPI_Testsome = &plumed_mpiw_MPI_Testsome;
  (void)typed_MPI_Testsome;
  plumed_mpiw_MPI_Iprobe_fn typed_MPI_Iprobe = &plumed_mpiw_MPI_Iprobe;
  (void)typed_MPI_Iprobe;
  plumed_mpiw_MPI_Probe_fn typed_MPI_Probe = &plumed_mpiw_MPI_Probe;
  (void)typed_MPI_Probe;
  plumed_mpiw_MPI_Cancel_fn typed_MPI_Cancel = &plumed_mpiw_MPI_Cancel;
  (void)typed_MPI_Cancel;
  plumed_mpiw_MPI_Test_cancelled_fn typed_MPI_Test_cancelled = &plumed_mpiw_MPI_Test_cancelled;
  (void)typed_MPI_Test_cancelled;
  plumed_mpiw_MPI_Send_init_fn typed_MPI_Send_init = &plumed_mpiw_MPI_Send_init;
  (void)typed_MPI_Send_init;
  plumed_mpiw_MPI_Bsend_init_fn typed_MPI_Bsend_init = &plumed_mpiw_MPI_Bsend_init;
  (void)typed_MPI_Bsend_init;
  plumed_mpiw_MPI_Ssend_init_fn typed_MPI_Ssend_init = &plumed_mpiw_MPI_Ssend_init;
  (void)typed_MPI_Ssend_init;
  plumed_mpiw_MPI_Rsend_init_fn typed_MPI_Rsend_init = &plumed_mpiw_MPI_Rsend_init;
  (void)typed_MPI_Rsend_init;
  plumed_mpiw_MPI_Recv_init_fn typed_MPI_Recv_init = &plumed_mpiw_MPI_Recv_init;
  (void)typed_MPI_Recv_init;
  plumed_mpiw_MPI_Start_fn typed_MPI_Start = &plumed_mpiw_MPI_Start;
  (void)typed_MPI_Start;
  plumed_mpiw_MPI_Startall_fn typed_MPI_Startall = &plumed_mpiw_MPI_Startall;
  (void)typed_MPI_Startall;
  plumed_mpiw_MPI_Sendrecv_replace_fn typed_MPI_Sendrecv_replace = &plumed_mpiw_MPI_Sendrecv_replace;
  (void)typed_MPI_Sendrecv_replace;
  plumed_mpiw_MPI_Type_contiguous_fn typed_MPI_Type_contiguous = &plumed_mpiw_MPI_Type_contiguous;
  (void)typed_MPI_Type_contiguous;
  plumed_mpiw_MPI_Type_vector_fn typed_MPI_Type_vector = &plumed_mpiw_MPI_Type_vector;
  (void)typed_MPI_Type_vector;
  plumed_mpiw_MPI_Type_indexed_fn typed_MPI_Type_indexed = &plumed_mpiw_MPI_Type_indexed;
  (void)typed_MPI_Type_indexed;
  plumed_mpiw_MPI_Type_commit_fn typed_MPI_Type_commit = &plumed_mpiw_MPI_Type_commit;
  (void)typed_MPI_Type_commit;
  plumed_mpiw_MPI_Type_free_fn typed_MPI_Type_free = &plumed_mpiw_MPI_Type_free;
  (void)typed_MPI_Type_free;
  plumed_mpiw_MPI_Type_size_fn typed_MPI_Type_size = &plumed_mpiw_MPI_Type_size;
  (void)typed_MPI_Type_size;
  plumed_mpiw_MPI_Get_elements_fn typed_MPI_Get_elements = &plumed_mpiw_MPI_Get_elements;
  (void)typed_MPI_Get_elements;
  plumed_mpiw_MPI_Pack_fn typed_MPI_Pack = &plumed_mpiw_MPI_Pack;
  (void)typed_MPI_Pack;
  plumed_mpiw_MPI_Unpack_fn typed_MPI_Unpack = &plumed_mpiw_MPI_Unpack;
  (void)typed_MPI_Unpack;
  plumed_mpiw_MPI_Pack_size_fn typed_MPI_Pack_size = &plumed_mpiw_MPI_Pack_size;
  (void)typed_MPI_Pack_size;
  plumed_mpiw_MPI_Gatherv_fn typed_MPI_Gatherv = &plumed_mpiw_MPI_Gatherv;
  (void)typed_MPI_Gatherv;
  plumed_mpiw_MPI_Scatterv_fn typed_MPI_Scatterv = &plumed_mpiw_MPI_Scatterv;
  (void)typed_MPI_Scatterv;
  plumed_mpiw_MPI_Alltoall_fn typed_MPI_Alltoall = &plumed_mpiw_MPI_Alltoall;
  (void)typed_MPI_Alltoall;
  plumed_mpiw_MPI_Alltoallv_fn typed_MPI_Alltoallv = &plumed_mpiw_MPI_Alltoallv;
  (void)typed_MPI_Alltoallv;
  plumed_mpiw_MPI_Reduce_fn typed_MPI_Reduce = &plumed_mpiw_MPI_Reduce;
  (void)typed_MPI_Reduce;
  plumed_mpiw_MPI_Op_free_fn typed_MPI_Op_free = &plumed_mpiw_MPI_Op_free;
  (void)typed_MPI_Op_free;
  plumed_mpiw_MPI_Reduce_scatter_fn typed_MPI_Reduce_scatter = &plumed_mpiw_MPI_Reduce_scatter;
  (void)typed_MPI_Reduce_scatter;
  plumed_mpiw_MPI_Scan_fn typed_MPI_Scan = &plumed_mpiw_MPI_Scan;
  (void)typed_MPI_Scan;
  plumed_mpiw_MPI_Group_size_fn typed_MPI_Group_size = &plumed_mpiw_MPI_Group_size;
  (void)typed_MPI_Group_size;
  plumed_mpiw_MPI_Group_rank_fn typed_MPI_Group_rank = &plumed_mpiw_MPI_Group_rank;
  (void)typed_MPI_Group_rank;
  plumed_mpiw_MPI_Group_translate_ranks_fn typed_MPI_Group_translate_ranks = &plumed_mpiw_MPI_Group_translate_ranks;
  (void)typed_MPI_Group_translate_ranks;
  plumed_mpiw_MPI_Group_compare_fn typed_MPI_Group_compare = &plumed_mpiw_MPI_Group_compare;
  (void)typed_MPI_Group_compare;
  plumed_mpiw_MPI_Comm_group_fn typed_MPI_Comm_group = &plumed_mpiw_MPI_Comm_group;
  (void)typed_MPI_Comm_group;
  plumed_mpiw_MPI_Group_union_fn typed_MPI_Group_union = &plumed_mpiw_MPI_Group_union;
  (void)typed_MPI_Group_union;
  plumed_mpiw_MPI_Group_intersection_fn typed_MPI_Group_intersection = &plumed_mpiw_MPI_Group_intersection;
  (void)typed_MPI_Group_intersection;
  plumed_mpiw_MPI_Group_difference_fn typed_MPI_Group_difference = &plumed_mpiw_MPI_Group_difference;
  (void)typed_MPI_Group_difference;
  plumed_mpiw_MPI_Group_incl_fn typed_MPI_Group_incl = &plumed_mpiw_MPI_Group_incl;
  (void)typed_MPI_Group_incl;
  plumed_mpiw_MPI_Group_excl_fn typed_MPI_Group_excl = &plumed_mpiw_MPI_Group_excl;
  (void)typed_MPI_Group_excl;
  plumed_mpiw_MPI_Group_range_incl_fn typed_MPI_Group_range_incl = &plumed_mpiw_MPI_Group_range_incl;
  (void)typed_MPI_Group_range_incl;
  plumed_mpiw_MPI_Group_range_excl_fn typed_MPI_Group_range_excl = &plumed_mpiw_MPI_Group_range_excl;
  (void)typed_MPI_Group_range_excl;
  plumed_mpiw_MPI_Group_free_fn typed_MPI_Group_free = &plumed_mpiw_MPI_Group_free;
  (void)typed_MPI_Group_free;
  plumed_mpiw_MPI_Comm_compare_fn typed_MPI_Comm_compare = &plumed_mpiw_MPI_Comm_compare;
  (void)typed_MPI_Comm_compare;
  plumed_mpiw_MPI_Comm_create_fn typed_MPI_Comm_create = &plumed_mpiw_MPI_Comm_create;
  (void)typed_MPI_Comm_create;
  plumed_mpiw_MPI_Comm_test_inter_fn typed_MPI_Comm_test_inter = &plumed_mpiw_MPI_Comm_test_inter;
  (void)typed_MPI_Comm_test_inter;
  plumed_mpiw_MPI_Comm_remote_size_fn typed_MPI_Comm_remote_size = &plumed_mpiw_MPI_Comm_remote_size;
  (void)typed_MPI_Comm_remote_size;
  plumed_mpiw_MPI_Comm_remote_group_fn typed_MPI_Comm_remote_group = &plumed_mpiw_MPI_Comm_remote_group;
  (void)typed_MPI_Comm_remote_group;
  plumed_mpiw_MPI_Intercomm_create_fn typed_MPI_Intercomm_create = &plumed_mpiw_MPI_Intercomm_create;
  (void)typed_MPI_Intercomm_create;
  plumed_mpiw_MPI_Intercomm_merge_fn typed_MPI_Intercomm_merge = &plumed_mpiw_MPI_Intercomm_merge;
  (void)typed_MPI_Intercomm_merge;
  plumed_mpiw_MPI_Cart_create_fn typed_MPI_Cart_create = &plumed_mpiw_MPI_Cart_create;
  (void)typed_MPI_Cart_create;
  plumed_mpiw_MPI_Dims_create_fn typed_MPI_Dims_create = &plumed_mpiw_MPI_Dims_create;
  (void)typed_MPI_Dims_create;
  plumed_mpiw_MPI_Graph_create_fn typed_MPI_Graph_create = &plumed_mpiw_MPI_Graph_create;
  (void)typed_MPI_Graph_create;
  plumed_mpiw_MPI_Topo_test_fn typed_MPI_Topo_test = &plumed_mpiw_MPI_Topo_test;
  (void)typed_MPI_Topo_test;
  plumed_mpiw_MPI_Graphdims_get_fn typed_MPI_Graphdims_get = &plumed_mpiw_MPI_Graphdims_get;
  (void)typed_MPI_Graphdims_get;
  plumed_mpiw_MPI_Graph_get_fn typed_MPI_Graph_get = &plumed_mpiw_MPI_Graph_get;
  (void)typed_MPI_Graph_get;
  plumed_mpiw_MPI_Cartdim_get_fn typed_MPI_Cartdim_get = &plumed_mpiw_MPI_Cartdim_get;
  (void)typed_MPI_Cartdim_get;
  plumed_mpiw_MPI_Cart_get_fn typed_MPI_Cart_get = &plumed_mpiw_MPI_Cart_get;
  (void)typed_MPI_Cart_get;
  plumed_mpiw_MPI_Cart_rank_fn typed_MPI_Cart_rank = &plumed_mpiw_MPI_Cart_rank;
  (void)typed_MPI_Cart_rank;
  plumed_mpiw_MPI_Cart_coords_fn typed_MPI_Cart_coords = &plumed_mpiw_MPI_Cart_coords;
  (void)typed_MPI_Cart_coords;
  plumed_mpiw_MPI_Graph_neighbors_count_fn typed_MPI_Graph_neighbors_count = &plumed_mpiw_MPI_Graph_neighbors_count;
  (void)typed_MPI_Graph_neighbors_count;
  plumed_mpiw_MPI_Graph_neighbors_fn typed_MPI_Graph_neighbors = &plumed_mpiw_MPI_Graph_neighbors;
  (void)typed_MPI_Graph_neighbors;
  plumed_mpiw_MPI_Cart_shift_fn typed_MPI_Cart_shift = &plumed_mpiw_MPI_Cart_shift;
  (void)typed_MPI_Cart_shift;
  plumed_mpiw_MPI_Cart_sub_fn typed_MPI_Cart_sub = &plumed_mpiw_MPI_Cart_sub;
  (void)typed_MPI_Cart_sub;
  plumed_mpiw_MPI_Cart_map_fn typed_MPI_Cart_map = &plumed_mpiw_MPI_Cart_map;
  (void)typed_MPI_Cart_map;
  plumed_mpiw_MPI_Graph_map_fn typed_MPI_Graph_map = &plumed_mpiw_MPI_Graph_map;
  (void)typed_MPI_Graph_map;
  plumed_mpiw_MPI_Get_processor_name_fn typed_MPI_Get_processor_name = &plumed_mpiw_MPI_Get_processor_name;
  (void)typed_MPI_Get_processor_name;
  plumed_mpiw_MPI_Errhandler_free_fn typed_MPI_Errhandler_free = &plumed_mpiw_MPI_Errhandler_free;
  (void)typed_MPI_Errhandler_free;
  plumed_mpiw_MPI_Error_class_fn typed_MPI_Error_class = &plumed_mpiw_MPI_Error_class;
  (void)typed_MPI_Error_class;
  plumed_mpiw_MPI_Wtime_fn typed_MPI_Wtime = &plumed_mpiw_MPI_Wtime;
  (void)typed_MPI_Wtime;
  plumed_mpiw_MPI_Wtick_fn typed_MPI_Wtick = &plumed_mpiw_MPI_Wtick;
  (void)typed_MPI_Wtick;
  plumed_mpiw_MPI_Finalized_fn typed_MPI_Finalized = &plumed_mpiw_MPI_Finalized;
  (void)typed_MPI_Finalized;
  plumed_mpiw_MPI_Comm_get_errhandler_fn typed_MPI_Comm_get_errhandler = &plumed_mpiw_MPI_Comm_get_errhandler;
  (void)typed_MPI_Comm_get_errhandler;
  plumed_mpiw_MPI_Comm_set_errhandler_fn typed_MPI_Comm_set_errhandler = &plumed_mpiw_MPI_Comm_set_errhandler;
  (void)typed_MPI_Comm_set_errhandler;
}
#define PLUMED_MPIW_ENTRY(name, id, version, fn) { (uint32_t)sizeof(plumed_mpiw_function_entry), PLUMED_MPIW_FUNCTION_REQUIRED, #name, id, version, (plumed_mpiw_generic_fn)(fn) }
static const plumed_mpiw_function_entry plumed_mpiw_functions[] = {
  PLUMED_MPIW_ENTRY(MPI_Initialized, 0xd4498369u, 1u, plumed_mpiw_MPI_Initialized),
  PLUMED_MPIW_ENTRY(MPI_Comm_dup, 0xf0dc6608u, 1u, plumed_mpiw_MPI_Comm_dup),
  PLUMED_MPIW_ENTRY(MPI_Comm_free, 0x4da4e1b2u, 1u, plumed_mpiw_MPI_Comm_free),
  PLUMED_MPIW_ENTRY(MPI_Comm_rank, 0x4e7672aau, 1u, plumed_mpiw_MPI_Comm_rank),
  PLUMED_MPIW_ENTRY(MPI_Comm_size, 0xda2369ecu, 1u, plumed_mpiw_MPI_Comm_size),
  PLUMED_MPIW_ENTRY(MPI_Comm_split, 0x97f1adf0u, 1u, plumed_mpiw_MPI_Comm_split),
  PLUMED_MPIW_ENTRY(MPI_Abort, 0x84f458d6u, 1u, plumed_mpiw_MPI_Abort),
  PLUMED_MPIW_ENTRY(MPI_Bcast, 0xe809e1cau, 1u, plumed_mpiw_MPI_Bcast),
  PLUMED_MPIW_ENTRY(MPI_Scatter, 0xb2fa0f58u, 1u, plumed_mpiw_MPI_Scatter),
  PLUMED_MPIW_ENTRY(MPI_Gather, 0xb1f903d9u, 1u, plumed_mpiw_MPI_Gather),
  PLUMED_MPIW_ENTRY(MPI_Allreduce, 0x788900dbu, 1u, plumed_mpiw_MPI_Allreduce),
  PLUMED_MPIW_ENTRY(MPI_Isend, 0xf5e9d915u, 1u, plumed_mpiw_MPI_Isend),
  PLUMED_MPIW_ENTRY(MPI_Wait, 0x4b15813du, 1u, plumed_mpiw_MPI_Wait),
  PLUMED_MPIW_ENTRY(MPI_Allgather, 0xab8b26c9u, 1u, plumed_mpiw_MPI_Allgather),
  PLUMED_MPIW_ENTRY(MPI_Allgatherv, 0x0f1b22f3u, 1u, plumed_mpiw_MPI_Allgatherv),
  PLUMED_MPIW_ENTRY(MPI_Recv, 0xdd56a4bcu, 1u, plumed_mpiw_MPI_Recv),
  PLUMED_MPIW_ENTRY(MPI_Barrier, 0xe7a052f4u, 1u, plumed_mpiw_MPI_Barrier),
  PLUMED_MPIW_ENTRY(MPI_Get_count, 0x71967a66u, 1u, plumed_mpiw_MPI_Get_count),
  PLUMED_MPIW_ENTRY(MPI_Sendrecv, 0xb09a6fc1u, 1u, plumed_mpiw_MPI_Sendrecv),
  PLUMED_MPIW_ENTRY(MPI_Error_string, 0xd272abecu, 1u, plumed_mpiw_MPI_Error_string),
  PLUMED_MPIW_ENTRY(MPI_Comm_f2c, 0x9a9e7fcdu, 1u, plumed_mpiw_MPI_Comm_f2c),
  PLUMED_MPIW_ENTRY(MPI_Send, 0xbd2b6912u, 1u, plumed_mpiw_MPI_Send),
  PLUMED_MPIW_ENTRY(MPI_Bsend, 0xa9dbf685u, 1u, plumed_mpiw_MPI_Bsend),
  PLUMED_MPIW_ENTRY(MPI_Ssend, 0x5edcf0deu, 1u, plumed_mpiw_MPI_Ssend),
  PLUMED_MPIW_ENTRY(MPI_Rsend, 0xbfa09b06u, 1u, plumed_mpiw_MPI_Rsend),
  PLUMED_MPIW_ENTRY(MPI_Buffer_attach, 0xe00bd480u, 1u, plumed_mpiw_MPI_Buffer_attach),
  PLUMED_MPIW_ENTRY(MPI_Buffer_detach, 0x74dec40fu, 1u, plumed_mpiw_MPI_Buffer_detach),
  PLUMED_MPIW_ENTRY(MPI_Ibsend, 0xa4d683afu, 1u, plumed_mpiw_MPI_Ibsend),
  PLUMED_MPIW_ENTRY(MPI_Issend, 0x8275e866u, 1u, plumed_mpiw_MPI_Issend),
  PLUMED_MPIW_ENTRY(MPI_Irsend, 0xb13d1d70u, 1u, plumed_mpiw_MPI_Irsend),
  PLUMED_MPIW_ENTRY(MPI_Irecv, 0xb69a0083u, 1u, plumed_mpiw_MPI_Irecv),
  PLUMED_MPIW_ENTRY(MPI_Test, 0x1d207e4cu, 1u, plumed_mpiw_MPI_Test),
  PLUMED_MPIW_ENTRY(MPI_Request_free, 0x475f38a1u, 1u, plumed_mpiw_MPI_Request_free),
  PLUMED_MPIW_ENTRY(MPI_Waitany, 0x5acd4436u, 1u, plumed_mpiw_MPI_Waitany),
  PLUMED_MPIW_ENTRY(MPI_Testany, 0xf329c686u, 1u, plumed_mpiw_MPI_Testany),
  PLUMED_MPIW_ENTRY(MPI_Waitall, 0x7a57ae23u, 1u, plumed_mpiw_MPI_Waitall),
  PLUMED_MPIW_ENTRY(MPI_Testall, 0x593a1626u, 1u, plumed_mpiw_MPI_Testall),
  PLUMED_MPIW_ENTRY(MPI_Waitsome, 0xb6562e1au, 1u, plumed_mpiw_MPI_Waitsome),
  PLUMED_MPIW_ENTRY(MPI_Testsome, 0x5a87d17bu, 1u, plumed_mpiw_MPI_Testsome),
  PLUMED_MPIW_ENTRY(MPI_Iprobe, 0xd1ba8db8u, 1u, plumed_mpiw_MPI_Iprobe),
  PLUMED_MPIW_ENTRY(MPI_Probe, 0xfed81500u, 1u, plumed_mpiw_MPI_Probe),
  PLUMED_MPIW_ENTRY(MPI_Cancel, 0xed47f9abu, 1u, plumed_mpiw_MPI_Cancel),
  PLUMED_MPIW_ENTRY(MPI_Test_cancelled, 0xec5e3092u, 1u, plumed_mpiw_MPI_Test_cancelled),
  PLUMED_MPIW_ENTRY(MPI_Send_init, 0xd381e137u, 1u, plumed_mpiw_MPI_Send_init),
  PLUMED_MPIW_ENTRY(MPI_Bsend_init, 0x16ede22eu, 1u, plumed_mpiw_MPI_Bsend_init),
  PLUMED_MPIW_ENTRY(MPI_Ssend_init, 0xeea1c733u, 1u, plumed_mpiw_MPI_Ssend_init),
  PLUMED_MPIW_ENTRY(MPI_Rsend_init, 0x018fab06u, 1u, plumed_mpiw_MPI_Rsend_init),
  PLUMED_MPIW_ENTRY(MPI_Recv_init, 0x9cbb4e8au, 1u, plumed_mpiw_MPI_Recv_init),
  PLUMED_MPIW_ENTRY(MPI_Start, 0xfb62fbe3u, 1u, plumed_mpiw_MPI_Start),
  PLUMED_MPIW_ENTRY(MPI_Startall, 0xc3d7cddfu, 1u, plumed_mpiw_MPI_Startall),
  PLUMED_MPIW_ENTRY(MPI_Sendrecv_replace, 0x7c6ffc57u, 1u, plumed_mpiw_MPI_Sendrecv_replace),
  PLUMED_MPIW_ENTRY(MPI_Type_contiguous, 0xe41681d2u, 1u, plumed_mpiw_MPI_Type_contiguous),
  PLUMED_MPIW_ENTRY(MPI_Type_vector, 0xc7b9dbb0u, 1u, plumed_mpiw_MPI_Type_vector),
  PLUMED_MPIW_ENTRY(MPI_Type_indexed, 0xeb606d34u, 1u, plumed_mpiw_MPI_Type_indexed),
  PLUMED_MPIW_ENTRY(MPI_Type_commit, 0xf848da1fu, 1u, plumed_mpiw_MPI_Type_commit),
  PLUMED_MPIW_ENTRY(MPI_Type_free, 0xdacde819u, 1u, plumed_mpiw_MPI_Type_free),
  PLUMED_MPIW_ENTRY(MPI_Type_size, 0x95660c77u, 1u, plumed_mpiw_MPI_Type_size),
  PLUMED_MPIW_ENTRY(MPI_Get_elements, 0xf71eaa9au, 1u, plumed_mpiw_MPI_Get_elements),
  PLUMED_MPIW_ENTRY(MPI_Pack, 0x5649ba7au, 1u, plumed_mpiw_MPI_Pack),
  PLUMED_MPIW_ENTRY(MPI_Unpack, 0x49901f8bu, 1u, plumed_mpiw_MPI_Unpack),
  PLUMED_MPIW_ENTRY(MPI_Pack_size, 0x24de0131u, 1u, plumed_mpiw_MPI_Pack_size),
  PLUMED_MPIW_ENTRY(MPI_Gatherv, 0xc90455c9u, 1u, plumed_mpiw_MPI_Gatherv),
  PLUMED_MPIW_ENTRY(MPI_Scatterv, 0x765eb673u, 1u, plumed_mpiw_MPI_Scatterv),
  PLUMED_MPIW_ENTRY(MPI_Alltoall, 0xcfe270aeu, 1u, plumed_mpiw_MPI_Alltoall),
  PLUMED_MPIW_ENTRY(MPI_Alltoallv, 0xa4d99ed0u, 1u, plumed_mpiw_MPI_Alltoallv),
  PLUMED_MPIW_ENTRY(MPI_Reduce, 0x3a815604u, 1u, plumed_mpiw_MPI_Reduce),
  PLUMED_MPIW_ENTRY(MPI_Op_free, 0x94fea4e6u, 1u, plumed_mpiw_MPI_Op_free),
  PLUMED_MPIW_ENTRY(MPI_Reduce_scatter, 0x2f70a384u, 1u, plumed_mpiw_MPI_Reduce_scatter),
  PLUMED_MPIW_ENTRY(MPI_Scan, 0x64e2ee54u, 1u, plumed_mpiw_MPI_Scan),
  PLUMED_MPIW_ENTRY(MPI_Group_size, 0x7de536d5u, 1u, plumed_mpiw_MPI_Group_size),
  PLUMED_MPIW_ENTRY(MPI_Group_rank, 0x668fab96u, 1u, plumed_mpiw_MPI_Group_rank),
  PLUMED_MPIW_ENTRY(MPI_Group_translate_ranks, 0x2ac4d2c6u, 1u, plumed_mpiw_MPI_Group_translate_ranks),
  PLUMED_MPIW_ENTRY(MPI_Group_compare, 0xb0d34b44u, 1u, plumed_mpiw_MPI_Group_compare),
  PLUMED_MPIW_ENTRY(MPI_Comm_group, 0xd11f4483u, 1u, plumed_mpiw_MPI_Comm_group),
  PLUMED_MPIW_ENTRY(MPI_Group_union, 0xc288bd65u, 1u, plumed_mpiw_MPI_Group_union),
  PLUMED_MPIW_ENTRY(MPI_Group_intersection, 0xddf0ecb4u, 1u, plumed_mpiw_MPI_Group_intersection),
  PLUMED_MPIW_ENTRY(MPI_Group_difference, 0x37101ec1u, 1u, plumed_mpiw_MPI_Group_difference),
  PLUMED_MPIW_ENTRY(MPI_Group_incl, 0xbe8bfe71u, 1u, plumed_mpiw_MPI_Group_incl),
  PLUMED_MPIW_ENTRY(MPI_Group_excl, 0x45fa0902u, 1u, plumed_mpiw_MPI_Group_excl),
  PLUMED_MPIW_ENTRY(MPI_Group_range_incl, 0x801adc54u, 1u, plumed_mpiw_MPI_Group_range_incl),
  PLUMED_MPIW_ENTRY(MPI_Group_range_excl, 0xc416deb6u, 1u, plumed_mpiw_MPI_Group_range_excl),
  PLUMED_MPIW_ENTRY(MPI_Group_free, 0xfd1c62d1u, 1u, plumed_mpiw_MPI_Group_free),
  PLUMED_MPIW_ENTRY(MPI_Comm_compare, 0xdacd79cdu, 1u, plumed_mpiw_MPI_Comm_compare),
  PLUMED_MPIW_ENTRY(MPI_Comm_create, 0x1de85fa0u, 1u, plumed_mpiw_MPI_Comm_create),
  PLUMED_MPIW_ENTRY(MPI_Comm_test_inter, 0x29431089u, 1u, plumed_mpiw_MPI_Comm_test_inter),
  PLUMED_MPIW_ENTRY(MPI_Comm_remote_size, 0xaa0c6ac3u, 1u, plumed_mpiw_MPI_Comm_remote_size),
  PLUMED_MPIW_ENTRY(MPI_Comm_remote_group, 0x49ee9d8fu, 1u, plumed_mpiw_MPI_Comm_remote_group),
  PLUMED_MPIW_ENTRY(MPI_Intercomm_create, 0x320542c0u, 1u, plumed_mpiw_MPI_Intercomm_create),
  PLUMED_MPIW_ENTRY(MPI_Intercomm_merge, 0x1f6a920au, 1u, plumed_mpiw_MPI_Intercomm_merge),
  PLUMED_MPIW_ENTRY(MPI_Cart_create, 0xfd363844u, 1u, plumed_mpiw_MPI_Cart_create),
  PLUMED_MPIW_ENTRY(MPI_Dims_create, 0xfb2dd1cdu, 1u, plumed_mpiw_MPI_Dims_create),
  PLUMED_MPIW_ENTRY(MPI_Graph_create, 0x00eb0f02u, 1u, plumed_mpiw_MPI_Graph_create),
  PLUMED_MPIW_ENTRY(MPI_Topo_test, 0x09cbe810u, 1u, plumed_mpiw_MPI_Topo_test),
  PLUMED_MPIW_ENTRY(MPI_Graphdims_get, 0xc56e163bu, 1u, plumed_mpiw_MPI_Graphdims_get),
  PLUMED_MPIW_ENTRY(MPI_Graph_get, 0x12829ea5u, 1u, plumed_mpiw_MPI_Graph_get),
  PLUMED_MPIW_ENTRY(MPI_Cartdim_get, 0xa979e88cu, 1u, plumed_mpiw_MPI_Cartdim_get),
  PLUMED_MPIW_ENTRY(MPI_Cart_get, 0x0a3d9750u, 1u, plumed_mpiw_MPI_Cart_get),
  PLUMED_MPIW_ENTRY(MPI_Cart_rank, 0xf899fe26u, 1u, plumed_mpiw_MPI_Cart_rank),
  PLUMED_MPIW_ENTRY(MPI_Cart_coords, 0xf0bdc30cu, 1u, plumed_mpiw_MPI_Cart_coords),
  PLUMED_MPIW_ENTRY(MPI_Graph_neighbors_count, 0x5a79d6b6u, 1u, plumed_mpiw_MPI_Graph_neighbors_count),
  PLUMED_MPIW_ENTRY(MPI_Graph_neighbors, 0x567d4ff6u, 1u, plumed_mpiw_MPI_Graph_neighbors),
  PLUMED_MPIW_ENTRY(MPI_Cart_shift, 0x4d227e7cu, 1u, plumed_mpiw_MPI_Cart_shift),
  PLUMED_MPIW_ENTRY(MPI_Cart_sub, 0x1e9d3861u, 1u, plumed_mpiw_MPI_Cart_sub),
  PLUMED_MPIW_ENTRY(MPI_Cart_map, 0xba5a7b4cu, 1u, plumed_mpiw_MPI_Cart_map),
  PLUMED_MPIW_ENTRY(MPI_Graph_map, 0xefd795ebu, 1u, plumed_mpiw_MPI_Graph_map),
  PLUMED_MPIW_ENTRY(MPI_Get_processor_name, 0xf8e4ccc3u, 1u, plumed_mpiw_MPI_Get_processor_name),
  PLUMED_MPIW_ENTRY(MPI_Errhandler_free, 0xf03f8309u, 1u, plumed_mpiw_MPI_Errhandler_free),
  PLUMED_MPIW_ENTRY(MPI_Error_class, 0x3b9188a6u, 1u, plumed_mpiw_MPI_Error_class),
  PLUMED_MPIW_ENTRY(MPI_Wtime, 0x75704e2eu, 1u, plumed_mpiw_MPI_Wtime),
  PLUMED_MPIW_ENTRY(MPI_Wtick, 0x84f54b80u, 1u, plumed_mpiw_MPI_Wtick),
  PLUMED_MPIW_ENTRY(MPI_Finalized, 0x105b65c7u, 1u, plumed_mpiw_MPI_Finalized),
  PLUMED_MPIW_ENTRY(MPI_Comm_get_errhandler, 0x74d05159u, 1u, plumed_mpiw_MPI_Comm_get_errhandler),
  PLUMED_MPIW_ENTRY(MPI_Comm_set_errhandler, 0xe6af3a8fu, 1u, plumed_mpiw_MPI_Comm_set_errhandler)
};
#undef PLUMED_MPIW_ENTRY

static int32_t plumed_mpiw_store_constant(void *output, uint32_t capacity, const void *value, size_t size) {
  if (!output || capacity < size) {
    return (int32_t)MPI_ERR_ARG;
  }
  memset(output, 0, capacity);
  memcpy(output, value, size);
  return (int32_t)MPI_SUCCESS;
}
#define PLUMED_MPIW_HANDLE_CONSTANT(name, type) \
  static int32_t plumed_mpiw_value_##name(void *out, uint32_t cap) { type value = name; return plumed_mpiw_store_constant(out, cap, &value, sizeof(value)); }
#define PLUMED_MPIW_INTEGER_CONSTANT(name) \
  static int32_t plumed_mpiw_value_##name(void *out, uint32_t cap) { int32_t value = (int32_t)(name); return plumed_mpiw_store_constant(out, cap, &value, sizeof(value)); }
#define PLUMED_MPIW_POINTER_CONSTANT(name) \
  static int32_t plumed_mpiw_value_##name(void *out, uint32_t cap) { void *value = (void *)(name); return plumed_mpiw_store_constant(out, cap, &value, sizeof(value)); }
PLUMED_MPIW_HANDLE_CONSTANT(MPI_FLOAT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_DOUBLE, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_CHAR, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_UNSIGNED, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_UNSIGNED_LONG, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_UNSIGNED_LONG_LONG, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG_DOUBLE, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_COMM_WORLD, MPI_Comm)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_COMM_SELF, MPI_Comm)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_COMM_NULL, MPI_Comm)
PLUMED_MPIW_POINTER_CONSTANT(MPI_IN_PLACE)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_SUM, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_PROD, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_MAX, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_MIN, MPI_Op)
static int32_t plumed_mpiw_value_MPI_STATUS_IGNORE(void *out, uint32_t cap) {
  void *value = (void *)0; /* Normalize native status-ignore to wrapper null. */
  return plumed_mpiw_store_constant(out, cap, &value, sizeof(value));
}
PLUMED_MPIW_HANDLE_CONSTANT(MPI_GROUP_NULL, MPI_Group)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_GROUP_EMPTY, MPI_Group)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_REQUEST_NULL, MPI_Request)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_DATATYPE_NULL, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_OP_NULL, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_ERRHANDLER_NULL, MPI_Errhandler)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_ERRORS_ARE_FATAL, MPI_Errhandler)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_ERRORS_RETURN, MPI_Errhandler)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_BYTE, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_PACKED, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_SHORT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_UNSIGNED_CHAR, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_UNSIGNED_SHORT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG_LONG_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG_LONG, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_FLOAT_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_DOUBLE_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_2INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_SHORT_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LONG_DOUBLE_INT, MPI_Datatype)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_MAXLOC, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_MINLOC, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LAND, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_BAND, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LOR, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_BOR, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_LXOR, MPI_Op)
PLUMED_MPIW_HANDLE_CONSTANT(MPI_BXOR, MPI_Op)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_UNDEFINED)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ANY_SOURCE)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ANY_TAG)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_PROC_NULL)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_IDENT)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_CONGRUENT)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_SIMILAR)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_UNEQUAL)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_CART)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_GRAPH)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_MAX_PROCESSOR_NAME)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_MAX_ERROR_STRING)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_BSEND_OVERHEAD)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_SUCCESS)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_BUFFER)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_COUNT)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_TYPE)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_TAG)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_COMM)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_RANK)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_REQUEST)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_ROOT)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_GROUP)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_OP)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_TOPOLOGY)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_DIMS)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_ARG)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_UNKNOWN)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_TRUNCATE)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_OTHER)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_INTERN)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_PENDING)
PLUMED_MPIW_POINTER_CONSTANT(MPI_BOTTOM)
PLUMED_MPIW_INTEGER_CONSTANT(MPI_ERR_IN_STATUS)
#undef PLUMED_MPIW_HANDLE_CONSTANT
#undef PLUMED_MPIW_INTEGER_CONSTANT
#undef PLUMED_MPIW_POINTER_CONSTANT

static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_FLOAT = &plumed_mpiw_value_MPI_FLOAT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_DOUBLE = &plumed_mpiw_value_MPI_DOUBLE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_INT = &plumed_mpiw_value_MPI_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_CHAR = &plumed_mpiw_value_MPI_CHAR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNSIGNED = &plumed_mpiw_value_MPI_UNSIGNED;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNSIGNED_LONG = &plumed_mpiw_value_MPI_UNSIGNED_LONG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNSIGNED_LONG_LONG = &plumed_mpiw_value_MPI_UNSIGNED_LONG_LONG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG_DOUBLE = &plumed_mpiw_value_MPI_LONG_DOUBLE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_COMM_WORLD = &plumed_mpiw_value_MPI_COMM_WORLD;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_COMM_SELF = &plumed_mpiw_value_MPI_COMM_SELF;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_COMM_NULL = &plumed_mpiw_value_MPI_COMM_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_IN_PLACE = &plumed_mpiw_value_MPI_IN_PLACE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_SUM = &plumed_mpiw_value_MPI_SUM;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_PROD = &plumed_mpiw_value_MPI_PROD;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MAX = &plumed_mpiw_value_MPI_MAX;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MIN = &plumed_mpiw_value_MPI_MIN;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_STATUS_IGNORE = &plumed_mpiw_value_MPI_STATUS_IGNORE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_GROUP_NULL = &plumed_mpiw_value_MPI_GROUP_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_GROUP_EMPTY = &plumed_mpiw_value_MPI_GROUP_EMPTY;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_REQUEST_NULL = &plumed_mpiw_value_MPI_REQUEST_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_DATATYPE_NULL = &plumed_mpiw_value_MPI_DATATYPE_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_OP_NULL = &plumed_mpiw_value_MPI_OP_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERRHANDLER_NULL = &plumed_mpiw_value_MPI_ERRHANDLER_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERRORS_ARE_FATAL = &plumed_mpiw_value_MPI_ERRORS_ARE_FATAL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERRORS_RETURN = &plumed_mpiw_value_MPI_ERRORS_RETURN;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BYTE = &plumed_mpiw_value_MPI_BYTE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_PACKED = &plumed_mpiw_value_MPI_PACKED;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_SHORT = &plumed_mpiw_value_MPI_SHORT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG = &plumed_mpiw_value_MPI_LONG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNSIGNED_CHAR = &plumed_mpiw_value_MPI_UNSIGNED_CHAR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNSIGNED_SHORT = &plumed_mpiw_value_MPI_UNSIGNED_SHORT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG_LONG_INT = &plumed_mpiw_value_MPI_LONG_LONG_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG_LONG = &plumed_mpiw_value_MPI_LONG_LONG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_FLOAT_INT = &plumed_mpiw_value_MPI_FLOAT_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_DOUBLE_INT = &plumed_mpiw_value_MPI_DOUBLE_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG_INT = &plumed_mpiw_value_MPI_LONG_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_2INT = &plumed_mpiw_value_MPI_2INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_SHORT_INT = &plumed_mpiw_value_MPI_SHORT_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LONG_DOUBLE_INT = &plumed_mpiw_value_MPI_LONG_DOUBLE_INT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MAXLOC = &plumed_mpiw_value_MPI_MAXLOC;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MINLOC = &plumed_mpiw_value_MPI_MINLOC;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LAND = &plumed_mpiw_value_MPI_LAND;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BAND = &plumed_mpiw_value_MPI_BAND;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LOR = &plumed_mpiw_value_MPI_LOR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BOR = &plumed_mpiw_value_MPI_BOR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_LXOR = &plumed_mpiw_value_MPI_LXOR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BXOR = &plumed_mpiw_value_MPI_BXOR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNDEFINED = &plumed_mpiw_value_MPI_UNDEFINED;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ANY_SOURCE = &plumed_mpiw_value_MPI_ANY_SOURCE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ANY_TAG = &plumed_mpiw_value_MPI_ANY_TAG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_PROC_NULL = &plumed_mpiw_value_MPI_PROC_NULL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_IDENT = &plumed_mpiw_value_MPI_IDENT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_CONGRUENT = &plumed_mpiw_value_MPI_CONGRUENT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_SIMILAR = &plumed_mpiw_value_MPI_SIMILAR;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_UNEQUAL = &plumed_mpiw_value_MPI_UNEQUAL;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_CART = &plumed_mpiw_value_MPI_CART;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_GRAPH = &plumed_mpiw_value_MPI_GRAPH;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MAX_PROCESSOR_NAME = &plumed_mpiw_value_MPI_MAX_PROCESSOR_NAME;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_MAX_ERROR_STRING = &plumed_mpiw_value_MPI_MAX_ERROR_STRING;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BSEND_OVERHEAD = &plumed_mpiw_value_MPI_BSEND_OVERHEAD;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_SUCCESS = &plumed_mpiw_value_MPI_SUCCESS;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_BUFFER = &plumed_mpiw_value_MPI_ERR_BUFFER;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_COUNT = &plumed_mpiw_value_MPI_ERR_COUNT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_TYPE = &plumed_mpiw_value_MPI_ERR_TYPE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_TAG = &plumed_mpiw_value_MPI_ERR_TAG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_COMM = &plumed_mpiw_value_MPI_ERR_COMM;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_RANK = &plumed_mpiw_value_MPI_ERR_RANK;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_REQUEST = &plumed_mpiw_value_MPI_ERR_REQUEST;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_ROOT = &plumed_mpiw_value_MPI_ERR_ROOT;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_GROUP = &plumed_mpiw_value_MPI_ERR_GROUP;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_OP = &plumed_mpiw_value_MPI_ERR_OP;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_TOPOLOGY = &plumed_mpiw_value_MPI_ERR_TOPOLOGY;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_DIMS = &plumed_mpiw_value_MPI_ERR_DIMS;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_ARG = &plumed_mpiw_value_MPI_ERR_ARG;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_UNKNOWN = &plumed_mpiw_value_MPI_ERR_UNKNOWN;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_TRUNCATE = &plumed_mpiw_value_MPI_ERR_TRUNCATE;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_OTHER = &plumed_mpiw_value_MPI_ERR_OTHER;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_INTERN = &plumed_mpiw_value_MPI_ERR_INTERN;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_PENDING = &plumed_mpiw_value_MPI_ERR_PENDING;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_BOTTOM = &plumed_mpiw_value_MPI_BOTTOM;
static const plumed_mpiw_constant_accessor_fn plumed_mpiw_accessor_MPI_ERR_IN_STATUS = &plumed_mpiw_value_MPI_ERR_IN_STATUS;
#define PLUMED_MPIW_VALUE(name) \
  { (uint32_t)sizeof(plumed_mpiw_metadata_entry), PLUMED_MPIW_VALUE_ACCESSOR, 0u, 0u, "value:" #name, sizeof(plumed_mpiw_constant_accessor_fn), (const void *)&plumed_mpiw_accessor_##name }
static const plumed_mpiw_metadata_entry plumed_mpiw_metadata[] = {
  PLUMED_MPIW_VALUE(MPI_FLOAT),
  PLUMED_MPIW_VALUE(MPI_DOUBLE),
  PLUMED_MPIW_VALUE(MPI_INT),
  PLUMED_MPIW_VALUE(MPI_CHAR),
  PLUMED_MPIW_VALUE(MPI_UNSIGNED),
  PLUMED_MPIW_VALUE(MPI_UNSIGNED_LONG),
  PLUMED_MPIW_VALUE(MPI_UNSIGNED_LONG_LONG),
  PLUMED_MPIW_VALUE(MPI_LONG_DOUBLE),
  PLUMED_MPIW_VALUE(MPI_COMM_WORLD),
  PLUMED_MPIW_VALUE(MPI_COMM_SELF),
  PLUMED_MPIW_VALUE(MPI_COMM_NULL),
  PLUMED_MPIW_VALUE(MPI_IN_PLACE),
  PLUMED_MPIW_VALUE(MPI_SUM),
  PLUMED_MPIW_VALUE(MPI_PROD),
  PLUMED_MPIW_VALUE(MPI_MAX),
  PLUMED_MPIW_VALUE(MPI_MIN),
  PLUMED_MPIW_VALUE(MPI_STATUS_IGNORE),
  PLUMED_MPIW_VALUE(MPI_GROUP_NULL),
  PLUMED_MPIW_VALUE(MPI_GROUP_EMPTY),
  PLUMED_MPIW_VALUE(MPI_REQUEST_NULL),
  PLUMED_MPIW_VALUE(MPI_DATATYPE_NULL),
  PLUMED_MPIW_VALUE(MPI_OP_NULL),
  PLUMED_MPIW_VALUE(MPI_ERRHANDLER_NULL),
  PLUMED_MPIW_VALUE(MPI_ERRORS_ARE_FATAL),
  PLUMED_MPIW_VALUE(MPI_ERRORS_RETURN),
  PLUMED_MPIW_VALUE(MPI_BYTE),
  PLUMED_MPIW_VALUE(MPI_PACKED),
  PLUMED_MPIW_VALUE(MPI_SHORT),
  PLUMED_MPIW_VALUE(MPI_LONG),
  PLUMED_MPIW_VALUE(MPI_UNSIGNED_CHAR),
  PLUMED_MPIW_VALUE(MPI_UNSIGNED_SHORT),
  PLUMED_MPIW_VALUE(MPI_LONG_LONG_INT),
  PLUMED_MPIW_VALUE(MPI_LONG_LONG),
  PLUMED_MPIW_VALUE(MPI_FLOAT_INT),
  PLUMED_MPIW_VALUE(MPI_DOUBLE_INT),
  PLUMED_MPIW_VALUE(MPI_LONG_INT),
  PLUMED_MPIW_VALUE(MPI_2INT),
  PLUMED_MPIW_VALUE(MPI_SHORT_INT),
  PLUMED_MPIW_VALUE(MPI_LONG_DOUBLE_INT),
  PLUMED_MPIW_VALUE(MPI_MAXLOC),
  PLUMED_MPIW_VALUE(MPI_MINLOC),
  PLUMED_MPIW_VALUE(MPI_LAND),
  PLUMED_MPIW_VALUE(MPI_BAND),
  PLUMED_MPIW_VALUE(MPI_LOR),
  PLUMED_MPIW_VALUE(MPI_BOR),
  PLUMED_MPIW_VALUE(MPI_LXOR),
  PLUMED_MPIW_VALUE(MPI_BXOR),
  PLUMED_MPIW_VALUE(MPI_UNDEFINED),
  PLUMED_MPIW_VALUE(MPI_ANY_SOURCE),
  PLUMED_MPIW_VALUE(MPI_ANY_TAG),
  PLUMED_MPIW_VALUE(MPI_PROC_NULL),
  PLUMED_MPIW_VALUE(MPI_IDENT),
  PLUMED_MPIW_VALUE(MPI_CONGRUENT),
  PLUMED_MPIW_VALUE(MPI_SIMILAR),
  PLUMED_MPIW_VALUE(MPI_UNEQUAL),
  PLUMED_MPIW_VALUE(MPI_CART),
  PLUMED_MPIW_VALUE(MPI_GRAPH),
  PLUMED_MPIW_VALUE(MPI_MAX_PROCESSOR_NAME),
  PLUMED_MPIW_VALUE(MPI_MAX_ERROR_STRING),
  PLUMED_MPIW_VALUE(MPI_BSEND_OVERHEAD),
  PLUMED_MPIW_VALUE(MPI_SUCCESS),
  PLUMED_MPIW_VALUE(MPI_ERR_BUFFER),
  PLUMED_MPIW_VALUE(MPI_ERR_COUNT),
  PLUMED_MPIW_VALUE(MPI_ERR_TYPE),
  PLUMED_MPIW_VALUE(MPI_ERR_TAG),
  PLUMED_MPIW_VALUE(MPI_ERR_COMM),
  PLUMED_MPIW_VALUE(MPI_ERR_RANK),
  PLUMED_MPIW_VALUE(MPI_ERR_REQUEST),
  PLUMED_MPIW_VALUE(MPI_ERR_ROOT),
  PLUMED_MPIW_VALUE(MPI_ERR_GROUP),
  PLUMED_MPIW_VALUE(MPI_ERR_OP),
  PLUMED_MPIW_VALUE(MPI_ERR_TOPOLOGY),
  PLUMED_MPIW_VALUE(MPI_ERR_DIMS),
  PLUMED_MPIW_VALUE(MPI_ERR_ARG),
  PLUMED_MPIW_VALUE(MPI_ERR_UNKNOWN),
  PLUMED_MPIW_VALUE(MPI_ERR_TRUNCATE),
  PLUMED_MPIW_VALUE(MPI_ERR_OTHER),
  PLUMED_MPIW_VALUE(MPI_ERR_INTERN),
  PLUMED_MPIW_VALUE(MPI_ERR_PENDING),
  PLUMED_MPIW_VALUE(MPI_BOTTOM),
  PLUMED_MPIW_VALUE(MPI_ERR_IN_STATUS)
};

typedef struct {
  char c;
  MPI_Status value;
} plumed_mpiw_status_alignment;
static const plumed_mpiw_provider_descriptor plumed_mpiw_descriptor = {
  (uint32_t)sizeof(plumed_mpiw_provider_descriptor), 1u, 0u,
  PLUMED_MPIW_RUNTIME_PROCESS_MPI, 0u, {0u, 0u, 0u, 0u},
  (uint32_t)sizeof(MPI_Comm), (uint32_t)sizeof(MPI_Group),
  (uint32_t)sizeof(MPI_Datatype), (uint32_t)sizeof(MPI_Request),
  (uint32_t)sizeof(MPI_Op), (uint32_t)sizeof(MPI_Errhandler),
  (uint32_t)sizeof(MPI_Status), (uint32_t)offsetof(plumed_mpiw_status_alignment, value),
  (uint32_t)sizeof(int), 1u, (uint32_t)sizeof(MPI_Fint), 1u,
  (uint32_t)sizeof(plumed_mpiw_handle_storage), PLUMED_MPIW_STATUS_CAPACITY,
  {0u, 0u, 0u, 0u, 0u, 0u}, "native-mpi", "header-abi",
  (uint64_t)(sizeof(plumed_mpiw_functions) / sizeof(plumed_mpiw_functions[0])), plumed_mpiw_functions,
  (uint64_t)(sizeof(plumed_mpiw_metadata) / sizeof(plumed_mpiw_metadata[0])), plumed_mpiw_metadata,
  (plumed_mpiw_generic_fn)(&MPI_Comm_rank), (plumed_mpiw_generic_fn)(&MPI_Initialized), 0
};
#undef PLUMED_MPIW_VALUE
#undef PLUMED_MPIW_ENTRY
static inline const plumed_mpiw_provider_descriptor *plumed_mpiw_get_descriptor(void) {
  plumed_mpiw_verify_typed_functions();
  return &plumed_mpiw_descriptor;
}
