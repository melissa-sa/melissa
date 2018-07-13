      program melissa_to_serafin
        implicit none
        integer fid, fid2, i, j, ierr
        integer(kind=4) :: tag
        character(len=144) :: file_name,file_name2
        character(len=80) :: title
        character(len=32),allocatable :: varname(:)
        character(len=2) :: id
        integer nvar,ntimestep,fsize
        integer nelem,npoin,ndp,nplan,nptfr,nptir,has_date
        integer :: date(6)
        integer, allocatable :: ipobo_knolg(:),ikle(:)
        real, allocatable :: x(:),y(:)
        real, allocatable :: time(:)
        real, allocatable :: value(:,:,:)
        integer,parameter :: is=4
        integer,parameter :: rs=4
        integer pos_data, size_data_set
        !
        file_name = "./group0/rank0/r2d_gouttedo_v1p0.slf"
        file_name2 = "sobol_WD.slf"
        !
        fid = 666
        fid2 = 667
        open(unit=fid,file=file_name,form='UNFORMATTED',iostat=ierr) 
        open(unit=fid2,file=file_name2,form='UNFORMATTED')
        !
        ! Reading file information
        !
        ! Title
        read(fid) title 
        ! Number of variable
        read(fid) nvar,j
        ! Name of the variable
        allocate(varname(nvar))
        do i=1, nvar
          varname(i) = repeat(' ',32)
          read(fid) varname(i)
          print*, "varname = '",varname(i),"'"
        enddo
        ! 10 integers
        read(fid) j,j,j,j,j,j,nplan,nptfr,nptir,has_date
        if(has_date.ne.0) then
          read(fid) date
        else
          date = (/0,0,0,0,0,0/)
        endif
        ! 4 integers nelem,npoin,ndp,1
        read(fid) nelem,npoin,ndp,j
        ! Connectivity table
        allocate(ikle(nelem*ndp))
        read(fid) ikle
        ! ipobo/knolg
        ! If in parallel we read knolg otherwise ipobo
        allocate(ipobo_knolg(npoin))
        read(fid) ipobo_knolg
        ! x and y
        allocate(x(npoin))
        allocate(y(npoin))
        read(fid) x
        read(fid) y
        ! Compute the number of time step using the size of the file
        inquire(unit=fid,size=fsize)
        pos_data = 4+80+4 ! title
     &            + 4+2*IS+4 ! NVAR
     &            + 4+nvar*32+4 ! VARNAME
     &            + 4+10*IS+4 ! 10 integers
     &            + 4+4*IS+4 ! 4 integers
     &            + 4+nelem*ndp*IS+4 ! ikle
     &            + 4+npoin*IS+4 ! ipobo/knolg
     &            + 4+npoin*RS+4 ! x 
     &            + 4+npoin*RS+4 ! y
        if(has_date.ne.0) then
          pos_data = pos_data + 4+6*is+4
        endif
        size_data_set = 4+rs+4 !time
     &                + (4+npoin*rs+4)*nvar
        ntimestep = (fsize - pos_data + 1)/size_data_set
        write(*,*) 'ntimestep',ntimestep
        allocate(time(ntimestep))
        allocate(value(ntimestep,nvar,npoin))
        do j=1,ntimestep
          ! time step value
          read(fid) time(j)
          ! Variables value
          do i=1,nvar
            read(fid) value(j,i,:)
          enddo
        enddo
        !
        close(fid)
        deallocate(varname)
        deallocate(value)
        !
        ! Writing file header
        !
        ! Title
        write(fid2) title 
        ! Number of variable
        nvar=3
        write(fid2) nvar,0
        ! Name of the variable
        write(fid2) "WATER_DEPTH_H   M               "
        write(fid2) "WATER_DEPTH_X   M               "
        write(fid2) "WATER_DEPTH_Y   M               "
        ! 10 integers
        write(fid2) 1,0,0,0,0,0,nplan,nptfr,nptir,has_date
        if(has_date.ne.0) then
          write(fid2) date
        endif
        ! 4 integers nelem,npoin,ndp,1
        write(fid2) nelem,npoin,ndp,1
        ! Connectivity table
        write(fid2) ikle
        ! ipobo/knolg
        ! If in parallel we write knolg otherwise ipobo
        write(fid2) ipobo_knolg
        ! x and y
        write(fid2) x
        write(fid2) y
        !
        allocate(value(20,nvar,npoin))
        do j=1,20
            write(fid2) time(j)
            write(id,'(I2.2)') j
            print*, 'id= ',id
            file_name = "results.WATER_DEPTH______sobol0."//id
            open(unit=fid,file=file_name,iostat=ierr
     &           , status='old', action='read') 
            do i=1,npoin
                read (fid,*) value(j,1,i)
            enddo
            close(fid)
            write(fid2) value(j,1,:)
            file_name = "results.WATER_DEPTH______sobol1."//id
            open(unit=fid,file=file_name,iostat=ierr) 
            do i=1,npoin
                read (fid,*) value(j,2,i)
            enddo
            close(fid)
            write(fid2) value(j,2,:)
            file_name = "results.WATER_DEPTH______sobol2."//id
            open(unit=fid,file=file_name,iostat=ierr) 
            do i=1,npoin
                read (fid,*) value(j,3,i)
            enddo
            close(fid)
            write(fid2) value(j,3,:)
        enddo
        !
        close(fid2)
        deallocate(x,y)
        deallocate(ikle)
        deallocate(ipobo_knolg)
        deallocate(time)
        deallocate(value)
      end program


