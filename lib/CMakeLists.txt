# Chck utilities
ADD_SUBDIRECTORY(chck)

# Image texture compression library
ADD_SUBDIRECTORY(texhck)

# Math library
ADD_SUBDIRECTORY(kazmath)

IF (NOT EMSCRIPTEN)
   # Used for extension wrangling
   # Helps a lot, thanks GLEW devs
   ADD_SUBDIRECTORY(glew-glhck)
ENDIF ()

# Used for shader wrangling on OpenGL 3.x+ renderer.
ADD_SUBDIRECTORY(glsw)

# Used for triangle stripping
IF (GLHCK_TRISTRIP)
   ADD_SUBDIRECTORY(actc)
ENDIF ()

# Examples
IF (GLHCK_BUILD_EXAMPLES AND NOT EMSCRIPTEN)
   # Window and OGL context creation library
   ADD_SUBDIRECTORY(glfw)
ENDIF ()

# Importers
IF (GLHCK_IMPORT_MMD)
   # MikuMikuDance
   ADD_SUBDIRECTORY(mmd)
ENDIF ()
IF (GLHCK_IMPORT_OPENCTM)
   # OpenCTM
   ADD_SUBDIRECTORY(openctm)
ENDIF ()
IF (GLHCK_IMPORT_ASSIMP)
   # Assimp
   ADD_SUBDIRECTORY(assimp)
ENDIF ()
