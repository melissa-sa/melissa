module stats_api_fortran

  use ISO_C_BINDING

  interface

  subroutine connect_to_stats(nb_parameters,&
                              local_vect_size,&
                              comm_size,&
                              rank,&
                              comm) bind(c, name = 'connect_from_fortran')
    import C_INT
    integer(kind=C_INT) :: nb_parameters
    integer(kind=C_INT) :: local_vect_size
    integer(kind=C_INT) :: comm_size
    integer(kind=C_INT) :: rank
    integer(kind=C_INT) :: comm
  end subroutine connect_to_stats

  subroutine connect_to_stats_no_mpi(nb_parameters,&
                                     local_vect_size,&
                                     comm_size,&
                                     rank) bind(c, name = 'connect_to_stats_no_mpi')
    import C_INT
    integer(kind=C_INT) :: nb_parameters
    integer(kind=C_INT) :: local_vect_size
    integer(kind=C_INT) :: comm_size
    integer(kind=C_INT) :: rank
  end subroutine connect_to_stats_no_mpi

  subroutine send_to_stats(time_step,&
                           parameters,&
                           nb_parameters,&
                           field_name,&
                           send_vect,&
                           rank) bind(c, name = 'send_to_stats')
    import C_INT,C_DOUBLE,C_CHAR
    integer(kind=C_INT)              :: time_step
    integer(kind=C_INT),dimension(*) :: parameters
    integer(kind=C_INT)              :: nb_parameters
    character(kind=C_CHAR),dimension(*)   :: field_name
    real(kind=C_DOUBLE),dimension(*) :: send_vect
    integer(kind=C_INT)              :: rank
  end subroutine send_to_stats

  subroutine send_to_stats_no_mpi(time_step,&
                                  parameters,&
                                  nb_parameters,&
                                  field_name,&
                                  send_vect) bind(c, name = 'send_to_stats_no_mpi')
    import C_INT,C_DOUBLE,C_CHAR
    integer(kind=C_INT)              :: time_step
    integer(kind=C_INT),dimension(*) :: parameters
    integer(kind=C_INT)              :: nb_parameters
    character(kind=C_CHAR),dimension(*)   :: field_name
    real(kind=C_DOUBLE),dimension(*) :: send_vect
    end subroutine send_to_stats_no_mpi

  subroutine disconnect_from_stats() bind(c, name = 'disconnect_from_stats')
  end subroutine disconnect_from_stats

  end interface

end module
