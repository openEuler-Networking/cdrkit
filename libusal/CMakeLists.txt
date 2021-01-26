PROJECT (LIBSCG C)
INCLUDE_DIRECTORIES(../include ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/include)
include(../include/AddScgBits.cmake)
ADD_DEFINITIONS(-DUSE_RCMD_RSH)

#SET(LIBSCG_SRCS rdummy.c usalsettarget.c usaltimes.c scsi-linux-ata.c scsi-linux-pg.c scsi-linux-sg.c scsierrs.c scsihack.c scsihelp.c scsiopen.c scsitransp.c)
SET(LIBSCG_SRCS usalsettarget.c usaltimes.c scsierrs.c scsihack.c scsihelp.c scsiopen.c scsitransp.c scsi-remote.c)
LINK_DIRECTORIES(../librols)
ADD_LIBRARY (usal SHARED ${LIBSCG_SRCS})

TARGET_LINK_LIBRARIES(usal ${SCG_SELF_LIBS})
SET_TARGET_PROPERTIES(usal PROPERTIES
    VERSION 0.0.1
    SOVERSION 0
)
INSTALL(TARGETS usal DESTINATION lib${LIB_SUFFIX})
INSTALL(FILES usal/scsicdb.h usal/aspi-dos.h usal/usalcmd.h usal/usalio.h usal/scsireg.h usal/usalops.h usal/scsidefs.h usal/spti-wnt.h usal/srb_os2.h usal/scsitransp.h usal/aspi-win32.h usal/scsisense.h usaltimes.h pg.h DESTINATION include/usal)
