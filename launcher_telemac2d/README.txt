 
Launcher Telemac2D

The example is a custom gouttedo from Telemac2D examples.
Telemac2D must be slightly modified in order to work with Melissa.

In systel.cfg, add the path to melissa_api.so and zmq.so in libs_all. Don't forget to put them in your LD_LIBRARY_PATH.

In source/utils/special/declaration_special.F, add two global parameters (line 119):

      LOGICAL            :: MELISSA = .FALSE.
      INTEGER            :: SIMU_ID = 0

In sources/utils/bief/write_data.f include melissa_api.f and use ISO_C_BINDINGS.
Between "CALL CHECK_CALL" and "FIRST_VAR = .FALSE.", add:

            CALL BLANC2USCORE(VAR_NAME, len(VAR_NAME))
            IF(MELISSA.and.(TIMESTEP.ne.0))THEN
                IF(TIMESTEP .eq. 1)THEN
                CALL MELISSA_INIT(VAR_NAME(1:16)//C_NULL_CHAR,N,
     &                            NCSIZE,IPID,SIMU_ID,COMM,0)
                ENDIF
                CALL MELISSA_SEND(TIMESTEP,VAR_NAME(1:16)//C_NULL_CHAR,
     &                            BVARSOR%ADR(I)%P%R,IPID,SIMU_ID)
            ENDIF

In source/utils/bief/bief_close_files.f, line 94, add:

      IF(MELISSA) THEN
        CALL MELISSA_FINALIZE()
        MELISSA=.FALSE.
      ENDIF
