CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(glhck)
SET(GLHCK_NAME "GLhck Rendering Engine")
SET(GLHCK_DESCRIPTION "A dead simple OpenGL/GLES rendering engine")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${glhck_SOURCE_DIR}/CMake/modules)

# General build options
OPTION(GLHCK_BUILD_EXAMPLES "Build examples" ON)
OPTION(GLHCK_BUILD_TOOLS "Build tools" OFF)
OPTION(GLHCK_USE_GLES1 "Build for OpenGL ES 1.x" OFF)
OPTION(GLHCK_USE_GLES2 "Build for OpenGL ES 2.x" OFF)
OPTION(GLHCK_KAZMATH_DOUBLE "Define kmScalar as double" OFF)

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

# Debug build
SET(CMAKE_BUILD_TYPE Debug)

# Release build
# SET(CMAKE_BUILD_TYPE Release)

# Set UNIX depencies
IF (UNIX)
   IF (APPLE)  # MAC
      find_library(COCOA_FRAMEWORK Cocoa)
      find_library(IOKIT_FRAMEWORK IOKit)
      find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
      list(APPEND GLHCK_TEST_LINK ${COCOA_FRAMEWORK}
                                  ${OPENGL_gl_LIBRARY}
                                  ${IOKIT_FRAMEWORK}
                                  ${CORE_FOUNDATION_FRAMEWORK})
   ELSE ()     # LINUX
      FIND_PACKAGE(X11 REQUIRED)

      # X11
      LIST(APPEND GLHCK_TEST_INCL ${X11_X11_INCLUDE_PATH})
      LIST(APPEND GLHCK_TEST_LINK ${X11_X11_LIB})

      # XRandr
      IF (X11_Xrandr_FOUND)
         LIST(APPEND GLHCK_TEST_LINK ${X11_Xrandr_LIB})
      ENDIF ()

      # Xf86vmode
      IF (X11_xf86vmode_FOUND)
         LIST(APPEND GLHCK_TEST_INCL ${X11_xf86vmode_INCLUDE_PATH})

         # Workaround CMake bug 0006976
         IF (X11_xf86vmode_LIB)
            LIST(APPEND GLHCK_TEST_LINK ${X11_xf86vmode_LIB})
         ELSE ()
            LIST(APPEND GLHCK_TEST_LINK Xxf86vm)
         ENDIF ()
      ENDIF ()

      # Xkb
      IF (X11_Xkb_FOUND)
         LIST(APPEND GLHCK_TEST_INCL ${X11_Xkb_INCLUDE_PATH})
      ENDIF ()
   ENDIF () # ^ LINUX

   # Rt
   FIND_LIBRARY(RT_LIBRARY rt)
   MARK_AS_ADVANCED(RT_LIBRARY)
   IF (RT_LIBRARY)
      LIST(APPEND GLHCK_TEST_LINK ${RT_LIBRARY})
   ENDIF ()

   # Math
   FIND_LIBRARY(MATH_LIBRARY m)
   MARK_AS_ADVANCED(MATH_LIBRARY)
   IF (MATH_LIBRARY)
      LIST(APPEND GLHCK_TEST_LINK ${MATH_LIBRARY})
   ENDIF ()
ENDIF (UNIX)

# Pandora related build options
IF (PANDORA)
    ADD_DEFINITIONS(-DPANDORA)
    SET(GLHCK_USE_GLES1 ON CACHE BOOL)
ENDIF (PANDORA)

# Use double-precision floating point math on kazmath and glhck
IF (GLHCK_KAZMATH_DOUBLE)
   MESSAGE("Building GLhck with double precision")
   ADD_DEFINITIONS(-DUSE_DOUBLE_PRECISION)
ENDIF ()

# These are propably temporary
# In the end make renderers load this stuff dynamically(?)

IF (GLHCK_USE_GLES1)
   # GLES 1.x
   ADD_DEFINITIONS(-DGLHCK_USE_GLES1)
   FIND_PACKAGE(EGL REQUIRED)
   LIST(APPEND INCLUDE_DIR ${EGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${EGL_LIBRARY})
   FIND_PACKAGE(GLESv1 REQUIRED)
   LIST(APPEND INCLUDE_DIR ${GLESv1_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${GLESv1_LIBRARY})
   MESSAGE("GLES 1.x support not working yet")
ELSEIF (GLHCK_USE_GLES2)
   # GLES 2.x
   MESSAGE(FATAL_ERROR "GLES 2.x support not yet implemented")
ELSE ()
   # OPENGL
   FIND_PACKAGE(OpenGL REQUIRED)
   LIST(APPEND GLHCK_TEST_INCL ${OPENGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${OPENGL_gl_LIBRARY})
   MESSAGE("Building GLhck for OpenGL")
ENDIF ()

# Build checks
# Abort git submodule init && git submodule update has not been ran.
SET(SUBMODULE_ERROR "Run 'git submodule update --init --recursive' update first.")
IF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/kazmath/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/glfw/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ENDIF ()

# Throw the error
IF (SUBMODULE_ERROR_THROW)
   MESSAGE(FATAL_ERROR ${SUBMODULE_ERROR})
ENDIF ()

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
   SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wno-long-long")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
ENDIF ()

# Build glhck
ADD_SUBDIRECTORY(src)

# Build tools
IF (GLHCK_BUILD_TOOLS)
   ADD_SUBDIRECTORY(3rdparty)
ENDIF ()

# Build examples
IF (GLHCK_BUILD_EXAMPLES)
   ADD_SUBDIRECTORY(example)
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
