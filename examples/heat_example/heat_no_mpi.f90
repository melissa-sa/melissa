

program heat_no_mpi

  use heat_utils_no_mpi

  implicit none

  include "melissa_api.f90"

  integer :: nx, ny, n, nmax, vect_size, narg
  real*8 :: lx, ly, dt, dx, dy, d, t, epsilon, t1, t2
  real*8,dimension(:),pointer :: U => null(), F => null()
  real*8,dimension(3) :: A
  real*8,dimension(5) :: param
  character(len=32) :: arg
  integer ::  sample_id = 0, sobol_rank = 0
  character(len=5) :: name = C_CHAR_"heat"//C_NULL_CHAR

  narg = iargc()
  if (narg .lt. 4) then
    print*,"Missing parameter"
    return
  endif

  param(:) = 0
  call getarg(1, arg)
  print*, arg
  read( arg, * ) sobol_rank ! sobol rank
  call getarg(2, arg)
  read( arg, * ) sample_id ! sobol group
  print*, arg
  call getarg(3, arg)
  read( arg, * ) param(1) ! initial temperature
  print*, arg

  do n=5, 8
    if(narg .ge. n) then
      call getarg(n-1, arg)
      read( arg, * ) param(n-3)
    endif
  enddo

  call cpu_time(t1)

  nx        = 100
  ny        = 100
  lx        = 10.0
  ly        = 10.0
  d         = 1.0
  dt        = 0.01
  nmax      = 100
  dx        = lx/(nx+1)
  dy        = ly/(ny+1)
  epsilon   = 0.0001

  allocate(U(vect_size))
  allocate(F(vect_size))
  call init(U, vect_size, param(1))
  call filling_A(d, dx, dy, dt, A) ! fill A

  call melissa_init_no_mpi(vect_size, sobol_rank, sample_id)

  do n=1, nmax
    t = t + dt
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, vect_size, lx, ly, param)
    call conjgrad(A, F, U, nx, ny, epsilon)
    call melissa_send_no_mpi(n, name, u, sobol_rank, sample_id)
  end do

  call finalize(dx, dy, nx, ny, vect_size, u, f, sample_id)

  call melissa_finalize()
  
  call cpu_time(t2)
  print*,'Calcul time:', t2-t1, 'sec'
  print*,'Final time step:', t
  
  deallocate(U, F)

end program heat_no_mpi
