CC = g++
DEBUG = -g -v
FLAGS = -std=c++11 -ferror-limit=2

all: delta-tests

delta-tests: delta-crdts.cc delta-tests.cc
	$(CC) $(FLAGS) delta-tests.cc -o delta-tests

clean:
	rm delta-tests
