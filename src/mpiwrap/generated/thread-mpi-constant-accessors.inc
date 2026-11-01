// Generated stable thread-MPI constant accessors.
int32_t get_MPI_FLOAT(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_FLOAT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_FLOAT = &get_MPI_FLOAT;
int32_t get_MPI_DOUBLE(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_DOUBLE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_DOUBLE = &get_MPI_DOUBLE;
int32_t get_MPI_INT(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_INT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_INT = &get_MPI_INT;
int32_t get_MPI_CHAR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_CHAR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_CHAR = &get_MPI_CHAR;
int32_t get_MPI_UNSIGNED(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_UNSIGNED);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNSIGNED = &get_MPI_UNSIGNED;
int32_t get_MPI_UNSIGNED_LONG(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_UNSIGNED_LONG);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNSIGNED_LONG = &get_MPI_UNSIGNED_LONG;
int32_t get_MPI_UNSIGNED_LONG_LONG(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_UNSIGNED_LONG_LONG);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNSIGNED_LONG_LONG = &get_MPI_UNSIGNED_LONG_LONG;
int32_t get_MPI_LONG_DOUBLE(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LONG_DOUBLE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LONG_DOUBLE = &get_MPI_LONG_DOUBLE;
int32_t get_MPI_COMM_WORLD(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_COMM_WORLD);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_COMM_WORLD = &get_MPI_COMM_WORLD;
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_COMM_SELF = &get_self;
int32_t get_MPI_COMM_NULL(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_COMM_NULL);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_COMM_NULL = &get_MPI_COMM_NULL;
int32_t get_MPI_IN_PLACE(void *output, uint32_t capacity) {
  return exportPointer(output, capacity, TMPI_IN_PLACE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_IN_PLACE = &get_MPI_IN_PLACE;
int32_t get_MPI_SUM(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_SUM);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_SUM = &get_MPI_SUM;
int32_t get_MPI_PROD(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_PROD);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_PROD = &get_MPI_PROD;
int32_t get_MPI_MAX(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_MAX);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_MAX = &get_MPI_MAX;
int32_t get_MPI_MIN(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_MIN);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_MIN = &get_MPI_MIN;
int32_t get_MPI_STATUS_IGNORE(void *output, uint32_t capacity) {
  return exportPointer(output, capacity, TMPI_STATUS_IGNORE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_STATUS_IGNORE = &get_MPI_STATUS_IGNORE;
int32_t get_MPI_GROUP_NULL(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_GROUP_NULL);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_GROUP_NULL = &get_MPI_GROUP_NULL;
int32_t get_MPI_GROUP_EMPTY(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_GROUP_EMPTY);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_GROUP_EMPTY = &get_MPI_GROUP_EMPTY;
int32_t get_MPI_REQUEST_NULL(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_REQUEST_NULL);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_REQUEST_NULL = &get_MPI_REQUEST_NULL;
int32_t get_MPI_ERRORS_ARE_FATAL(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_ERRORS_ARE_FATAL);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERRORS_ARE_FATAL = &get_MPI_ERRORS_ARE_FATAL;
int32_t get_MPI_ERRORS_RETURN(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_ERRORS_RETURN);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERRORS_RETURN = &get_MPI_ERRORS_RETURN;
int32_t get_MPI_BYTE(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_BYTE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_BYTE = &get_MPI_BYTE;
int32_t get_MPI_SHORT(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_SHORT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_SHORT = &get_MPI_SHORT;
int32_t get_MPI_LONG(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LONG);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LONG = &get_MPI_LONG;
int32_t get_MPI_UNSIGNED_CHAR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_UNSIGNED_CHAR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNSIGNED_CHAR = &get_MPI_UNSIGNED_CHAR;
int32_t get_MPI_UNSIGNED_SHORT(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_UNSIGNED_SHORT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNSIGNED_SHORT = &get_MPI_UNSIGNED_SHORT;
int32_t get_MPI_LONG_LONG_INT(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LONG_LONG_INT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LONG_LONG_INT = &get_MPI_LONG_LONG_INT;
int32_t get_MPI_LONG_LONG(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LONG_LONG);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LONG_LONG = &get_MPI_LONG_LONG;
int32_t get_MPI_LAND(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LAND);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LAND = &get_MPI_LAND;
int32_t get_MPI_BAND(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_BAND);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_BAND = &get_MPI_BAND;
int32_t get_MPI_LOR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LOR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LOR = &get_MPI_LOR;
int32_t get_MPI_BOR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_BOR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_BOR = &get_MPI_BOR;
int32_t get_MPI_LXOR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_LXOR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_LXOR = &get_MPI_LXOR;
int32_t get_MPI_BXOR(void *output, uint32_t capacity) {
  return exportHandle(output, capacity, TMPI_BXOR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_BXOR = &get_MPI_BXOR;
int32_t get_MPI_UNDEFINED(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_UNDEFINED);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNDEFINED = &get_MPI_UNDEFINED;
int32_t get_MPI_ANY_SOURCE(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ANY_SOURCE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ANY_SOURCE = &get_MPI_ANY_SOURCE;
int32_t get_MPI_ANY_TAG(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ANY_TAG);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ANY_TAG = &get_MPI_ANY_TAG;
int32_t get_MPI_IDENT(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_IDENT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_IDENT = &get_MPI_IDENT;
int32_t get_MPI_CONGRUENT(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_CONGRUENT);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_CONGRUENT = &get_MPI_CONGRUENT;
int32_t get_MPI_SIMILAR(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_SIMILAR);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_SIMILAR = &get_MPI_SIMILAR;
int32_t get_MPI_UNEQUAL(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_UNEQUAL);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_UNEQUAL = &get_MPI_UNEQUAL;
int32_t get_MPI_CART(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_CART);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_CART = &get_MPI_CART;
int32_t get_MPI_GRAPH(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_GRAPH);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_GRAPH = &get_MPI_GRAPH;
int32_t get_MPI_MAX_PROCESSOR_NAME(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_MAX_PROCESSOR_NAME);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_MAX_PROCESSOR_NAME = &get_MPI_MAX_PROCESSOR_NAME;
int32_t get_MPI_MAX_ERROR_STRING(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_MAX_ERROR_STRING);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_MAX_ERROR_STRING = &get_MPI_MAX_ERROR_STRING;
int32_t get_MPI_SUCCESS(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_SUCCESS);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_SUCCESS = &get_MPI_SUCCESS;
int32_t get_MPI_ERR_TYPE(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ERR_TYPE);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERR_TYPE = &get_MPI_ERR_TYPE;
int32_t get_MPI_ERR_COMM(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ERR_COMM);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERR_COMM = &get_MPI_ERR_COMM;
int32_t get_MPI_ERR_GROUP(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ERR_GROUP);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERR_GROUP = &get_MPI_ERR_GROUP;
int32_t get_MPI_ERR_DIMS(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ERR_DIMS);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERR_DIMS = &get_MPI_ERR_DIMS;
int32_t get_MPI_ERR_UNKNOWN(void *output, uint32_t capacity) {
  return exportInteger(output, capacity, TMPI_ERR_UNKNOWN);
}
static const plumed_mpiw_constant_accessor_fn constant_accessor_MPI_ERR_UNKNOWN = &get_MPI_ERR_UNKNOWN;
