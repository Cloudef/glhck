# We could add_subdirectory(src) here,
# but we only compile the tools we really need from chck

PROJECT(chckGlhck)
SET(CHCK_SRC
   src/buffer/buffer.c
   src/array/array.c
   src/pool/pool.c
   )

IF (UNIX AND CMAKE_COMPILER_IS_GNUCC)
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

ADD_LIBRARY(chckGlhck ${CHCK_SRC})
TARGET_LINK_LIBRARIES(chckGlhck)
