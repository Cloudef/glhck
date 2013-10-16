PROJECT(pandorahck)
SET(PANDORAHCK_SRC src/pandorahck.c)
ADD_DEFINITIONS(-std=c99 -fPIC)

OPTION(PANDORAHCK_NO_GRAB "Disable X11 input grabbing from pandorahck" OFF)
OPTION(PANDORAHCK_NO_FRAMEBUFFER "Disable framebuffer mode from  pandorahck" OFF)

IF (PANDORAHCK_NO_GRAB)
   MESSAGE("Disabling input grab from libpandorahck")
   ADD_DEFINITIONS(-DPANDORAHCK_NO_GRAB=1)
ENDIF ()

IF (PANDORAHCK_NO_FRAMEBUFFER)
   MESSAGE("Disabling framebuffer mode from libpandorahck")
   ADD_DEFINITIONS(-DPANDORAHCK_NO_FRAMEBUFFER=1)
ENDIF ()

ADD_LIBRARY(pandorahck SHARED ${PANDORAHCK_SRC})
TARGET_LINK_LIBRARIES(pandorahck)
INSTALL(TARGETS pandorahck DESTINATION preload)
