# Chck utilities
ADD_SUBDIRECTORY(chck)

# Image texture compression library
ADD_SUBDIRECTORY(texhck)

# Math library
ADD_SUBDIRECTORY(kazmath)

# Used for triangle stripping
IF (GLHCK_TRISTRIP)
   ADD_SUBPROJECT(actc)
ENDIF ()

IF (GLHCK_IMPORT_MMD)
   # MikuMikuDance
   ADD_SUBDIRECTORY(mmd)
ENDIF ()
