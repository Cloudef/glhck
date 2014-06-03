SET(GLHCK_SRC
   geometry/cube.c
   geometry/plane.c
   geometry/sphere.c
   geometry/model.c
   glhck.c
   handle.c
   importers/import.c
   importers/importer.c
   renderers/render.c
   renderers/renderer.c
   trace.c
   types/atlas.c
   types/camera.c
   types/framebuffer.c
   types/frustum.c
   types/geometry.c
   types/hwbuffer.c
   types/list.c
   types/material.c
   types/object.c
   types/renderbuffer.c
   types/shader.c
   types/skeletal/animation.c
   types/skeletal/animator.c
   types/skeletal/bone.c
   types/skeletal/node.c
   types/skeletal/skinbone.c
   types/string.c
   types/text/text.c
   types/texture.c
   types/view.c
)

# Use ACTC for triangle stripping
IF (GLHCK_TRISTRIP)
   MESSAGE("Building GLhck with ACTC triangle stripping support")
   LIST(APPEND OPT_LIBS actc)
   LIST(APPEND OPT_INCL ${actc_SOURCE_DIR}/src)
   ADD_DEFINITIONS(-DGLHCK_TRISTRIP=1)
ENDIF ()

# include directories
INCLUDE_DIRECTORIES(
   ${glhck_SOURCE_DIR}/include
   ${glhck_SOURCE_DIR}/src
   ${kazmath_SOURCE_DIR}/src
   ${chckGlhck_SOURCE_DIR}/src
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

ADD_DEFINITIONS(-std=c99)

# compile renderers
ADD_SUBDIRECTORY(renderers)

# compile importers
ADD_SUBDIRECTORY(importers)

# compile glhck
ADD_LIBRARY(glhck ${GLHCK_SRC})
SET_TARGET_PROPERTIES(glhck PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${glhck_BINARY_DIR})
TARGET_LINK_LIBRARIES(glhck chckGlhck texhck kazmath ${GLHCK_LINK} ${OPT_LIBS})
INSTALL(TARGETS glhck DESTINATION lib)
