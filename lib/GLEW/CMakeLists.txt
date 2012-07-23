PROJECT(GLEW)

SET(GLEW_SRC
   src/src/glew.c
   )
SET(GLEW_INC
   src/include/GL/glew.h
   src/include/GL/glxew.h
   src/include/GL/wglew.h
   )

INCLUDE_DIRECTORIES(src/include)
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(GLEW ${GLEW_SRC})
TARGET_LINK_LIBRARIES(GLEW)
