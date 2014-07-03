all: delta-tests

delta-tests: delta-crdts.cc delta-tests.cc
	g++ delta-crdts.cc delta-tests.cc -o delta-tests

clean:
	rm delta-tests