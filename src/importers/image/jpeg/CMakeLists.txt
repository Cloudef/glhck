ADD_LIBRARY(glhck-importer-jpeg SHARED jpeg.c)
SET_TARGET_PROPERTIES(glhck-importer-jpeg PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-jpeg)
