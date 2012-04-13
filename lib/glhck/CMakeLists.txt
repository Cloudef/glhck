PROJECT(glhck)

SET(GLHCK_SRC
   src/src/init.c
   src/src/alloc.c
   src/src/trace.c
   src/src/util.c
   src/src/object.c
   src/src/texture.c
   src/src/geometry/cube.c
   src/src/render/OpenGL.c
   src/src/import/import.c)

SET(GLHCK_INC
   src/include/GL/glhck.h)

FILE(COPY ${GLHCK_INC} DESTINATION ${INCLUDE_DIR}/GL)
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck dl SOIL kazmath m)
