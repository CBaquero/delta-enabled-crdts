CC = gcc

all: id-gen

id-gen: id-gen.c
	$(CC) -o id-gen id-gen.c

clean:
	rm id-gen
