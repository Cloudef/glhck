# Check if we should build with OpenGL ES support
IF (GLHCK_USE_GLES1)
   SET(GLEW_ES_ONLY 1 CACHE BOOL "Build GLEW with ES support only")
ELSEIF (GLHCK_USE_GLES2)
   SET(GLEW_ES_ONLY 1 CACHE BOOL "Build GLEW with ES support only")
ELSE ()
   SET(GLEW_ES_ONLY 0 CACHE BOOL "Build GLEW normally")
ENDIF ()

ADD_SUBDIRECTORY(src)
