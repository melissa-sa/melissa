!*****************************************************************!
!                            Melissa                              !
!-----------------------------------------------------------------!
!   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    !
!                                                                 !
! This source is covered by the BSD 3-Clause License.             !
! Refer to the  LICENCE file for further information.             !
!                                                                 !
!-----------------------------------------------------------------!
!  Original Contributors:                                         !
!    Theophile Terraz,                                            !
!    Bruno Raffin,                                                !
!    Alejandro Ribes,                                             !
!    Bertrand Iooss,                                              !
!*****************************************************************!

 
program heat

  use heat_utils
  use iso_c_binding, only: C_CHAR, C_NULL_CHAR

  implicit none

  include "melissa_api.f90"

  integer :: i, j, k, nx, ny, n, nmax, me, np, i1, iN, statinfo, vect_size, next, previous, narg
  integer, dimension(mpi_status_size) :: status
  real*8 :: lx, ly, dt, dx, dy, d, t, epsilon, t1, t2
  real*8, dimension(:), pointer :: U => null(), F => null()
  real*8, dimension(3) :: A
  real*8,dimension(5) :: param
  character(len=32) :: arg
  integer :: comm
  integer(kind=MPI_ADDRESS_KIND) :: appnum
  character(len=5) :: name = C_CHAR_"heat1"  !C_NULL_CHAR

  call mpi_init(statinfo)

  ! The program now takes at least 2 parameter:
  ! - The coupling method for Sobol' groups
  ! - the initial temperature

  narg = iargc()
  param(:) = 0
  if (narg .lt. 2) then
    print*,"Missing parameter"
    stop
  endif

  param(:) = 0.

  ! The initial temperature is stored in param(1)
  ! The four next optional parameters are the boundary temperatures
  do n=1, 5
    if(narg .ge. n) then
      call getarg(n, arg)
      read( arg, * ) param(n)
    endif
  enddo

  ! The new MPI communicator is build by splitting MPI_COMM_WORLD by simulation inside the group.
  ! In the case of a single simulation group, this is equivalent to MPI_Comm_dup.
  call mpi_comm_rank(MPI_COMM_WORLD, me, statinfo)
  call MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, appnum, statinfo);
  call MPI_Comm_split(MPI_COMM_WORLD, appnum, me, comm, statinfo);
  call mpi_comm_rank(comm, me, statinfo)
  call mpi_comm_size(comm, np, statinfo)

  ! Init timer
  t1=mpi_wtime()

  ! Neighbour ranks
  next = me+1
  previous = me-1
  
  if(next == np)     next=mpi_proc_null
  if(previous == -1) previous=mpi_proc_null

  nx        = 100 ! x axis grid subdivisions
  ny        = 100 ! y axis grid subdivisions
  lx        = 10.0 ! x length
  ly        = 10.0 ! y length
  d         = 1.0 ! diffusion coefficient
  dt        = 0.01 ! timestep value
  nmax      = 100 ! number of timesteps
  dx        = lx/(nx+1) ! x axis step
  dy        = ly/(ny+1) ! y axis step
  epsilon   = 0.0001 ! conjugated gradient precision
  n         = nx*ny ! number of cells in the drid

  ! work repartition over the MPI processes
  ! i1 and in: first and last global cell indices atributed to this process
  call load(me, nx*ny, Np, i1, iN)

  ! local number of cells
  vect_size = in-i1+1

  ! initialization
  allocate(U(in-i1+1))
  allocate(F(in-i1+1))
  ! we will solve Au=F
  call init(U, i1, iN, dx, dy, nx, lx, ly, param(1))
  ! init A (tridiagonal matrix):
  call filling_A(d, dx, dy, dt, nx, ny, A) ! fill A

  ! melissa_init is the first Melissa function to call, and it is called only once by each process in comm.
  ! It mainly contacts the server.
  call melissa_init (name, vect_size, comm)

  ! main loop:
  do n=1, nmax
    t = t + dt
    ! filling F (RHS) before each iteration:
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, i1, in, lx, ly, param)
    ! conjugated gradient to solve Au = F.
    call conjgrad(A, F, U, nx, ny, epsilon, i1, in, np, me, next, previous, comm)
    ! The result is u
    ! melissa_send is called at each iteration to send u to the server.
    call melissa_send(name, u)
  end do

  ! melissa_finalize closes the connexion with the server.
  ! No Melissa function should be called after melissa_finalize.
  call melissa_finalize ()
  
  t2 = mpi_wtime()
  print*, 'Calcul time:', t2-t1, 'sec'
  print*, 'Final time step:', t
  
  deallocate(U, F)

  call mpi_finalize(statinfo)

end program heat
