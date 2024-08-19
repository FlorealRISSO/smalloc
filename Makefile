.PHONY: all test clean format
.POSIX:

CC=gcc

test: build-test
	./test

build-test:
	$(CC) test.c -o test -g

format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]
clean:
	rm -f test
