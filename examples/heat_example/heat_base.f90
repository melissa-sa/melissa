 
program heat

  use heat_utils

  implicit none

  integer :: i, j, k, nx, ny, n, nmax, me, np, i1, iN, statinfo, vect_size, next, previous, narg
  integer, dimension(mpi_status_size) :: status
  real*8 :: lx, ly, dt, dx, dy, d, t, epsilon, t1, t2, temp
  real*8, dimension(:), pointer :: U => null(), F => null()
  real*8, dimension(3) :: A
  real*8,dimension(5) :: param
  character(len=32) :: arg
  integer :: comm

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

  call MPI_Comm_dup(MPI_COMM_WORLD, comm, statinfo);
  call mpi_comm_rank(comm, me, statinfo)
  call mpi_comm_size(comm, np, statinfo)
  
  t1=mpi_wtime()
  
  next = me+1
  previous = me-1
  
  if(next == np)     next=mpi_proc_null
  if(previous == -1) previous=mpi_proc_null

  call read_file(nx, ny, lx, ly, d)

  call load(me, nx*ny, Np, i1, iN)

  vect_size = in-i1+1
  dt        = 0.01
  nmax      = 100
  dx        = lx/(nx+1)
  dy        = ly/(ny+1)
  epsilon   = 0.0001

  allocate(U(in-i1+1))
  allocate(F(in-i1+1))
  call init(U, i1, iN, dx, dy, nx, lx, ly, temp)
  call filling_A(d, dx, dy, dt, nx, ny, A) ! fill A

  do n=1, nmax
    t = t + dt
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, i1, in, lx, ly, param)
    call conjgrad(A, F, U, nx, ny, epsilon, i1, in, np, me, next, previous, comm)
    print*, 'Time step:', t, '(', n, ')'
  end do

  call finalize(dx, dy, nx, ny, i1, in, u, f, me, 1)
  
  t2 = mpi_wtime()
  print*, 'Calcul time:', t2-t1, 'sec'
  
  deallocate(U, F)

  call mpi_finalize(statinfo)

end program heat
