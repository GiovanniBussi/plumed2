# MPI wrapper boundary

Provider-mode MPI handle wrappers keep their native-handle bytes at offset zero
inside zero-initialized storage. This is a deliberate compatibility contract:
existing PLUMED callers can continue passing `&Communicator::Get_comm()` to a
nested kernel without changing their source or constructing an export buffer.
The receiving compatible kernel copies exactly the provider-reported native
handle size from that address into an aligned native handle object. The wrapper
storage capacity is validated before use; equal bytes alone do not make a
communicator valid across different MPI runtimes.

Native-MPI builds retain native MPI handle types and direct calls. Explicit MPI
provider injection is an opt-in API for callers that need to share an existing
MPI runtime with a nested PLUMED kernel.
