PROJECT(glhck)

SET(GLHCK_SRC
   src/src/init.c
   src/src/alloc.c
   src/src/trace.c
   src/src/util.c
   src/src/object.c
   src/src/texture.c
   src/src/atlas.c
   src/src/rtt.c
   src/src/texture_packer.c
   src/src/geometry/model.c
   src/src/geometry/plane.c
   src/src/geometry/cube.c
   src/src/render/OpenGL.c
   src/src/import/import.c
   src/src/import/import_mmd.c)

SET(GLHCK_INC
   src/include/GL/glhck.h)

FILE(COPY ${GLHCK_INC} DESTINATION ${INCLUDE_DIR}/GL)
SET(INCLUDE_DIR ${GLHCK_INCLUDE_DIR})
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck SOIL kazmath MMD actc dl m)
