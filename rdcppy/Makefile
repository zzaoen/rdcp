.PHONY: clean

CFLAGS  := -g
# CFLAGS  := -Wall -Werror -g
LD      := gcc
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := rdcp-server rdcp

all: ${APPS}


rdcp-server: common.o server.o
	${LD} -o $@ $^ ${LDLIBS}

rdcp: common.o client.o read_cfg.o
	${LD} -o $@ $^ ${LDLIBS}

install-server:
	sudo cp ./rdcp-server /usr/bin/

install-client:
	sudo cp ./rdcp /usr/bin/

clean:
	rm -f *.o ${APPS}

