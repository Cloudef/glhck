PROJECT(kazmath)

SET(KAZMATH_SRC
   src/kazmath/aabb.c
   src/kazmath/mat3.c
   src/kazmath/mat4.c
   src/kazmath/plane.c
   src/kazmath/quaternion.c
   src/kazmath/ray2.c
   src/kazmath/utility.c
   src/kazmath/vec2.c
   src/kazmath/vec3.c
   src/kazmath/vec4.c)

SET(KAZMATH_INC
   src/kazmath/kazmath.h
   src/kazmath/aabb.h
   src/kazmath/mat3.h
   src/kazmath/mat4.h
   src/kazmath/ray2.h
   src/kazmath/plane.h
   src/kazmath/quaternion.h
   src/kazmath/utility.h
   src/kazmath/vec2.h
   src/kazmath/vec3.h
   src/kazmath/vec4.h)

FILE(COPY ${KAZMATH_INC} DESTINATION ${GLHCK_INCLUDE_DIR}/kazmath)
FILE(COPY ${KAZMATH_INC} DESTINATION ${GLHCK_TEST_INCLUDE_DIR}/kazmath)
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(kazmath ${KAZMATH_SRC})
TARGET_LINK_LIBRARIES(kazmath m)

