PROJECT(triedge)

SET(TRIEDGE_SRC
   src/triedge.c)

ADD_EXECUTABLE(triedge ${TRIEDGE_SRC})
TARGET_LINK_LIBRARIES(triedge actc)
