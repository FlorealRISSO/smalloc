.PHONY: all test clean format
.POSIX:

CC=gcc

all: build-test viewer
	@echo "Run 'make test' to run the tests"
	@echo "Run './viewer <file> [-n <NB_STATUS>] [-c <columns>]' to view a file"
	@echo "Or use the tools/viewer.html file in a browser"

test: build-test
	./test

build-test:
	$(CC) test.c -o test -g

viewer: tools/viewer.c
	$(CC) tools/viewer.c -o viewer

format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]

clean:
	rm -f test
