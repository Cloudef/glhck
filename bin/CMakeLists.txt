IF (GLHCK_BUILD_EXAMPLES OR GLHCK_BUILD_TESTS)
   ADD_SUBDIRECTORY(lib)
ENDIF ()

IF (GLHCK_BUILD_TESTS)
   # Build tests
   MESSAGE("Building GLhck with tests")
   ADD_SUBDIRECTORY(tests)
ENDIF ()

IF (GLHCK_BUILD_EXAMPLES)
   # Build examples
   MESSAGE("Building GLhck with examples")
   ADD_SUBDIRECTORY(examples)
ENDIF ()

# Copy media
FILE(COPY media DESTINATION .)
