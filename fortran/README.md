# Fortran interfaces

PLUMED provides three Fortran interfaces. The generated sources are
`plumed.f90` and `plumed_f08.f90`; their templates are maintained alongside
them.

## Legacy external routines

The external `plumed_f_*` entry points (also available through the older global
`plumed_f_g*` routines) use a fixed 32-character handle and pass command data
as an untyped address. The caller is responsible for passing storage with the
right type, rank, and lifetime. These calls use the legacy error behavior: a
PLUMED command error is fatal to the process. Use the module interfaces below
for typed argument checking and recoverable errors.

## `plumed_module`

`USE plumed_module` provides procedural `plumed_f_cmd` and `plumed_f_gcmd`
generics. They have generated overloads for supported intrinsic types, kinds,
and ranks; the safe-pointer metadata lets PLUMED check the argument type and
shape against the command. The handle is still the 32-character legacy handle,
and this interface does not provide the `plumed` object's automatic
reference-counted lifetime or optional error object.

The module passes array extents in reverse Fortran dimension order at the C
boundary, so C-side shape metadata follows the ordering expected by PLUMED.
Character values use the declared Fortran character length; unlike the F08
`cmd_val` convenience call, the module's character overload does not append a
NUL terminator to the value. Append `c_null_char` to command keys and to character values when the command
expects a C string; the procedural module does not terminate them for you.

## `plumed_f08_module`

`USE plumed_f08_module` provides the `type(plumed)` handle, created with
`plumed_create` or lazily on first command. Defined assignment creates a shared
reference, and finalization releases the object's reference. `valid()` and
`use_count()` inspect a handle. `finalize()` is available for explicit release.

For typed arguments, use:

- `cmd_val(key, value)` for an input value that PLUMED may use only during the
  call. Numeric scalars and contiguous arrays are supported. Character values
  are passed with a trailing NUL.
- `cmd_ref(key, value)` for a mutable argument available only during the call.
- `cmd_ptr(key, pointer)` or `cmd_const_ptr(key, pointer)` when PLUMED may keep
  the buffer address after the call. The pointed-to storage must remain valid
  for as long as PLUMED may use it. Rank-greater-than-zero pointer arguments
  must be contiguous; non-contiguous pointers return error 20300 when an error
  object is supplied, and otherwise stop execution.

For `cmd_ptr` and `cmd_const_ptr`, the pointer category tells PLUMED whether it
may write through the address. The `c_ptr` overloads are an escape hatch for
interoperability: they carry no element type or shape metadata, so the caller
must know the expected representation and storage. They still set explicit
mutable (`cmd_ptr`) or const (`cmd_const_ptr`) pointer metadata; `c_ptr` does
not make constness implicit. Because shape is unknown, use raw pointers only
when the command can work with the supplied address without typed shape
metadata.

For typed rank-N arrays, the wrapper passes extents in reverse Fortran order
at the C boundary. Character command keys are NUL-terminated by the wrapper;
`cmd_val` also appends a NUL to its character value. Raw pointer calls do not
add a terminator or infer the pointed-to string length: provide the string
representation required by the command.

Pass an optional `type(plumed_error)` argument to any command to receive
recoverable failures. Its `code` is zero on success; `what` contains the error
text, and `nested` may hold a nested error when nested exceptions are enabled.
Without this argument, a command error follows the throwing/fatal path. Check
the returned code before using results that depend on a successful command.

## Reference-count workarounds

Ordinary ownership should use `plumed_create`, defined assignment, and automatic
finalization. `incref()` and `decref()` remain public for advanced recovery from
compiler-specific finalization defects. They adjust the shared handle count;
`decref(1)` is appropriate only when the caller knows there should be exactly
one owning reference. It must not discard references still owned elsewhere.
Do not call it automatically after ordinary assignment. Finalization behavior
for function results and derived-type cases varies by compiler and version, so
validate the relevant compiler before relying on a workaround.

## Using installed module sources

The installation exports `PLUMED_FORTRAN` and `PLUMED_FORTRAN08` paths to the
generated source files. A Make or Autoconf build can compile the selected source
with its own Fortran compiler, writing the object and `.mod` files into its own
build tree. For example, use `$(PLUMED_FORTRAN)` for `plumed_module` or
`$(PLUMED_FORTRAN08)` for `plumed_f08_module`; add the resulting module output
directory to the consumer's module search path. Fortran `.mod` files are
compiler-specific, so use the consumer's compiler and do not share them across
compiler builds.

