ADD_LIBRARY(glhck-importer-png SHARED png.c)
SET_TARGET_PROPERTIES(glhck-importer-png PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-png)
