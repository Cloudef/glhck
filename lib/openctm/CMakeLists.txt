PROJECT(OpenCTM)
SET(OPENCTM_SRC
   src/lib/openctm.c
   src/lib/stream.c
   src/lib/compressMG1.c
   src/lib/compressMG2.c
   src/lib/compressRAW.c
   src/lib/liblzma/Alloc.c
   src/lib/liblzma/LzFind.c
   src/lib/liblzma/LzmaDec.c
   src/lib/liblzma/LzmaEnc.c
   src/lib/liblzma/LzmaLib.c)
INCLUDE_DIRECTORIES(src/lib/liblzma)
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(openctm ${OPENCTM_SRC})
TARGET_LINK_LIBRARIES(openctm)
