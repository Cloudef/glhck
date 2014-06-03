ADD_LIBRARY(glhck-importer-glhck SHARED glhck.c)
SET_TARGET_PROPERTIES(glhck-importer-glhck PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-glhck)
