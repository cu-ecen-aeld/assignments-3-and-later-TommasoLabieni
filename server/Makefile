IDIR=-Ilinked_list/
OBJ = aesdsocket.o list.o

ifdef CROSS_COMPILE
	CC=${CROSS_COMPILE}gcc
endif

ifndef LDFLAGS
	LDFLAGS=-g -lpthread -lrt
endif

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< ${IDIR} $(LDFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ ${IDIR} $(LDFLAGS)

all:	aesdsocket

clean:
	rm -f aesdsocket *.o

