SET(IMPORTERS
   "bmp"
   "tga"
   "png"
   # "jpeg"
)

IF (EMSCRIPTEN)
   SET(IMPORTERS "emscripten")
ENDIF ()

FOREACH (importer ${IMPORTERS})
   ADD_SUBDIRECTORY(${importer})
ENDFOREACH ()
