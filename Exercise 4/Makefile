#
# Makefile
#
# Operating Systems, Exercise 4
#

CC = gcc
#CFLAGS = -Wall -g
CFLAGS = -Wall -O2 -g

all: scheduler shell prog scheduler-shell_1.2 scheduler-shell_1.3

scheduler: scheduler.o proc-common.o
	$(CC) -o scheduler scheduler.o proc-common.o

scheduler-shell_1.2: scheduler-shell_1.2.o proc-common.o
	$(CC) -o scheduler-shell_1.2 scheduler-shell_1.2.o proc-common.o

scheduler-shell_1.3: scheduler-shell_1.3.o proc-common.o
	$(CC) -o scheduler-shell_1.3 scheduler-shell_1.3.o proc-common.o

shell: shell.o proc-common.o
	$(CC) -o shell shell.o proc-common.o

prog: prog.o proc-common.o
	$(CC) -o prog prog.o proc-common.o

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS) -o proc-common.o -c proc-common.c

shell.o: shell.c proc-common.h request.h
	$(CC) $(CFLAGS) -o shell.o -c shell.c

scheduler.o: scheduler.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler.o -c scheduler.c

scheduler-shell_1.2.o: scheduler-shell_1.2.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler-shell_1.2.o -c scheduler-shell_1.2.c

scheduler-shell_1.3.o: scheduler-shell_1.3.c proc-common.h request.h
	$(CC) $(CFLAGS) -o scheduler-shell_1.3.o -c scheduler-shell_1.3.c

prog.o: prog.c
	$(CC) $(CFLAGS) -o prog.o -c prog.c

clean:
	rm -f scheduler shell prog scheduler-shell_1.2 scheduler-shell_1.3 execve-example strace-test sigchld-example *.o
