.PHONY: clean

CFLAGS  := -Wall -Werror -g
# CFLAGS  := -Wall -Werror -g
LD      := gcc
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := rdcp-server

all: ${APPS}


rdcp-server: common.o server.o
	${LD} -o $@ $^ ${LDLIBS}

clean:
	rm -f *.o ${APPS}

