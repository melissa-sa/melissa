 
program heat

  use heat_utils
  use iso_c_binding, only: C_CHAR, C_NULL_CHAR

  implicit none

  include "melissa_api.f90"

  integer :: i, j, k, nx, ny, n, nmax, me, np, i1, iN, statinfo, nb_op, next, previous, narg
  integer, dimension(mpi_status_size) :: status
  real*8 :: lx, ly, dt, dx, dy, d, t, epsilon, t1, t2, temp
  real*8, dimension(:), pointer :: U => null(), F => null()
  real*8, dimension(3) :: A
  real*8,dimension(5) :: param
  character(len=32) :: arg
  integer :: comm
  integer :: sample_id = 0, sobol_rank = 0, coupling = 1
  character(len=5) :: name = C_CHAR_"heat"//C_NULL_CHAR

  call mpi_init(statinfo)

  narg = iargc()
  param(:) = 0
  if (narg .lt. 2) then
    print*,"Missing parameter"
    return
  endif
  do n=2, 6
    if(narg .ge. n) then
      call getarg(n, arg)
      read( arg, * ) param(n-1)
    endif
  enddo
  ! initial temperature
  temp = param(1)
  if(narg .gt. 3) then
    call getarg(narg - 1, arg)
    read( arg, * ) sobol_rank ! sobol rank
  endif
  if(narg .gt. 2) then
    call getarg(narg, arg)
    read( arg, * ) sample_id ! sobol group
  endif

  call MPI_Comm_split(MPI_COMM_WORLD, sobol_rank, me, comm, statinfo);
  call mpi_comm_rank(comm, me, statinfo)
  call mpi_comm_size(comm, np, statinfo)
  
  t1=mpi_wtime()
  
  next = me+1
  previous = me-1
  
  if(next == np)     next=mpi_proc_null
  if(previous == -1) previous=mpi_proc_null

  call read_file(nx, ny, lx, ly, d)

  call load(me, nx*ny, Np, i1, iN)

  nb_op   = in-i1+1
  dt      = 0.01
  nmax    = 100
  dx      = lx/(nx+1)
  dy      = ly/(ny+1)
  epsilon = 0.0001

  allocate(U(in-i1+1))
  allocate(F(in-i1+1))
  call init(U, i1, iN, dx, dy, nx, lx, ly, temp)
  call filling_A(d, dx, dy, dt, nx, ny, A) ! fill A

  call melissa_init (nb_op, np, me, sobol_rank, sample_id, comm, coupling)

  do n=1, nmax
    t = t + dt
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, i1, in, lx, ly, param)
    call conjgrad(A, F, U, nx, ny, epsilon, i1, in, np, me, next, previous, comm)

  end do

  n = 1
  call melissa_send(n, name, u, me, sobol_rank, sample_id)
  call finalize(dx, dy, nx, ny, i1, in, u, f, me, sample_id)

  call melissa_finalize ()
  
  t2 = mpi_wtime()
  print*, 'Calcul time:', t2-t1, 'sec'
  print*, 'Final time step:', t
  
  deallocate(U, F)

  call mpi_finalize(statinfo)

end program heat
