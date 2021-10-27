all : zing zing2

zing : main.o zing.o
	gcc -o zing main.o zing.o

zing2 : main.o zing2.o
	gcc -o exec2 main.o zing2.o

main.o : main.c
	gcc -c main.c

zing2.o : zing2.c
	gcc -c zing2.c
