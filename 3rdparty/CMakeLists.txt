# Build tools
IF (GLHCK_BUILD_TOOLS)
    MESSAGE("Building GLhck with tools")

    # 3rd party tristripping utility
    ADD_SUBDIRECTORY(triedge)
ENDIF ()

# Pandora related hacks
IF (PANDORA)
    MESSAGE("Compiling libpandorahck.so preloadable library for Pandora (HACK)")
    MESSAGE("See documentation for more details what this is")
    ADD_SUBDIRECTORY(pandora)
ENDIF ()
