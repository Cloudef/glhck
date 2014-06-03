IF (NOT EMSCRIPTEN)
   # Window and OGL context creation library
   ADD_SUBPROJECT(glfw)
ENDIF ()
