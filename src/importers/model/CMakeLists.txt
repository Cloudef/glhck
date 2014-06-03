SET(IMPORTERS
   "glhck"
   # "assimp"
   # "openctm"
)

IF (GLHCK_IMPORT_MMD)
   LIST(APPEND IMPORTERS "mmd")
ENDIF ()

FOREACH (importer ${IMPORTERS})
   ADD_SUBDIRECTORY(${importer})
ENDFOREACH ()
