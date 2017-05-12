 
program heat

  use heat_utils
  use melissa_api_fortran
  
  implicit none

  integer :: nx, ny, n, nmax, nb_op, narg
  real*8 :: lx, ly, dt, dx, dy, d, t, epsilon, t1, t2, temp = 0
  real*8,dimension(:),pointer :: U => null(), F => null()
  real*8,dimension(3) :: A
  character(len=32) :: arg
  character(len=5) :: name = C_CHAR_"heat"//C_NULL_CHAR
  integer :: sobol_rank = 0, sobol_group = 0

  narg = iargc()
  if(narg.ge.1) call getarg(1, arg)
  read( arg, * ) temp ! initial temperature
!  allocate(parameters(1))
!  parameters(1) = temp
  if(narg.ge.3) then
    call getarg(narg - 1, arg)
    read( arg, * ) sobol_rank ! sobol rank
    call getarg(narg, arg)
    read( arg, * ) sobol_group ! sobol group
  endif

  call cpu_time(t1)

  call read_file(nx, ny, lx, ly, d)

  nb_op   = nx*ny
  dt      = 0.01
  nmax    = 100
  dx      = lx/(nx+1)
  dy      = ly/(ny+1)
  epsilon = 0.0001

  allocate(U(nb_op))
  allocate(F(nb_op))
  call init(U, nb_op, temp)
  call filling_A(d, dx, dy, dt, A) ! fill A

  call melissa_init_no_mpi(nb_op, sobol_rank, sobol_group)

  do n=1,nmax
    t = t + dt
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, nb_op, lx, ly)
    call conjgrad(A, F, U, nx, ny, epsilon)

    call melissa_send_no_mpi(n, name, u, sobol_rank, sobol_group)
  end do

  call finalize(dx, dy, nx, ny, nb_op, u, f)

  call melissa_finalize()
  
  call cpu_time(t2)
  print*,'Calcul time:', t2-t1, 'sec'
  print*,'Final time step:', t
  
  deallocate(U, F)

end program heat
