ADD_LIBRARY(glhck-renderer-stub SHARED stub.c)
SET_TARGET_PROPERTIES(glhck-renderer-stub PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-renderer-stub)
