ADD_LIBRARY(glhck-importer-emscripten SHARED emscripten.c)
SET_TARGET_PROPERTIES(glhck-importer-emscripten PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-emscripten)
