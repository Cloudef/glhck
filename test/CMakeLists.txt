SET(TEST_EXE
   display)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TEST_OUT_DIR})
FOREACH (test ${TEST_EXE})
   PROJECT(${test})
   ADD_EXECUTABLE(${test} src/${test}.c)
   INCLUDE_DIRECTORIES(
      ${glhck_SOURCE_DIR}/include
      ${GLFW_SOURCE_DIR}/include
      ${kazmath_SOURCE_DIR}/src
   )
   TARGET_LINK_LIBRARIES(${test} glfw glhck ${GLFW_LIBRARIES})
   INSTALL(TARGETS ${test} DESTINATION test)
ENDFOREACH (test)

INSTALL(DIRECTORY media DESTINATION test)
