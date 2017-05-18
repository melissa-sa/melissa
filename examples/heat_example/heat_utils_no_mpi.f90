
module heat_utils_no_mpi
  use iso_c_binding

  implicit none

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

    integer :: num, nx
    integer :: invert_num_j
    invert_num_j = floor((num-1)/real(nx))+1

  end function
  
  function invert_num_i(nx, num)

    implicit none

    integer :: num, j, nx
    integer :: invert_num_i
    j = invert_num_j(nx, num)
    if (num-(j-1)*nx  == 0) then
      invert_num_i = nx
    else
      invert_num_i = num-(j-1)*nx
    end if

  end function

!-------------------------subroutines

  subroutine read_file(nx, ny, lx, ly, d) bind (c, name = 'read_file')

    implicit none

    integer :: nx, ny
    real*8  :: lx, ly, d

    open(unit = 12, file = 'data.txt', action = 'read')
    read(12, *) nx
    read(12, *) ny
    read(12, *) lx
    read(12, *) ly
    read(12, *) d
    close(12)

  end subroutine read_file

  subroutine init(u0, nb_op, temp) bind (c, name = 'init')

    implicit none

    real*8, dimension(*) :: u0
    real*8, intent(in) :: temp
    integer, intent(in) :: nb_op

    u0(1:nb_op) = temp

  end subroutine init
  
  subroutine filling_A(d, dx, dy, dt, a) bind (c, name = 'filling_A') ! pour remplir A

    implicit none

    real*8, intent(in) :: d, dx, dy, dt
    real*8, dimension(3), intent(out) :: a
    
    a(1) = -d*dt/(dx**2)
    a(2) = 1.D0+2.D0*d*dt*(1.D0/dx**2+1.D0/dy**2)
    a(3) = -d*dt/(dy**2)
    
  end subroutine filling_A
  
  subroutine filling_F(nx, ny, u, d, dx, dy, dt, t, f, nb_op, lx, ly, param) bind (c, name = 'filling_F')

    implicit none

    integer, intent(in) :: nx, ny, nb_op
    real*8, dimension(*) :: f
    real*8, dimension(*) :: u
    real*8, dimension(*) :: param
    integer :: i, j, k
    real*8, intent(in) :: d, dx, dy, dt, t, lx, ly

    do k = 1, nb_op
      f(k) = 0
      i = invert_num_i(nx, k)
      j = invert_num_j(nx, k)
      f(k) = f(k)+func(i*dx, j*dy, t, lx, ly)*dt+u(k)
      if (i  == 1) then
        f(k) = f(k)+d*dt*h(0.d0, j*dy, t, param(2))/(dx**2)
      else if (i  == nx) then
        f(k) = f(k)+d*dt*h((i+1)*dx, j*dy, t, param(3))/(dx**2)
      end if
      if (j  == 1) then
        f(k) = f(k)+d*dt*g(i*dx, 0.d0, t, param(4))/(dy**2)
      else if (j  == ny) then
        f(k) = f(k)+d*dt*g(i*dx, (j+1)*dy, t, param(5))/(dy**2)
      end if
    end do
  
  end subroutine filling_F

  subroutine rename(name, simu_number)! bind (c, name = 'rename')

    implicit none

    character*13 :: name
    integer :: simu_number
    character*5 :: sn
    integer :: i1, i2, i3, i4, i5

    i1 = simu_number/10000
    i2 = (simu_number - 10000*i1)/1000
    i3 = (simu_number - 10000*i1 - 1000*i2)/100
    i4 = (simu_number - 10000*i1 - 1000*i2 - 100*i3)/10
    i5 = simu_number - 10000*i1 -1000*i2 - 100*i3 - 10*i4
    sn = char(i1+48)//char(i2+48)//char(i3+48)//char(i4+48)//char(i5+48)
    name = 'sol'//sn//'.dat'

  end subroutine rename

  subroutine finalize(dx, dy, nx, ny, nb_op, u, f, simu_number) bind (c, name = 'finalize')

    implicit none

    real*8, dimension(*), intent(inout) :: u, f
    integer, intent(in) :: nx, ny, nb_op, simu_number
    real*8, intent(in) :: dx, dy
    integer :: i, j
    character*13 :: name

    call rename(name, simu_number)

    open(unit = 13, file = name, action = 'write')
    do j = 1, nb_op
      write(13, *), dx*invert_num_i(nx, j), dy*invert_num_j(nx, j), u(j)
    end do
    close(13)

  end subroutine finalize
  
!------------------------- CG subroutines

  subroutine ConjGrad(Ah, F, U, nx, ny, epsilon) bind (c, name = 'conjgrad')

    implicit none

    real*8, dimension(3) :: Ah
    real*8, dimension(*) :: F, U
    real*8, dimension(:), pointer :: R, P, Q
    integer :: i, k, nx, ny, n
    real*8, intent(in) :: epsilon
    real*8 :: rho, rho_, beta, delta, a

    n = nx*ny
    allocate(R(n), P(n), Q(n))
    do i = 1, n
       U(i) = 0.0D0
       R(i) = -F(i)
    end do

    i = 1
    rho = scal(R, R, n)

    do while(norm(R, n)/norm(F, n) > epsilon)
       rho_ = rho
       rho = scal(R, R, n)
       if(i   ==  1) then
          do k = 1, n
             P(k) = R(k)
          end do
       else
          beta = rho/rho_
          do k = 1, n
             P(k) = beta*P(k)+R(k)
          end do
       end if
       call multiply(ah, p, q, nx, ny)
       q = -q
       delta = scal(P, Q, n)
       a = rho/delta
       do k = 1, n
          U(k) = U(k)+a*P(k)
          R(k) = R(k)-a*Q(k)
       end do
       i = i+1
    end do

    deallocate(R, P, Q)

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

  function norm(X, n) result(y)

    real*8 :: y
    integer :: n
    real*8, dimension(*) :: X

    y = sqrt(scal(X, X, n))

  end function norm

  subroutine multiply(a, x, y, nx, ny) !Pour le produit Ax = y

    implicit none

    real*8, dimension(3) :: a
    real*8, dimension(:), pointer, intent(in) :: x
    integer, intent(in) :: nx, ny
    real*8, dimension(:), pointer, intent(out) :: y
    integer :: i, j

    y = x*a(2)
    do j = 1, ny
      do i = 1, nx
        if ((i /= 1) .and. (i /= nx)) then
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i-1, j, nx))*a(1)+x(num(i+1, j, nx))*a(1)
        else if (i  == 1) then
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i+1, j, nx))*a(1)
        else
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i-1, j, nx))*a(1)
        end if
        if ((j /= 1) .and. (j /= ny)) then
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i, j+1, nx))*a(3)+x(num(i, j-1, nx))*a(3)
        else if (j  == 1) then
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i, j+1, nx))*a(3)
        else
          y(num(i, j, nx)) = y(num(i, j, nx))+x(num(i, j-1, nx))*a(3)
        end if
      end do
    end do

  end subroutine multiply

end module heat_utils_no_mpi
