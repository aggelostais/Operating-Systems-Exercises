CC = gcc

# CAUTION: Always use '-pthread' when compiling POSIX threads-based
# applications, instead of linking with "-lpthread" directly.
CFLAGS = -Wall -O2 -pthread
LIBS = 

all: sync_mutex sync_atomic sync_atomic.s sync_mutex.s

sync_mutex: sync_mutex.o
	$(CC) $(CFLAGS) -o sync_mutex sync_mutex.o $(LIBS)

sync_mutex.o: sync.c
	$(CC) $(CFLAGS) -DSYNC_MUTEX -c -o sync_mutex.o sync.c

sync_atomic: sync_atomic.o
	$(CC) $(CFLAGS) -o sync_atomic sync_atomic.o $(LIBS)

sync_atomic.o: sync.c
	$(CC) $(CFLAGS) -DSYNC_ATOMIC -c -o sync_atomic.o sync.c

sync_atomic.s: sync.c
	$(CC) $(CFLAGS) -DSYNC_ATOMIC -S -g -o sync_atomic.s sync.c

sync_mutex.s: sync.c
	$(CC) $(CFLAGS) -DSYNC_MUTEX -S -g -o sync_mutex.s sync.c

clean:
	rm -f *.s *.o sync_mutex sync_atomic
