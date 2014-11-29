SET(GLFW_BUILD_EXAMPLES 0 CACHE BOOL "Don't build examples for GLFW")
SET(GLFW_BUILD_TESTS 0 CACHE BOOL "Don't build tests for GLFW")

# Check if we should use EGL
IF (GLHCK_USE_GLES1)
   SET(GLFW_USE_EGL 1 CACHE BOOL "Use EGL with GLFW")
   SET(GLFW_CLIENT_LIBRARY "glesv1" CACHE STRING "Link EGL against glesv1")
ELSEIF (GLHCK_USE_GLES2)
   SET(GLFW_USE_EGL 1 CACHE BOOL "Use EGL with GLFW")
   SET(GLFW_CLIENT_LIBRARY "glesv2" CACHE STRING "Link EGL against glesv2")
ELSE ()
   SET(GLFW_USE_EGL 0 CACHE BOOL "Don't use EGL with GLFW")
ENDIF ()

IF (WAYLAND)
   SET(GLFW_USE_WAYLAND 1 CACHE BOOL "Use Wayland for context creation (implies EGL as well)")
ENDIF ()

ADD_SUBDIRECTORY(src)
