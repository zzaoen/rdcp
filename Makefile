.PHONY: clean

CFLAGS  := -Wall -Werror -g
# CFLAGS  := -Wall -Werror -g
LD      := gcc
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := rdcp

all: ${APPS}


rdcp: common.o server.o
	${LD} -o $@ $^ ${LDLIBS}

clean:
	rm -f *.o ${APPS}

