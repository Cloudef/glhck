CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(glhck)
SET(GLHCK_NAME "GLhck")
SET(GLHCK_DESCRIPTION "Entity manager and rendering engine framework")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${glhck_SOURCE_DIR}/CMake)

# Includes
INCLUDE(CTest)
INCLUDE(subproject)

# OpenPandora hacks
IF (PANDORA)
   MESSAGE("Building GLhck with Pandora support")
   ADD_DEFINITIONS(-DPANDORA)
   SET(GLHCK_USE_GLES1 ON) # XXX: temporary
ENDIF ()

IF (WAYLAND)
   SET(GLHCK_USE_GLES1 ON) # XXX: temporary
ENDIF ()

# Emscripten build
IF (EMSCRIPTEN)
   MESSAGE("Building GLhck with Emscripten support")
   MESSAGE("NOTE: For examples you need glfw3 emulation in Emscripten.")
   MESSAGE("      This is not yet pushed to upstream so, query for more details about this.")
   ADD_DEFINITIONS(-DEMSCRIPTEN)
ENDIF ()

# General build options
OPTION(GLHCK_BUILD_TESTS "Build tests" ON)
OPTION(GLHCK_BUILD_EXAMPLES "Build examples" ON)
OPTION(GLHCK_BUILD_TOOLS "Build tools" OFF)
OPTION(GLHCK_TRISTRIP "Build GLhck with ACTC triangle stripping support (BUGS)" OFF)
OPTION(GLHCK_DISABLE_TRACE "Disable GLhck's tracing functionality (EXPERIMENTAL)" OFF)
OPTION(GLHCK_FLOAT_TEXT "Use floating point vertex data for text" OFF)

# Model importers
OPTION(GLHCK_IMPORT_MMD "Enable MikuMikuDance importer" OFF)

# Disable tracing functionality
IF (GLHCK_DISABLE_TRACE)
   MESSAGE("Building GLhck without tracing (EXPERIMENTAL)")
   ADD_DEFINITIONS(-DGLHCK_DISABLE_TRACE=1)
ENDIF ()

# Use floating point text
IF (GLHCK_FLOAT_TEXT)
   MESSAGE("Building GLhck with floating point text precision")
   ADD_DEFINITIONS(-DGLHCK_TEXT_FLOAT_PRECISION=1)
ENDIF ()

# Math library is needed
FIND_LIBRARY(MATH_LIBRARY m)
MARK_AS_ADVANCED(MATH_LIBRARY)
IF (MATH_LIBRARY)
   LIST(APPEND GLHCK_LINK ${MATH_LIBRARY})
ELSE ()
   MESSAGE(FATAL_ERROR "Math library was not found")
ENDIF ()

# Unix dynamic links
IF (UNIX)
   FIND_LIBRARY(DL_LIBRARY dl)
   MARK_AS_ADVANCED(DL_LIBRARY)
   IF (DL_LIBRARY)
      LIST(APPEND GLHCK_LINK ${DL_LIBRARY})
   ELSE ()
      MESSAGE(FATAL_ERROR "On unix systems dl library is needed")
   ENDIF ()
ENDIF (UNIX)

# Export GLHCK library dependencies
SET(GLHCK_LIBRARIES ${GLHCK_LINK} CACHE STRING "Dependencies of GLHCK")

# Build checks
# Abort git submodule init && git submodule update has not been ran.
SET(SUBMODULE_ERROR "Run 'git submodule update --init --recursive' update first.")
IF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/kazmath/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/chck/src/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ELSEIF (NOT EXISTS ${glhck_SOURCE_DIR}/lib/texhck/src/.git)
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
   IF (MINGW)
      SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mwindows -static")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -static")
   ENDIF ()
ENDIF ()

# Build glhck
ADD_SUBDIRECTORY(src)

# Build 3rdparty tools
ADD_SUBDIRECTORY(3rdparty)

# Build binaries
ADD_SUBDIRECTORY(bin)

# Install header
FILE(COPY include/glhck/glhck.h DESTINATION include/glhck)
