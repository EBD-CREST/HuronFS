AM_CPPFLAGS = -DCBB_PRELOAD -DLOG
AM_CXXFLAGS = -Wall -std=c++11
AM_LDFLAGS = -lpthread 

SYNC_FLAG = -DSTRICT_SYNC_DATA -DSYNC_DATA_WITH_REPLY
SERVER_WAIT_FLAG = -DBUSY_WAIT -DSLEEP_YIELD
CLIENT_WAIT_FLAG = -DBUSY_WAIT -DSLEEP_YIELD

if WRITE_BACK
WRITE_BACK = -DWRITE_BACK
endif

if DEBUG
AM_CPPFLAGS += -DDEBUG
endif

if USING_CCI
AM_CPPFLAGS += -DCCI $(CCI_INCLUDE)
AM_LDFLAGS +=  ${CCI_LIB} -lcci
else
AM_CPPFLAGS += -DTCP
endif
