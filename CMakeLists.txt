CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(glhck)
SET(GLHCK_NAME "GLhck")
SET(GLHCK_DESCRIPTION "A dead simple OpenGL(ES) framework")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${glhck_SOURCE_DIR}/CMake/modules)

# General build options
OPTION(GLHCK_BUILD_EXAMPLES "Build examples" ON)
OPTION(GLHCK_BUILD_TOOLS "Build tools" OFF)
OPTION(GLHCK_USE_GLES1 "Build for OpenGL ES 1.x" OFF)
OPTION(GLHCK_USE_GLES2 "Build for OpenGL ES 2.x" OFF)
OPTION(GLHCK_KAZMATH_DOUBLE "Define kmScalar as double" OFF)
OPTION(GLHCK_DISABLE_TRACE "Disable GLhck's tracing functionality" OFF)

# Build importers dynamically?
# FIXME: Not implemented
OPTION(GLHCK_IMPORT_DYNAMIC "Build importers dynamically" OFF)

# Model importers
OPTION(GLHCK_IMPORT_ASSIMP "Enable assimp importer" OFF)
OPTION(GLHCK_IMPORT_OPENCTM "Enable OpenCTM importer" ON)
OPTION(GLHCK_IMPORT_MMD "Enable MikuMikuDance importer" ON)

# Image importers
OPTION(GLHCK_IMPORT_BMP "Enable BMP importer" ON)
OPTION(GLHCK_IMPORT_PNG "Enable PNG importer" ON)
OPTION(GLHCK_IMPORT_TGA "Enable TGA importer" ON)
OPTION(GLHCK_IMPORT_JPEG "Enable JPEG importer" ON)

# Pandora related build options
IF (PANDORA)
    ADD_DEFINITIONS(-DPANDORA)
    SET(GLHCK_USE_GLES1 ON CACHE BOOL "Build for OpenGL ES 1.x")
    SET(GLHCK_USE_GLES2 OFF CACHE BOOL "Don't build for OpenGL ES 2.x")
    SET(GLHCK_USE_OPENCTM OFF CACHE BOOL "Turn OFF OpenCTM (Linker errors)")
ENDIF (PANDORA)

# Use double-precision floating point math on kazmath and glhck
IF (GLHCK_KAZMATH_DOUBLE)
   MESSAGE("Building GLhck with double precision")
   ADD_DEFINITIONS(-DUSE_DOUBLE_PRECISION)
ENDIF ()

# Disable tracing functionality
IF (GLHCK_DISABLE_TRACE)
   MESSAGE("Building GLhck without tracing (EXPERIMENTAL)")
   ADD_DEFINITIONS(-DGLHCK_DISABLE_TRACE)
ENDIF ()

# These are propably temporary
# In the end make renderers load this stuff dynamically(?)

IF (GLHCK_USE_GLES1)
   # GLES 1.x
   FIND_PACKAGE(EGL REQUIRED)
   LIST(APPEND GLHCK_INCL ${EGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_LINK ${EGL_LIBRARY})
   FIND_PACKAGE(GLESv1 REQUIRED)
   LIST(APPEND GLHCK_INCL ${GLESv1_INCLUDE_DIR})
   LIST(APPEND GLHCK_LINK ${GLESv1_LIBRARY})
   MESSAGE("Building GLhck for GLES 1.x")
ELSEIF (GLHCK_USE_GLES2)
   # GLES 2.x
   FIND_PACKAGE(EGL REQUIRED)
   LIST(APPEND GLHCK_INCL ${EGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_LINK ${EGL_LIBRARY})
   FIND_PACKAGE(GLESv2 REQUIRED)
   LIST(APPEND GLHCK_INCL ${GLESv2_INCLUDE_DIR})
   LIST(APPEND GLHCK_LINK ${GLESv2_LIBRARY})
   MESSAGE(FATAL_ERROR "GLES 2.x support not yet implemented")
ELSE ()
   # OPENGL
   FIND_PACKAGE(OpenGL REQUIRED)
   LIST(APPEND GLHCK_INCL ${OPENGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_LINK ${OPENGL_gl_LIBRARY})
   MESSAGE("Building GLhck for OpenGL")
ENDIF ()

# Export GLHCK library dependencies
SET(GLHCK_LIBRARIES ${GLHCK_LINK} CACHE STRING "Dependencies of GLHCK")

# Build checks
# Abort git submodule init && git submodule update has not been ran.
SET(SUBMODULE_ERROR "Run 'git submodule update --init --recursive' update first.")
IF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/kazmath/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/glfw/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/glew-glhck/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ENDIF ()

# Throw the error
IF (SUBMODULE_ERROR_THROW)
   MESSAGE(FATAL_ERROR ${SUBMODULE_ERROR})
ENDIF ()

# Libs like to define custom target (uninstall) and doesn't guard it...
SET_PROPERTY(GLOBAL APPEND PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS true)

# Build depencies
ADD_SUBDIRECTORY(lib)

# Warnings
IF (MSVC)
   IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   ELSE ()
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
   ENDIF ()
ELSEIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-variadic-macros -Wno-long-long")
ENDIF ()

# Build glhck
ADD_SUBDIRECTORY(src)

# Build tools
IF (GLHCK_BUILD_TOOLS)
   ADD_SUBDIRECTORY(3rdparty)
   MESSAGE("Building GLhck with tools")
ENDIF ()

# Build examples
IF (GLHCK_BUILD_EXAMPLES)
   ADD_SUBDIRECTORY(example)
   MESSAGE("Building GLhck with examples")
ENDIF ()

#
# FIXME: add real tests
#

# Install
FILE(COPY include/glhck/glhck.h DESTINATION include/glhck)

# Copy media?
IF (GLHCK_BUILD_EXAMPLES)
   FILE(COPY example/media DESTINATION ./example)
ENDIF ()
