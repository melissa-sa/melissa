 
program heat

  use heat_utils
  use mpi
  use stats_api_fortran
  
  implicit none

  integer::i,j,k,nx,ny,n,nmax,me,np,i1,iN,statinfo,nb_op,next,previous
  integer,dimension(mpi_status_size)::status
  real*8::lx,ly,dt,dx,dy,d,t,epsilon,t1,t2,temp
  real*8,dimension(:),pointer::U => null(), F => null()
  real*8,dimension(3)::A
  character(len=32)::arg
  integer,dimension(:),allocatable :: parameters;
  integer :: comm
  character(len=6) name

  name = C_CHAR_"heat"//C_NULL_CHAR

  call mpi_init(statinfo)
  call mpi_comm_rank(mpi_comm_world,me,statinfo)
  call mpi_comm_size(mpi_comm_world,np,statinfo)
  comm = MPI_COMM_WORLD;

  call getarg(1, arg)
  read( arg, * ) temp ! initial temperature
  allocate(parameters(1))
  parameters(1) = temp
  
  t1=mpi_wtime()
  
  next=me+1
  previous=me-1
  
  if(next==np)     next=mpi_proc_null
  if(previous==-1) previous=mpi_proc_null

  call read_file(nx,ny,lx,ly,d)

  call load(me, nx*ny, Np, i1, iN)

  nb_op   = in-i1+1
  dt      = 0.01
  nmax    = 100
  dx      = lx/(nx+1)
  dy      = ly/(ny+1)
  epsilon = 0.0001

  allocate(U(in-i1+1))
  allocate(F(in-i1+1))
  call init(U,i1,iN,dx,dy,nx,lx,ly,temp)
  call filling_A(d,dx,dy,dt,nx,ny,A) ! fill A

  i = 1
  n = nx*ny
  call connect_to_stats (i, nb_op, np, me, MPI_COMM_WORLD)

  do n=1,nmax
    t = t + dt
    call filling_F(nx,ny,U,d,dx,dy,dt,t,F,i1,in,lx,ly)
    call conjgrad(A,F,U,nx,ny,epsilon,i1,in,np,me,next,previous)

    call send_to_stats (n, parameters, i, name, u, me)
  end do

  call finalize(dx,dy,nx,ny,i1,in,u,f,me)

  call disconnect_from_stats ()
  
  t2=mpi_wtime()
  print*,'Calcul time:', t2-t1, 'sec'
  print*,'Final time step:', t
  
  deallocate(U, F, parameters)

  call mpi_finalize(statinfo)

end program heat
