!*****************************************************************!
!                            Melissa                              !
!-----------------------------------------------------------------!
!   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    !
!                                                                 !
! This source is covered by the BSD 3-Clause License.             !
! Refer to the  LICENSE file for further information.             !
!                                                                 !
!-----------------------------------------------------------------!
!  Original Contributors:                                         !
!    Theophile Terraz,                                            !
!    Bruno Raffin,                                                !
!    Alejandro Ribes,                                             !
!    Bertrand Iooss,                                              !
!*****************************************************************!


interface

subroutine melissa_init(field_name,&
                        local_vect_size,&
                        comm) bind(c, name = 'melissa_init_f')
    use ISO_C_BINDING, only: C_INT, C_CHAR
    character(kind=C_CHAR),dimension(*) :: field_name
    integer(kind=C_INT) :: local_vect_size
    integer(kind=C_INT) :: comm
end subroutine melissa_init

subroutine melissa_send(field_name,&
                        send_vect) bind(c, name = 'melissa_send')
    use ISO_C_BINDING, only: C_INT, C_DOUBLE, C_CHAR
    character(kind=C_CHAR),dimension(*) :: field_name
    real(kind=C_DOUBLE),dimension(*)    :: send_vect
end subroutine melissa_send

subroutine melissa_finalize() bind(c, name = 'melissa_finalize')
end subroutine melissa_finalize

end interface

integer, parameter :: MELISSA_COUPLING_NONE = 0
integer, parameter :: MELISSA_COUPLING_ZMQ = 0
integer, parameter :: MELISSA_COUPLING_DEFAULT = MELISSA_COUPLING_ZMQ
integer, parameter :: MELISSA_COUPLING_MPI = 1
integer, parameter :: MELISSA_COUPLING_FLOWVR = 2
