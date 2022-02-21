main: main.o args.o
	gcc -pthread -o main main.o args.o 

main.o: main.c
	gcc -c main.c -o main.o

args.o: args.c
	gcc -c args.c -o args.o

