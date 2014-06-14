SET(RENDERERS
   "stub"
   "opengl2"
   "opengl3"
)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/renderers)

FOREACH (renderer ${RENDERERS})
   ADD_SUBDIRECTORY(${renderer})
ENDFOREACH ()

