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


      INTERFACE

      SUBROUTINE MELISSA_INIT(FIELD_NAME,
     & LOCAL_VECT_SIZE,
     & COMM)
     & BIND(C, NAME = 'melissa_init_f')
          USE ISO_C_BINDING, ONLY: C_INT, C_CHAR
          CHARACTER(KIND=C_CHAR),DIMENSION(*) :: FIELD_NAME
          INTEGER(KIND=C_INT) :: LOCAL_VECT_SIZE
          INTEGER(KIND=C_INT) :: COMM
      END SUBROUTINE MELISSA_INIT

      SUBROUTINE MELISSA_INIT_NO_MPI(FIELD_NAME,
     & VECT_SIZE)
     & BIND(C, NAME = 'melissa_init_no_mpi_f')
          USE ISO_C_BINDING, ONLY: C_INT, C_CHAR
          CHARACTER(KIND=C_CHAR),DIMENSION(*) :: FIELD_NAME
          INTEGER(KIND=C_INT) :: VECT_SIZE
      END SUBROUTINE MELISSA_INIT_NO_MPI

      SUBROUTINE MELISSA_SEND(FIELD_NAME,
     & SEND_VECT)
     & BIND(C, NAME = 'melissa_send')
          USE ISO_C_BINDING, ONLY: C_INT, C_DOUBLE, C_CHAR
          CHARACTER(KIND=C_CHAR),DIMENSION(*) :: FIELD_NAME
          REAL(KIND=C_DOUBLE),DIMENSION(*)    :: SEND_VECT
      END SUBROUTINE MELISSA_SEND

      SUBROUTINE MELISSA_SEND_NO_MPI(FIELD_NAME,
     & SEND_VECT)
     & BIND(C, NAME = 'melissa_send_no_mpi')
          USE ISO_C_BINDING, ONLY: C_INT, C_DOUBLE, C_CHAR
          CHARACTER(KIND=C_CHAR),DIMENSION(*) :: FIELD_NAME
          REAL(KIND=C_DOUBLE),DIMENSION(*)    :: SEND_VECT
          END SUBROUTINE MELISSA_SEND_NO_MPI

      SUBROUTINE MELISSA_FINALIZE() BIND(C, NAME = 'melissa_finalize')
      END SUBROUTINE MELISSA_FINALIZE

      END INTERFACE

      INTEGER, PARAMETER :: MELISSA_COUPLING_NONE = 0
      INTEGER, PARAMETER :: MELISSA_COUPLING_ZMQ = 0
      INTEGER, PARAMETER :: MELISSA_COUPLING_DEFAULT = 0
      INTEGER, PARAMETER :: MELISSA_COUPLING_MPI = 1
      INTEGER, PARAMETER :: MELISSA_COUPLING_FLOWVR = 2
