PROJECT(glhck)

SET(GLHCK_SRC
   src/src/init.c
   src/src/alloc.c
   src/src/trace.c
   src/src/util.c
   src/src/object.c
   src/src/text.c
   src/src/camera.c
   src/src/texture.c
   src/src/atlas.c
   src/src/rtt.c
   src/src/texture_packer.c
   src/src/geometry/model.c
   src/src/geometry/plane.c
   src/src/geometry/cube.c
   src/src/render/OpenGL.c
   src/src/import/import.c
   src/src/import/import_mmd.c
   src/src/import/import_tga.c)

SET(GLHCK_INC
   src/include/GL/glhck.h)

FIND_PACKAGE(Freetype REQUIRED)
FILE(COPY ${GLHCK_INC} DESTINATION ${GLHCK_TEST_INCLUDE_DIR}/GL)
INCLUDE_DIRECTORIES(${GLHCK_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck SOIL kazmath MMD actc ${FREETYPE_LIBRARY} ${MATH_LIBRARY})
