// Generated provider definitions for calls without a runtime dispatch.
int mpiwrap_Init(int *argc, char ***argv) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Init() {
  return false;
}
int mpiwrap_Finalize() {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Finalize (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Finalize() {
  return false;
}
int mpiwrap_Initialized(int *flag) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Initialized (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Initialized() {
  return false;
}
int mpiwrap_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_dup (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_dup() {
  return false;
}
int mpiwrap_Comm_free(MPI_Comm *comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_free() {
  return false;
}
int mpiwrap_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_split (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_split() {
  return false;
}
int mpiwrap_Abort(MPI_Comm comm, int errorcode) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Abort (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Abort() {
  return false;
}
int mpiwrap_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Scatter (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Scatter() {
  return false;
}
int mpiwrap_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Gather (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Gather() {
  return false;
}
int mpiwrap_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Allgather (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Allgather() {
  return false;
}
int mpiwrap_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Allgatherv (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Allgatherv() {
  return false;
}
int mpiwrap_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Recv (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Recv() {
  return false;
}
int mpiwrap_Error_string(int errorcode, char *string, int *resultlen) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Error_string (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Error_string() {
  return false;
}
int mpiwrap_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Errhandler *errhandler) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_create_errhandler (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_create_errhandler() {
  return false;
}
MPI_Comm mpiwrap_Comm_f2c(std::int32_t comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_f2c (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_f2c() {
  return false;
}
int mpiwrap_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Send (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Send() {
  return false;
}
int mpiwrap_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Bsend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Bsend() {
  return false;
}
int mpiwrap_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Ssend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Ssend() {
  return false;
}
int mpiwrap_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Rsend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Rsend() {
  return false;
}
int mpiwrap_Buffer_attach(void *buffer, int size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Buffer_attach (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Buffer_attach() {
  return false;
}
int mpiwrap_Buffer_detach(void *buffer_addr, int *size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Buffer_detach (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Buffer_detach() {
  return false;
}
int mpiwrap_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Ibsend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Ibsend() {
  return false;
}
int mpiwrap_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Issend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Issend() {
  return false;
}
int mpiwrap_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Irsend (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Irsend() {
  return false;
}
int mpiwrap_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Test (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Test() {
  return false;
}
int mpiwrap_Request_free(MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Request_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Request_free() {
  return false;
}
int mpiwrap_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Waitany (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Waitany() {
  return false;
}
int mpiwrap_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Testany (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Testany() {
  return false;
}
int mpiwrap_Waitall(int count, MPI_Request array_of_requests[], MPI_Status *array_of_statuses) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Waitall (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Waitall() {
  return false;
}
int mpiwrap_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status *array_of_statuses) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Testall (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Testall() {
  return false;
}
int mpiwrap_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Waitsome (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Waitsome() {
  return false;
}
int mpiwrap_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Testsome (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Testsome() {
  return false;
}
int mpiwrap_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Iprobe (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Iprobe() {
  return false;
}
int mpiwrap_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Probe (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Probe() {
  return false;
}
int mpiwrap_Cancel(MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cancel (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cancel() {
  return false;
}
int mpiwrap_Test_cancelled(const MPI_Status *status, int *flag) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Test_cancelled (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Test_cancelled() {
  return false;
}
int mpiwrap_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Send_init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Send_init() {
  return false;
}
int mpiwrap_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Bsend_init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Bsend_init() {
  return false;
}
int mpiwrap_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Ssend_init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Ssend_init() {
  return false;
}
int mpiwrap_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Rsend_init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Rsend_init() {
  return false;
}
int mpiwrap_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Recv_init (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Recv_init() {
  return false;
}
int mpiwrap_Start(MPI_Request *request) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Start (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Start() {
  return false;
}
int mpiwrap_Startall(int count, MPI_Request array_of_requests[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Startall (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Startall() {
  return false;
}
int mpiwrap_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Sendrecv_replace (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Sendrecv_replace() {
  return false;
}
int mpiwrap_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_contiguous (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_contiguous() {
  return false;
}
int mpiwrap_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_vector (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_vector() {
  return false;
}
int mpiwrap_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_indexed (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_indexed() {
  return false;
}
int mpiwrap_Type_commit(MPI_Datatype *datatype) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_commit (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_commit() {
  return false;
}
int mpiwrap_Type_free(MPI_Datatype *datatype) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_free() {
  return false;
}
int mpiwrap_Type_size(MPI_Datatype datatype, int *size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Type_size (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Type_size() {
  return false;
}
int mpiwrap_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Get_elements (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Get_elements() {
  return false;
}
int mpiwrap_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Pack (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Pack() {
  return false;
}
int mpiwrap_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Unpack (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Unpack() {
  return false;
}
int mpiwrap_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Pack_size (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Pack_size() {
  return false;
}
int mpiwrap_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Gatherv (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Gatherv() {
  return false;
}
int mpiwrap_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Scatterv (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Scatterv() {
  return false;
}
int mpiwrap_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Alltoall (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Alltoall() {
  return false;
}
int mpiwrap_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Alltoallv (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Alltoallv() {
  return false;
}
int mpiwrap_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Reduce (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Reduce() {
  return false;
}
int mpiwrap_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Op_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Op_create() {
  return false;
}
int mpiwrap_Op_free(MPI_Op *op) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Op_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Op_free() {
  return false;
}
int mpiwrap_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Reduce_scatter (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Reduce_scatter() {
  return false;
}
int mpiwrap_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Scan (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Scan() {
  return false;
}
int mpiwrap_Group_size(MPI_Group group, int *size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_size (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_size() {
  return false;
}
int mpiwrap_Group_rank(MPI_Group group, int *rank) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_rank (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_rank() {
  return false;
}
int mpiwrap_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_translate_ranks (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_translate_ranks() {
  return false;
}
int mpiwrap_Group_compare(MPI_Group group1, MPI_Group group2, int *result) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_compare (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_compare() {
  return false;
}
int mpiwrap_Comm_group(MPI_Comm comm, MPI_Group *group) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_group (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_group() {
  return false;
}
int mpiwrap_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_union (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_union() {
  return false;
}
int mpiwrap_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_intersection (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_intersection() {
  return false;
}
int mpiwrap_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_difference (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_difference() {
  return false;
}
int mpiwrap_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_incl (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_incl() {
  return false;
}
int mpiwrap_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_excl (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_excl() {
  return false;
}
int mpiwrap_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_range_incl (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_range_incl() {
  return false;
}
int mpiwrap_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_range_excl (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_range_excl() {
  return false;
}
int mpiwrap_Group_free(MPI_Group *group) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Group_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Group_free() {
  return false;
}
int mpiwrap_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_compare (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_compare() {
  return false;
}
int mpiwrap_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_create() {
  return false;
}
int mpiwrap_Comm_test_inter(MPI_Comm comm, int *flag) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_test_inter (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_test_inter() {
  return false;
}
int mpiwrap_Comm_remote_size(MPI_Comm comm, int *size) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_remote_size (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_remote_size() {
  return false;
}
int mpiwrap_Comm_remote_group(MPI_Comm comm, MPI_Group *group) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_remote_group (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_remote_group() {
  return false;
}
int mpiwrap_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Intercomm_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Intercomm_create() {
  return false;
}
int mpiwrap_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Intercomm_merge (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Intercomm_merge() {
  return false;
}
int mpiwrap_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_create() {
  return false;
}
int mpiwrap_Dims_create(int nnodes, int ndims, int dims[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Dims_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Dims_create() {
  return false;
}
int mpiwrap_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[], int reorder, MPI_Comm *comm_graph) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graph_create (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graph_create() {
  return false;
}
int mpiwrap_Topo_test(MPI_Comm comm, int *status) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Topo_test (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Topo_test() {
  return false;
}
int mpiwrap_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graphdims_get (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graphdims_get() {
  return false;
}
int mpiwrap_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graph_get (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graph_get() {
  return false;
}
int mpiwrap_Cartdim_get(MPI_Comm comm, int *ndims) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cartdim_get (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cartdim_get() {
  return false;
}
int mpiwrap_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_get (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_get() {
  return false;
}
int mpiwrap_Cart_rank(MPI_Comm comm, const int coords[], int *rank) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_rank (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_rank() {
  return false;
}
int mpiwrap_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_coords (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_coords() {
  return false;
}
int mpiwrap_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graph_neighbors_count (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graph_neighbors_count() {
  return false;
}
int mpiwrap_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graph_neighbors (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graph_neighbors() {
  return false;
}
int mpiwrap_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_shift (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_shift() {
  return false;
}
int mpiwrap_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_sub (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_sub() {
  return false;
}
int mpiwrap_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Cart_map (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Cart_map() {
  return false;
}
int mpiwrap_Graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Graph_map (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Graph_map() {
  return false;
}
int mpiwrap_Get_processor_name(char *name, int *resultlen) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Get_processor_name (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Get_processor_name() {
  return false;
}
int mpiwrap_Errhandler_free(MPI_Errhandler *errhandler) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Errhandler_free (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Errhandler_free() {
  return false;
}
int mpiwrap_Error_class(int errorcode, int *errorclass) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Error_class (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Error_class() {
  return false;
}
double mpiwrap_Wtime() {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Wtime (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Wtime() {
  return false;
}
double mpiwrap_Wtick() {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Wtick (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Wtick() {
  return false;
}
int mpiwrap_Pcontrol(const int level, ...) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Pcontrol (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Pcontrol() {
  return false;
}
int mpiwrap_Finalized(int *flag) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Finalized (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Finalized() {
  return false;
}
int mpiwrap_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_get_errhandler (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_get_errhandler() {
  return false;
}
int mpiwrap_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) {
  throw std::runtime_error(std::string("MPI function unavailable: MPI_Comm_set_errhandler (provider ") + mpiwrap_provider::currentRegistry().providerName() + ")");
}
bool mpiwrap_has_Comm_set_errhandler() {
  return false;
}
