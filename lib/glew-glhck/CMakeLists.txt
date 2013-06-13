# Check if we should build with OpenGL ES support
IF (GLHCK_USE_GLES1)
   SET(GLEW_USE_GLES1 1 CACHE BOOL "Build GLEW with ES1.x support")
ELSEIF (GLHCK_USE_GLES2)
   SET(GLEW_USE_GLES2 1 CACHE BOOL "Build GLEW with ES2.x support")
ENDIF ()

ADD_SUBDIRECTORY(src)
