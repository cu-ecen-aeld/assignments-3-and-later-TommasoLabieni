CFLAGS=-I.
DEPS = ""
OBJ = writer.o 

ifdef CROSS_COMPILE
	CC=${CROSS_COMPILE}gcc
else
	CC=gcc
endif

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

writer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all:	writer

clean:
	rm -f writer *.o
