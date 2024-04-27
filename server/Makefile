IDIR=-Ilinked_list/
OBJ = aesdsocket.o list.o


ifdef CROSS_COMPILE
	CC=${CROSS_COMPILE}gcc
endif

ifndef LDFLAGS
	LDFLAGS=-g -lpthread -lrt
endif

all:	aesdsocket test_ioctl

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< ${IDIR} $(LDFLAGS)

test_ioctl.o: test_ioctl.c 
	$(CC) -c -o $@ $< ${IDIR} $(LDFLAGS)

test_ioctl: test_ioctl.o
	$(CC) -o $@ $^ $(LDFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ ${IDIR} $(LDFLAGS)



clean:
	rm -f aesdsocket test_ioctl *.o

