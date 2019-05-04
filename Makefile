
notebook: main.o
	gcc main.o -o notebook

main.o: main.c
	gcc -c main.c

clean:
	rm *.o notebook
