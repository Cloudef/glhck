ADD_LIBRARY(glhck-importer-bmp SHARED bmp.c)
SET_TARGET_PROPERTIES(glhck-importer-bmp PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-bmp)
