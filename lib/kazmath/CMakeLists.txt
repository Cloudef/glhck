PROJECT(kazmath)
SET(KAZMATH_SRC
   src/kazmath/aabb.c
   src/kazmath/mat3.c
   src/kazmath/mat4.c
   src/kazmath/plane.c
   src/kazmath/quaternion.c
   src/kazmath/ray2.c
   src/kazmath/ray3.c
   src/kazmath/utility.c
   src/kazmath/vec2.c
   src/kazmath/vec3.c
   src/kazmath/vec4.c)

IF (UNIX AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
   SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.9)
      ADD_DEFINITIONS(-fPIC)
   ENDIF ()
ENDIF ()

IF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
   ADD_DEFINITIONS(-std=c99)
ENDIF ()

ADD_LIBRARY(kazmath ${KAZMATH_SRC})
TARGET_LINK_LIBRARIES(kazmath ${MATH_LIBRARY})

