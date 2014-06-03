INCLUDE_DIRECTORIES(include)
ADD_LIBRARY(glhck-renderer-opengl3 SHARED opengl3.c glad.c glsw.c bstrlib.c)
SET_TARGET_PROPERTIES(glhck-renderer-opengl3 PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-renderer-opengl3)
