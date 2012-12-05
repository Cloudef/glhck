SET(GLHCK_SRC
   glhck.c
   alloc.c
   trace.c
   util.c
   geometry.c
   object.c
   text.c
   frustum.c
   camera.c
   texture.c
   atlas.c
   rtt.c
   texture_packer.c
   geometry/cube.c
   geometry/plane.c
   geometry/model.c
   render/OpenGL.c
   import/import.c
   import/import_jpeg.c
   import/import_png.c
   import/import_tga.c
   import/import_bmp.c
   import/import_octm.c
   import/import_mmd.c
   import/import_assimp.c)

FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)

# optional libraries
# NOTE: for now needed
SET(OPT_LIBS
   ${JPEG_LIBRARY}
   ${PNG_LIBRARY}
   openctm
   MMD)

# optional include directories
# NOTE: for now needed
SET(OPT_INCL
   ${JPEG_INCLUDE_DIR}
   ${PNG_INCLUDE_DIR}
   ${OpenCTM_SOURCE_DIR}/src/lib
   ${MMD_SOURCE_DIR}/src)

# no GLEW on GLES build
IF (NOT GLHCK_USE_GLES1)
   SET(OPT_LIBS ${OPT_LIBS} GLEW)
   SET(OPT_INCL ${OPT_INCL} ${GLEW_SOURCE_DIR}/src/include)
ENDIF ()

# include directories
INCLUDE_DIRECTORIES(
   ${glhck_SOURCE_DIR}/include
   ${kazmath_SOURCE_DIR}/src
   ${actc_SOURCE_DIR}/src
   ${IMGHCK_SOURCE_DIR}/src
   ${OPT_INCL}
)

ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck kazmath imghck actc assimp ${OPT_LIBS} ${MATH_LIBRARY})

INSTALL(TARGETS glhck DESTINATION lib)
