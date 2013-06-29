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
   geometry/cube.c
   geometry/sphere.c
   geometry/plane.c
   geometry/model.c
   skeletal/bone.c
   skeletal/animation.c
   skeletal/animator.c
   render/render.c
   render/helper_stub.c
   render/stub.c
   import/import.c)

IF (GLHCK_USE_GLES1)
   ADD_DEFINITIONS(-DGLHCK_USE_GLES1)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL_FIXED_PIPELINE)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl_fixpipe.c)
   MESSAGE("Building GLhck with the OpenGL fixed pipeline renderer")
ELSEIF (GLHCK_USE_GLES2)
   ADD_DEFINITIONS(-DGLHCK_USE_GLES2)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl.c)
   MESSAGE("Building GLhck with the OpenGL renderer")
ELSE ()
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL)
   ADD_DEFINITIONS(-DGLHCK_HAS_OPENGL_FIXED_PIPELINE)
   LIST(APPEND GLHCK_SRC render/helper_opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl.c)
   LIST(APPEND GLHCK_SRC render/opengl_fixpipe.c)
   MESSAGE("Building GLhck with the OpenGL renderer")
   MESSAGE("Building GLhck with the OpenGL fixed pipeline renderer")
ENDIF ()

# Use ACTC for triangle stripping
IF (GLHCK_TRISTRIP)
   LIST(APPEND OPT_LIBS actc)
   LIST(APPEND OPT_INCL ${actc_SOURCE_DIR}/src)
   MESSAGE("Building GLhck with ACTC triangle stripping support")
   ADD_DEFINITIONS(-DGLHCK_TRISTRIP=1)
ENDIF ()

# Setup importers
IF (GLHCK_IMPORT_OPENCTM)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_OPENCTM)
   LIST(APPEND GLHCK_SRC import/import_octm.c)
   LIST(APPEND OPT_LIBS openctm)
   LIST(APPEND OPT_INCL ${OpenCTM_SOURCE_DIR}/src/lib)
   MESSAGE("Building GLhck with OpenCTM support")
ENDIF ()
IF (GLHCK_IMPORT_MMD)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_MMD)
   LIST(APPEND GLHCK_SRC import/import_mmd.c)
   LIST(APPEND OPT_LIBS MMD)
   LIST(APPEND OPT_INCL ${MMD_SOURCE_DIR}/src)
   MESSAGE("Building GLhck with MikuMikuDance support")
ENDIF ()
IF (GLHCK_IMPORT_ASSIMP)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_ASSIMP)
   LIST(APPEND GLHCK_SRC import/import_assimp.c)
   LIST(APPEND OPT_LIBS assimp)
   LIST(APPEND OPT_INCL ${Assimp_SOURCE_DIR}/include)
   MESSAGE("Building GLhck with Assimp support")
ENDIF ()

IF (GLHCK_IMPORT_PNG)
   FIND_PACKAGE(PNG REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_PNG)
   LIST(APPEND GLHCK_SRC import/import_png.c)
   LIST(APPEND OPT_LIBS ${PNG_LIBRARY})
   LIST(APPEND OPT_INCL ${PNG_INCLUDE_DIR})
   MESSAGE("Building GLhck with PNG support")
ENDIF ()
IF (GLHCK_IMPORT_JPEG)
   FIND_PACKAGE(JPEG REQUIRED)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_JPEG)
   LIST(APPEND GLHCK_SRC import/import_jpeg.c)
   LIST(APPEND OPT_LIBS ${JPEG_LIBRARY})
   LIST(APPEND OPT_INCL ${JPEG_INCLUDE_DIR})
   MESSAGE("Building GLhck with JPEG support")
ENDIF ()
IF (GLHCK_IMPORT_TGA)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_TGA)
   LIST(APPEND GLHCK_SRC import/import_tga.c)
   MESSAGE("Building GLhck with TGA support")
ENDIF ()
IF (GLHCK_IMPORT_BMP)
   ADD_DEFINITIONS(-DGLHCK_IMPORT_BMP)
   LIST(APPEND GLHCK_SRC import/import_bmp.c)
   MESSAGE("Building GLhck with BMP support")
ENDIF ()

# include directories
INCLUDE_DIRECTORIES(
   ${glhck_SOURCE_DIR}/include
   ${glew-glhck_SOURCE_DIR}/include
   ${kazmath_SOURCE_DIR}/src
   ${IMGHCK_SOURCE_DIR}/src
   ${GLSW_SOURCE_DIR}/src
   ${GLHCK_INCL}
   ${OPT_INCL}
)

ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(glhck ${GLHCK_SRC})
TARGET_LINK_LIBRARIES(glhck kazmath imghck glsw glew ${GLHCK_LINK} ${OPT_LIBS} ${MATH_LIBRARY})
INSTALL(TARGETS glhck DESTINATION lib)
