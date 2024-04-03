CFLAGS=-Ilinked_list/ -g -lpthread -lrt
DEPS = ""
OBJ = aesdsocket.o list.o

ifdef CROSS_COMPILE
	ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
		CC=${CROSS_COMPILE}gcc
	else
		CC=gcc
	endif
else
	CC=gcc
endif

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all:	aesdsocket

clean:
	rm -f aesdsocket *.o
