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
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(kazmath ${KAZMATH_SRC})
TARGET_LINK_LIBRARIES(kazmath ${MATH_LIBRARY})

