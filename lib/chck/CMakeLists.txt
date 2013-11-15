# We could add_subdirectory(src) here,
# but we only compile the tools we really need from chck

PROJECT(chck)
SET(CHCK_SRC src/buffer/buffer.c)
ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(chck ${CHCK_SRC})
TARGET_LINK_LIBRARIES(chck)
