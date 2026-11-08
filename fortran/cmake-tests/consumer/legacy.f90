program check_legacy
  use iso_c_binding
  use plumed_module
  implicit none
  character(len=32) :: p
  integer(c_int) :: version
  version=-1
  call plumed_f_create(p)
  call plumed_f_cmd(p,"getApiVersion"//c_null_char,version)
  if(version<=0) stop 1
  call plumed_f_finalize(p)
end program check_legacy
