SET(TESTS
   "init"
   "object"
   )

SET(EXT "")
IF (WIN32)
   SET(EXT ".exe")
ELSEIF (EMSCRIPTEN)
   SET(EXT ".bc")
ENDIF ()

ADD_DEFINITIONS(-std=c99)
FOREACH (test ${TESTS})
   ADD_EXECUTABLE(${test}${EXT} ${test}.c)
   INCLUDE_DIRECTORIES(
      ${glhck_SOURCE_DIR}/include
      ${GLFW_SOURCE_DIR}/include
      ${kazmath_SOURCE_DIR}/src
   )
   TARGET_LINK_LIBRARIES(${test}${EXT} glfw glhck ${GLHCK_LIBRARIES} ${GLFW_LIBRARIES})
   INSTALL(TARGETS ${test}${EXT} DESTINATION bin/tests)
   ADD_TEST(NAME ${test}${EXT} WORKING_DIRECTORY ${glhck_BINARY_DIR} COMMAND ${TEST_PRG} ${test}${EXT})
ENDFOREACH (test)
