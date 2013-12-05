PROJECT(OpenCTM)
SET(OPENCTM_SRC
   src/lib/openctm.c
   src/lib/stream.c
   src/lib/compressMG1.c
   src/lib/compressMG2.c
   src/lib/compressRAW.c
   src/lib/liblzma/Alloc.c
   src/lib/liblzma/LzFind.c
   src/lib/liblzma/LzmaDec.c
   src/lib/liblzma/LzmaEnc.c
   src/lib/liblzma/LzmaLib.c)
INCLUDE_DIRECTORIES(src/lib/liblzma)

IF (UNIX AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

IF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
   ADD_DEFINITIONS(-std=c99)
ENDIF ()

ADD_LIBRARY(openctm ${OPENCTM_SRC})
TARGET_LINK_LIBRARIES(openctm)
