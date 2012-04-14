PROJECT(stripy)

SET(STRIPY_SRC
   src/main.cpp
   src/NvTriStrip.cpp
   src/NvTriStripObjects.cpp)
   #src/VertexCache.cpp)

FILE(COPY src/teststripy.py DESTINATION ${3RDPARTY_OUT_DIR})
ADD_EXECUTABLE(stripy ${STRIPY_SRC})
TARGET_LINK_LIBRARIES(stripy)
