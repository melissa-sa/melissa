module melissa_api_fortran

  use ISO_C_BINDING

  interface

  subroutine melissa_init(local_vect_size,&
                          comm_size,&
                          rank,&
                          sobol_rank,&
                          sobol_group,&
                          comm,&
                          coupling) bind(c, name = 'melissa_init_f')
    import C_INT
    integer(kind=C_INT) :: local_vect_size
    integer(kind=C_INT) :: comm_size
    integer(kind=C_INT) :: rank
    integer(kind=C_INT) :: sobol_rank
    integer(kind=C_INT) :: sobol_group
    integer(kind=C_INT) :: comm
    integer(kind=C_INT) :: coupling
  end subroutine melissa_init

  subroutine melissa_init_no_mpi(vect_size,&
                                 sobol_rank,&
                                 sobol_group) bind(c, name = 'melissa_init_no_mpi')
    import C_INT
    integer(kind=C_INT) :: vect_size
    integer(kind=C_INT) :: sobol_rank
    integer(kind=C_INT) :: sobol_group
  end subroutine melissa_init_no_mpi

  subroutine melissa_send(time_step,&
                          field_name,&
                          send_vect,&
                          rank,&
                          sobol_rank,&
                          sobol_group) bind(c, name = 'melissa_send')
    import C_INT,C_DOUBLE,C_CHAR
    integer(kind=C_INT)                 :: time_step
    character(kind=C_CHAR),dimension(*) :: field_name
    real(kind=C_DOUBLE),dimension(*)    :: send_vect
    integer(kind=C_INT)                 :: rank
    integer(kind=C_INT)                 :: sobol_rank
    integer(kind=C_INT)                 :: sobol_group
  end subroutine melissa_send

  subroutine melissa_send_no_mpi(time_step,&
                                 field_name,&
                                 send_vect,&
                                 sobol_rank,&
                                 sobol_group) bind(c, name = 'melissa_send_no_mpi')
    import C_INT,C_DOUBLE,C_CHAR
    integer(kind=C_INT)                 :: time_step
    character(kind=C_CHAR),dimension(*) :: field_name
    real(kind=C_DOUBLE),dimension(*)    :: send_vect
    integer(kind=C_INT)                 :: sobol_rank
    integer(kind=C_INT)                 :: sobol_group
    end subroutine melissa_send_no_mpi

  subroutine melissa_finalize() bind(c, name = 'melissa_finalize')
  end subroutine melissa_finalize

  end interface

end module
