program check_f08
  use iso_c_binding
  use plumed_f08_module
  implicit none
  type(plumed) :: p
  type(plumed_error) :: error
  integer(c_int), target :: version
  version=-1
  call p%cmd_const_ptr("getApiVersion",c_loc(version),error=error)
  if(error%code/=20300.or.version/=-1) stop 1
  call p%cmd_ptr("getApiVersion",c_loc(version),error=error)
  if(error%code/=0.or.version<=0) stop 2
  call p%finalize()
end program check_f08
