CFLAGS=-Ilinked_list/ -g -lpthread -lrt
DEPS = ""
OBJ = aesdsocket.o list.o

ifdef CROSS_COMPILE
	CC=${CROSS_COMPILE}gcc
endif

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all:	aesdsocket

clean:
	rm -f aesdsocket *.o
