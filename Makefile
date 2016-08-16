memtest:	memtest.c mem.c mem.h
	gcc -g -Wall -Werror -o memtest -I. -DUNIT_TEST mem.c memtest.c

clean:
	rm memtest
