
module heat_utils

  implicit none

  include "mpif.h"

  contains

!-------------------------functions

  function func(x, y, t, lx, ly)

    implicit none

    real*8 :: func, x, y, t, lx, ly
    !func = 2*(y-y**2+x-x**2)
    !func = sin(x)+cos(y)
    func = 0.d0
    !func = (exp(-(x-lx/2.)**2))*(exp(-(y-ly/2.)**2))*cos(t*acos(0.0)/2)

  end function

  function g(x, y, t, param)

    implicit none

    real*8 :: g, x, y, t, param
    !g = 0.
    !g = sin(x)+cos(y)
    g = param

  end function

  function h(x, y, t, param)

    implicit none

    real*8 :: h, x, y, t, param
    !h = 0.
    !h = sin(x)+cos(y)
    h = param

   end function
   
  function num(i, j, nx)

    implicit none

    integer :: num, i, j, nx
    num = i+(j-1)*nx

  end function
  
  function invert_num_j(nx, num)

    implicit none

    integer :: num, nx
    integer :: invert_num_j
    invert_num_j = floor((num-1)/real(nx))+1

  end function
  
  function invert_num_i(nx, num)

    implicit none

    integer :: num, j, nx
    integer :: invert_num_i
    j = invert_num_j(nx, num)
    if (num-(j-1)*nx == 0) then
      invert_num_i = nx
    else
      invert_num_i = num-(j-1)*nx
    end if

  end function

!-------------------------subroutines

  subroutine load(me, n, Np, i1, iN) bind (c, name = 'load')

    implicit none

    integer :: me, n, Np, i1, iN
    integer :: card, surplus

    card = n/Np
    surplus = mod(n, Np)

    if(me < surplus) then
       i1 = me*card+me+1
       iN = i1+card
    else
       i1 = me*card+surplus+1
       iN = i1+card-1
    end if

  end subroutine load
  
  subroutine filling_A(d, dx, dy, dt, nx, ny, a) bind (c, name = 'filling_A') ! pour remplir A

    implicit none

    real*8, intent(in) :: d, dx, dy, dt
    integer, intent(in) :: nx, ny
    real*8, dimension(3), intent(out) :: a
    
    a(1) = -d*dt/(dx**2)
    a(2) = 1.D0+2.D0*d*dt*(1.D0/dx**2+1.D0/dy**2)
    a(3) = -d*dt/(dy**2)
    
  end subroutine filling_A
  
  subroutine filling_F(nx, ny, u, d, dx, dy, dt, t, f, i1, in, lx, ly, param) bind (c, name = 'filling_F')

    implicit none

    integer, intent(in) :: nx, ny, i1, in
    real*8, dimension(*) :: f
    real*8, dimension(*) :: u
    real*8, dimension(*) :: param
    integer :: i, j, k
    real*8, intent(in) :: d, dx, dy, dt, t, lx, ly

    do k = 1, in-i1+1
      f(k) = 0
      i = invert_num_i(nx, i1+k-1)
      j = invert_num_j(nx, i1+k-1)
      f(k) = f(k)+func(i*dx, j*dy, t, lx, ly)*dt+u(k)
      if (i == 1) then
        f(k) = f(k)+d*dt*h(0.d0, j*dy, t, param(2))/(dx**2)
      else if (i == nx) then
        f(k) = f(k)+d*dt*h((i+1)*dx, j*dy, t, param(3))/(dx**2)
      end if
      if (j == 1) then
        f(k) = f(k)+d*dt*g(i*dx, 0.d0, t, param(4))/(dy**2)
      else if (j == ny) then
        f(k) = f(k)+d*dt*g(i*dx, (j+1)*dy, t, param(5))/(dy**2)
      end if
    end do
  
  end subroutine filling_F

  subroutine rename(Me, name, simu_number)! bind (c, name = 'rename')

    implicit none

    integer :: Me, simu_number
    character*32  :: name
    character*3 :: tn
    character*5 :: sn
    integer :: i1, i2, i3, i4, i5
    i1 = Me/100
    i2  = ( Me - 100*i1)/10
    i3 = Me - 100*i1 -10*i2
    tn = char(i1+48)//char(i2+48)//char(i3+48)
    i1 = simu_number/10000
    i2 = (simu_number - 10000*i1)/1000
    i3 = (simu_number - 10000*i1 - 1000*i2)/100
    i4 = (simu_number - 10000*i1 - 1000*i2 - 100*i3)/10
    i5 = simu_number - 10000*i1 -1000*i2 - 100*i3 - 10*i4
    sn = char(i1+48)//char(i2+48)//char(i3+48)//char(i4+48)//char(i5+48)
    name = 'sol'//tn//'_'//sn//'.dat'

  end subroutine rename
  
  subroutine init(u0, i1, iN, dx, dy, nx, lx, ly, temp) bind (c, name = 'init')

    implicit none

    real*8, dimension(*) :: u0
    real*8, intent(in) :: dx, dy, lx, ly, temp
    integer, intent(in) :: i1, in, nx
    integer :: i
    do i = i1, in
      u0(i-i1+1) = temp
      !u0(i-i1+1) = func(invert_num_i(nx, i)*dx, invert_num_j(nx, i)*dy, 0.D0, lx, ly)
    end do

  end subroutine init

  subroutine finalize(dx, dy, nx, ny, i1, in, u, f, me, simu_number) bind (c, name = 'finalize')

    implicit none

    real*8, dimension(*), intent(inout) :: u, f
    integer, intent(in) :: nx, ny, i1, in, me, simu_number
    real*8, intent(in) :: dx, dy
    integer :: i, j
    character*32 :: name

    call rename(me, name, simu_number)

    open(unit = 13, file = name, action = 'write')
    do j = i1, in
      write(13, *), dx*invert_num_i(nx, j), dy*invert_num_j(nx, j), u(j-i1+1)
    end do
    close(13)

  end subroutine finalize
  
!------------------------- CG subroutines

  subroutine ConjGrad(Ah, F, U, nx, ny, epsilon, i1, in, np, me, next, previous, mpi_comm) bind (c, name = 'conjgrad')

    implicit none

    real*8, dimension(3) :: Ah
    real*8, dimension(:), pointer :: x1, x2
    real*8, dimension(*) :: F, U
    real*8, dimension(:), pointer :: R, P, Q, T, T1
    integer :: i, k, nx, ny, i1, in, charge, np, statinfo, me, next, previous
    real*8, intent(in) :: epsilon
    real*8 :: rho, rho_, beta, delta, a, residu, residu1, residu2
    integer, intent(in) :: mpi_comm
    integer, dimension(mpi_status_size) :: status

    charge = in-i1+1
    allocate(R(charge), P(charge), Q(charge))
    allocate(x1(nx), x2(nx))
    
    residu = 1.
    x1 = 0.0
    x2 = 0.0
    do i = 1, charge
       U(i) = 0.0
       R(i) = -F(i)
    end do
    
    i = 1
    call mpi_allreduce(scal(r , r , charge), rho, 1, mpi_real8, mpi_sum, mpi_comm, statinfo)
    
    do while(residu > epsilon)
       rho_ = rho
       call mpi_allreduce(scal(r, r, charge), rho, 1, mpi_real8, mpi_sum, mpi_comm, statinfo)
       if(i  ==  1) then
          do k = 1, charge
             P(k) = R(k)
          end do
       else
          beta = rho/rho_
          do k = 1, charge
             P(k) = beta*P(k)+R(k)
          end do
       end if
       
          call mpi_send(p(1:nx), nx, mpi_real8, previous, 999, mpi_comm, statinfo)
          call mpi_send(p(charge-nx+1:charge), nx, mpi_real8, next, 888, mpi_comm, statinfo)
          call mpi_recv(x1, nx, mpi_real8, previous, 888, mpi_comm, status, statinfo)
          call mpi_recv(x2, nx, mpi_real8, next, 999, mpi_comm, status, statinfo)

       call multiply(ah, p, q, nx, ny, i1, in, x1, x2)
       q = -q
       call mpi_allreduce(scal(p, q, charge), delta, 1, mpi_real8, mpi_sum, mpi_comm, statinfo)
       a = rho/delta
       do k = 1, charge
          U(k) = U(k)+a*P(k)
          R(k) = R(k)-a*Q(k)
       end do
       i = i+1
       call mpi_allreduce(scal(r, r, charge), residu1, 1, mpi_real8, mpi_sum, mpi_comm, statinfo)
       call mpi_allreduce(scal(f, f, charge), residu2, 1, mpi_real8, mpi_sum, mpi_comm, statinfo)
       residu = sqrt(residu1)/sqrt(residu2)
      
    end do
    
    deallocate(R, P, Q)
    deallocate(x1, x2)

  end subroutine ConjGrad

  function scal(X, Y, n) result(z)

    implicit none

    real*8 :: z
    real*8, dimension(*) :: X, Y
    integer :: i, n

    z = 0.0

    do i = 1, n
       z = z+X(i)*Y(i)
    end do

  end function scal
  
  subroutine multiply(a, x, y, nx, ny, i1, in, x1, x2) ! Ax = y

    implicit none

    real*8, dimension(3) :: a
    real*8, dimension(:), pointer, intent(in) :: x, x1, x2
    integer, intent(in) :: nx, ny
    real*8, dimension(:), pointer, intent(out) :: y
    integer :: j, i, k, l, m, i1, in
    
    l = 1
    m = 1
    do k = 1, size(y)
      i = invert_num_i(nx, (i1+k-1))
      j = invert_num_j(nx, (i1+k-1))
      y(k) = x(k)*a(2)
      if ((i /= 1) .and. (i /= nx)) then
        if (k == 1) then
          y(k) = y(k)+x1(nx)*a(1)+x(k+1)*a(1)
        else if (k == size(y)) then
          y(k) = y(k)+x(k-1)*a(1)+x2(1)*a(1)
	else
          y(k) = y(k)+x(k-1)*a(1)+x(k+1)*a(1)
	end if
      else if (i == 1) then
        if (k == size(y)) then
          y(k) = y(k)+x(k+1)*a(1)+x2(1)*a(1)
	else
          y(k) = y(k)+x(k+1)*a(1)
	end if
      else
        y(k) = y(k)+x(k-1)*a(1)
      end if
      if ((j /= 1) .and. (j /= ny)) then
        if (k <= nx) then
          y(k) = y(k)+x(k+nx)*a(3)+x1(l)*a(3)
          l = l+1
        else if (k >= (size(y)-nx+1)) then
          y(k) = y(k)+x2(m)*a(3)+x(k-nx)*a(3)
          m = m+1
	else
          y(k) = y(k)+x(k+nx)*a(3)+x(k-nx)*a(3)
	end if
      else if (j == 1) then
          y(k) = y(k)+x(k+nx)*a(3)
      else
          y(k) = y(k)+x(k-nx)*a(3)
      end if
    end do

  end subroutine multiply

end module heat_utils
