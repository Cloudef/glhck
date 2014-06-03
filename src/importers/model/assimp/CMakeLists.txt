ADD_LIBRARY(glhck-importer-assimp SHARED assimp.c)
SET_TARGET_PROPERTIES(glhck-importer-assimp PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(glhck-importer-assimp)
