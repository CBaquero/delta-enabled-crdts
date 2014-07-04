CC = g++
DEBUG = -g

all: delta-tests

delta-tests: delta-crdts.cc delta-tests.cc
	$(CC) $(DEBUG) delta-crdts.cc delta-tests.cc -o delta-tests

clean:
	rm delta-tests