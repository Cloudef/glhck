SET(EXAMPLE_EXE
   display
   distanceField
   game)

SET(EXT "")
IF (EMSCRIPTEN)
   SET(EXT ".bc")
ENDIF ()

FOREACH (example ${EXAMPLE_EXE})
   PROJECT(${example})
   ADD_EXECUTABLE(${example}${EXT} src/${example}.c)
   INCLUDE_DIRECTORIES(
      ${glhck_SOURCE_DIR}/include
      ${GLFW_SOURCE_DIR}/include
      ${kazmath_SOURCE_DIR}/src
   )
   TARGET_LINK_LIBRARIES(${example}${EXT} glfw glhck ${GLHCK_LIBRARIES} ${GLFW_LIBRARIES} ${EGL_LIBRARY})
   INSTALL(TARGETS ${example}${EXT} DESTINATION example)
ENDFOREACH (example)

INSTALL(DIRECTORY media DESTINATION example)
