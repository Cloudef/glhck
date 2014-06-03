ADD_LIBRARY(glhck-importer-tga SHARED tga.c)
SET_TARGET_PROPERTIES(glhck-importer-tga PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-tga)
