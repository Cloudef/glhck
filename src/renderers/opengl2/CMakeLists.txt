INCLUDE_DIRECTORIES(include)
ADD_LIBRARY(glhck-renderer-opengl2 SHARED opengl2.c glad.c)
SET_TARGET_PROPERTIES(glhck-renderer-opengl2 PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-renderer-opengl2)
