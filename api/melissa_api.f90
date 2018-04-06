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


interface

subroutine melissa_init(field_name,&
                        local_vect_size,&
                        comm_size,&
                        rank,&
                        simu_id,&
                        comm,&
                        coupling) bind(c, name = 'melissa_init_f')
    use ISO_C_BINDING, only: C_INT, C_CHAR
    character(kind=C_CHAR),dimension(*) :: field_name
    integer(kind=C_INT) :: local_vect_size
    integer(kind=C_INT) :: comm_size
    integer(kind=C_INT) :: rank
    integer(kind=C_INT) :: simu_id
    integer(kind=C_INT) :: comm
    integer(kind=C_INT) :: coupling
end subroutine melissa_init

subroutine melissa_init_no_mpi(field_name,&
                               vect_size,&
                               simu_id) bind(c, name = 'melissa_init_no_mpi')
    use ISO_C_BINDING, only: C_INT, C_CHAR
    character(kind=C_CHAR),dimension(*) :: field_name
    integer(kind=C_INT) :: vect_size
    integer(kind=C_INT) :: simu_id
end subroutine melissa_init_no_mpi

subroutine melissa_send(time_step,&
                        field_name,&
                        send_vect,&
                        rank,&
                        simu_id) bind(c, name = 'melissa_send')
    use ISO_C_BINDING, only: C_INT, C_DOUBLE, C_CHAR
    integer(kind=C_INT)                 :: time_step
    character(kind=C_CHAR),dimension(*) :: field_name
    real(kind=C_DOUBLE),dimension(*)    :: send_vect
    integer(kind=C_INT)                 :: rank
    integer(kind=C_INT)                 :: simu_id
end subroutine melissa_send

subroutine melissa_send_no_mpi(time_step,&
                               field_name,&
                               send_vect,&
                               simu_id) bind(c, name = 'melissa_send_no_mpi')
    use ISO_C_BINDING, only: C_INT, C_DOUBLE, C_CHAR
    integer(kind=C_INT)                 :: time_step
    character(kind=C_CHAR),dimension(*) :: field_name
    real(kind=C_DOUBLE),dimension(*)    :: send_vect
    integer(kind=C_INT)                 :: simu_id
    end subroutine melissa_send_no_mpi

subroutine melissa_finalize() bind(c, name = 'melissa_finalize')
end subroutine melissa_finalize

end interface

integer, parameter :: MELISSA_COUPLING_NONE = 0
integer, parameter :: MELISSA_COUPLING_ZMQ = 0
integer, parameter :: MELISSA_COUPLING_DEFAULT = MELISSA_COUPLING_ZMQ
integer, parameter :: MELISSA_COUPLING_MPI = 1
integer, parameter :: MELISSA_COUPLING_FLOWVR = 2
