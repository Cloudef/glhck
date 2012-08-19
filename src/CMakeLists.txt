SET(GLHCK_SRC
   init.c
   alloc.c
   trace.c
   util.c
   object.c
   text.c
   frustum.c
   camera.c
   texture.c
   atlas.c
   rtt.c
   texture_packer.c
   geometry/model.c
   geometry/plane.c
   geometry/cube.c
   render/OpenGL.c
   import/import.c
   import/import_mmd.c
   import/import_octm.c
   import/import_png.c
   import/import_tga.c
   import/import_bmp.c)

FIND_PACKAGE(PNG REQUIRED)

INCLUDE_DIRECTORIES(
  ${GLHCK_INCLUDE_DIR}
  ${PNG_INCLUDE_DIR}
  ${kazmath_SOURCE_DIR}/src
  ${GLEW_SOURCE_DIR}/src/include
  ${actc_SOURCE_DIR}/src
  ${MMD_SOURCE_DIR}/src
  ${IMGHCK_SOURCE_DIR}/src
  ${OpenCTM_SOURCE_DIR}/src/lib
)

ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck kazmath imghck openctm MMD actc GLEW ${PNG_LIBRARY} ${MATH_LIBRARY})

INSTALL(TARGETS glhck DESTINATION lib)
