DHDB_LIB_DIR=../dhdb
DHDB_INCLUDE_DIR=../dhdb
CFLAGS=-I${DHDB_INCLUDE_DIR}
LDFLAGS=-lncurses

all: dhdb-edit

OBJS=\
	dhdb-edit.o
DHDB=\
	${DHDB_LIB_DIR}/dhdb.o \
	${DHDB_LIB_DIR}/dhdb_dump.o \
	${DHDB_LIB_DIR}/dhdb_json.o

dhdb-edit: ${OBJS}
	${CC} ${OBJS} -o$@ ${DHDB} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${OBJS} dhdb-edit
