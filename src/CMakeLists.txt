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
   import/import.c)

# Setup importers
IF (GLHCK_IMPORT_OPENCTM)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_OPENCTM)
   LIST(APPEND GLHCK_SRC import/import_octm.c)
   LIST(APPEND OPT_LIBS openctm)
   LIST(APPEND OPT_INCL ${OpenCTM_SOURCE_DIR}/src/lib)
ENDIF ()
IF (GLHCK_IMPORT_MMD)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_MMD)
   LIST(APPEND GLHCK_SRC import/import_mmd.c)
   LIST(APPEND OPT_LIBS MMD)
   LIST(APPEND OPT_INCL ${MMD_SOURCE_DIR}/src)
ENDIF ()
IF (GLHCK_IMPORT_ASSIMP)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_ASSIMP)
   LIST(APPEND GLHCK_SRC import/import_assimp.c)
   LIST(APPEND OPT_LIBS assimp)
ENDIF ()

IF (GLHCK_IMPORT_PNG)
   FIND_PACKAGE(PNG REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_PNG)
   LIST(APPEND GLHCK_SRC import/import_png.c)
   LIST(APPEND OPT_LIBS ${PNG_LIBRARY})
   LIST(APPEND OPT_INCL ${PNG_INCLUDE_DIR})
ENDIF ()
IF (GLHCK_IMPORT_JPEG)
   FIND_PACKAGE(JPEG REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_JPEG)
   LIST(APPEND GLHCK_SRC import/import_jpeg.c)
   LIST(APPEND OPT_LIBS ${JPEG_LIBRARY})
   LIST(APPEND OPT_INCL ${JPEG_INCLUDE_DIR})
ENDIF ()
IF (GLHCK_IMPORT_BMP)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_BMP)
   LIST(APPEND GLHCK_SRC import/import_bmp.c)
ENDIF ()

# no GLEW on GLES build
IF (NOT GLHCK_USE_GLES1 AND NOT GLHCK_USE_GLES_2)
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
TARGET_LINK_LIBRARIES(glhck kazmath imghck actc ${OPT_LIBS} ${MATH_LIBRARY})
INSTALL(TARGETS glhck DESTINATION lib)
