CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(glhck)

SET(PROJECT_NAME "GLhck Rendering Engine")
SET(GLUE_DESCRIPTION "A dead simple OpenGL/GLES rendering engine")
OPTION(GLHCK_USE_GLES "Build for OpenGL ES" OFF)
OPTION(GLHCK_KAZMATH_FLOAT "Defines kmScalar as float" ON)

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
   ENDIF() # ^ LINUX

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
    SET(GLHCK_USE_GLES 1)
ENDIF (PANDORA)

# Use floating point math on kazmath
IF (GLHCK_KAZMATH_FLOAT)
   ADD_DEFINITIONS(-DGLHCK_KAZMATH_FLOAT -DUSE_SINGLE_PRECISION)
ENDIF ()

# These are propably temporary
# In the end make renderers load this stuff dynamically(?)

IF (GLHCK_USE_GLES)
   # GLES
   SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/modules)
   ADD_DEFINITIONS(-DGLES)
   FIND_PACKAGE(EGL REQUIRED)
   LIST(APPEND INCLUDE_DIR ${EGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${EGL_LIBRARY})
   FIND_PACKAGE(GLESv1 REQUIRED)
   LIST(APPEND INCLUDE_DIR ${GLESv1_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${GLESv1_LIBRARY})
ELSE ()
   # OPENGL
   FIND_PACKAGE(OpenGL REQUIRED)
   LIST(APPEND GLHCK_TEST_INCL ${OPENGL_INCLUDE_DIR})
   LIST(APPEND GLHCK_TEST_LINK ${OPENGL_gl_LIBRARY} GLEW)
ENDIF ()


# Build checks
# Abort git submodule init && git submodule update
# has not been ran.
SET(SUBMODULE_ERROR "Run git submodule init && git submodule update first.")
IF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/kazmath/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/glfw/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ENDIF ()

# Throw the error, if unix try fix it first
IF (SUBMODULE_ERROR_THROW)
   MESSAGE(FATAL_ERROR ${SUBMODULE_ERROR})
ENDIF ()

SET(GLHCK_INCLUDE_DIR ${glhck_SOURCE_DIR}/include)

# Do it (tm)
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

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(3rdparty)
ADD_SUBDIRECTORY(test)

INSTALL(FILES include/GL/glhck.h DESTINATION include/GL)
INSTALL(DIRECTORY test/media DESTINATION ./test)
FILE(COPY test/media DESTINATION ./test)
