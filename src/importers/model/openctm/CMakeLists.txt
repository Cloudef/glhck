ADD_LIBRARY(glhck-importer-openctm SHARED openctm.c)
SET_TARGET_PROPERTIES(glhck-importer-openctm PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-openctm)
