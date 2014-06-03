SET(EXAMPLES
   # display
   # distanceField
   game
   refactor
   )

SET(EXT "")
IF (WIN32)
   SET(EXT ".exe")
ELSEIF (EMSCRIPTEN)
   SET(EXT ".bc")
ENDIF ()

ADD_DEFINITIONS(-std=c99)
FOREACH (example ${EXAMPLES})
   ADD_EXECUTABLE(${example}${EXT} ${example}.c)
   INCLUDE_DIRECTORIES(
      ${glhck_SOURCE_DIR}/include
      ${GLFW_SOURCE_DIR}/include
      ${kazmath_SOURCE_DIR}/src
   )
   TARGET_LINK_LIBRARIES(${example}${EXT} glfw glhck ${GLHCK_LIBRARIES} ${GLFW_LIBRARIES})
   INSTALL(TARGETS ${example}${EXT} DESTINATION bin/examples)
ENDFOREACH (example)
