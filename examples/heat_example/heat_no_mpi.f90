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
  integer ::  simu_id = 0
  character(len=5) :: name = C_CHAR_"heat"//C_NULL_CHAR

  ! The program now takes at least 3 parameter:
  ! - the simulation id given by the launcher
  ! - the initial temperature

  narg = iargc()
  if (narg .lt. 3) then
    print*,"Missing parameter"
    stop
  endif

  param(:) = 0
  call getarg(1, arg)
  read( arg, * ) simu_id ! simulation id
  call getarg(2, arg)
  read( arg, * ) param(1) ! initial temperature
  print*, arg

  ! The four next optional parameters are the boundary temperatures
  do n=4, 7
    if(narg .ge. n) then
      call getarg(n-1, arg)
      read( arg, * ) param(n-3)
    endif
  enddo

  ! Init timer
  call cpu_time(t1)

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
  vect_size = nx*ny ! number of cells in the drid

  ! initialization
  allocate(U(vect_size))
  allocate(F(vect_size))
  ! we will solve Au=F
  call init(U, vect_size, param(1))
  ! init A (tridiagonal matrix):
  call filling_A(d, dx, dy, dt, A) ! fill A

  ! melissa_init_no_mpi is the first Melissa function to call, and it is called only once.
  ! It mainly contacts the server.
  call melissa_init_no_mpi(name, vect_size, simu_id)

  ! main loop:
  do n=1, nmax
    t = t + dt
    ! filling F (RHS) before each iteration:
    call filling_F(nx, ny, U, d, dx, dy, dt, t, F, vect_size, lx, ly, param)
    ! conjugated gradient to solve Au = F.
    call conjgrad(A, F, U, nx, ny, epsilon)
    ! The result is u
    ! melissa_send_no_mpi is called at each iteration to send u to the server.
    call melissa_send_no_mpi(n, name, u, simu_id)
  end do

  ! write results on disk
  call finalize(dx, dy, nx, ny, vect_size, u, f, simu_id)

  ! melissa_finalize closes the connexion with the server.
  ! No Melissa function should be called after melissa_finalize.
  call melissa_finalize()

  ! end timer
  call cpu_time(t2)
  print*,'Calcul time:', t2-t1, 'sec'
  print*,'Final time step:', t
  
  deallocate(U, F)

end program heat_no_mpi
