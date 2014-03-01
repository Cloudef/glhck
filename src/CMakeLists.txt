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
   material.c
   texture.c
   atlas.c
   light.c
   renderbuffer.c
   framebuffer.c
   hwbuffer.c
   shader.c
   texture_packer.c
   collision.c
   kazmath.c
   geometry/cube.c
   geometry/sphere.c
   geometry/plane.c
   geometry/model.c
   skeletal/bone.c
   skeletal/skinbone.c
   skeletal/animation.c
   skeletal/animator.c
   render/render.c
   render/helper_stub.c
   render/stub.c
   import/import.c
   import/import_glhck.c)

IF (GLHCK_USE_GLES1)
   MESSAGE("Building GLhck with the OpenGL fixed pipeline renderer")
   ADD_DEFINITIONS(-DGLHCK_USE_GLES1)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL_FIXED_PIPELINE)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl_fixpipe.c)
ELSEIF (GLHCK_USE_GLES2)
   MESSAGE("Building GLhck with the OpenGL renderer")
   ADD_DEFINITIONS(-DGLHCK_USE_GLES2)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl.c)
ELSE ()
   MESSAGE("Building GLhck with the OpenGL renderer")
   MESSAGE("Building GLhck with the OpenGL fixed pipeline renderer")
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL_FIXED_PIPELINE)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl_fixpipe.c)
ENDIF ()

# Use ACTC for triangle stripping
IF (GLHCK_TRISTRIP)
   MESSAGE("Building GLhck with ACTC triangle stripping support")
   LIST(APPEND OPT_LIBS actc)
   LIST(APPEND OPT_INCL ${actc_SOURCE_DIR}/src)
   ADD_DEFINITIONS(-DGLHCK_TRISTRIP=1)
ENDIF ()

# Setup importers
IF (GLHCK_IMPORT_OPENCTM)
   MESSAGE("Building GLhck with OpenCTM support")
   ADD_DEFINITIONS(-DGLHCK_IMPORT_OPENCTM)
   LIST(APPEND GLHCK_SRC import/import_octm.c)
   LIST(APPEND OPT_LIBS openctm)
   LIST(APPEND OPT_INCL ${OpenCTM_SOURCE_DIR}/src/lib)
ENDIF ()
IF (GLHCK_IMPORT_MMD)
   MESSAGE("Building GLhck with MikuMikuDance support")
   ADD_DEFINITIONS(-DGLHCK_IMPORT_MMD)
   LIST(APPEND GLHCK_SRC import/import_mmd.c)
   LIST(APPEND OPT_LIBS mmd)
   LIST(APPEND OPT_INCL ${mmd_SOURCE_DIR})
ENDIF ()
IF (GLHCK_IMPORT_ASSIMP)
   MESSAGE("Building GLhck with Assimp support")
   ADD_DEFINITIONS(-DGLHCK_IMPORT_ASSIMP)
   LIST(APPEND GLHCK_SRC import/import_assimp.c)
   LIST(APPEND OPT_LIBS assimp)
   LIST(APPEND OPT_INCL ${Assimp_SOURCE_DIR}/include)
ENDIF ()

IF (GLHCK_IMPORT_PNG)
   MESSAGE("Building GLhck with PNG support")
   FIND_PACKAGE(PNG REQUIRED)
   FIND_PACKAGE(ZLIB REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_PNG)
   LIST(APPEND GLHCK_SRC import/import_png.c)
   LIST(APPEND OPT_LIBS ${PNG_LIBRARY} ${ZLIB_LIBRARIES})
   LIST(APPEND OPT_INCL ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
ENDIF ()
IF (GLHCK_IMPORT_JPEG)
   MESSAGE("Building GLhck with JPEG support")
   FIND_PACKAGE(JPEG REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_JPEG)
   LIST(APPEND GLHCK_SRC import/import_jpeg.c)
   LIST(APPEND OPT_LIBS ${JPEG_LIBRARY})
   LIST(APPEND OPT_INCL ${JPEG_INCLUDE_DIR})
ENDIF ()
IF (GLHCK_IMPORT_TGA)
   MESSAGE("Building GLhck with TGA support")
   ADD_DEFINITIONS(-DGLHCK_IMPORT_TGA)
   LIST(APPEND GLHCK_SRC import/import_tga.c)
ENDIF ()
IF (GLHCK_IMPORT_BMP)
   MESSAGE("Building GLhck with BMP support")
   ADD_DEFINITIONS(-DGLHCK_IMPORT_BMP)
   LIST(APPEND GLHCK_SRC import/import_bmp.c)
ENDIF ()

# include directories
INCLUDE_DIRECTORIES(
   ${glhck_SOURCE_DIR}/include
   ${glew-glhck_SOURCE_DIR}/include
   ${kazmath_SOURCE_DIR}/src
   ${GLSW_SOURCE_DIR}/src
   ${chck_SOURCE_DIR}/src
   ${texhck_SOURCE_DIR}
   ${GLHCK_INCL}
   ${OPT_INCL}
)

IF (UNIX AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

ADD_DEFINITIONS(-DGLEW_STATIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck chck texhck kazmath glsw glew ${GLHCK_LINK} ${OPT_LIBS} ${MATH_LIBRARY})
INSTALL(TARGETS glhck DESTINATION lib)
