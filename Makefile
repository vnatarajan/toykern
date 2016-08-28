all:	memtest proctest

memtest:	memtest.c mem.c mem.h
	gcc -g -Wall -Werror -o memtest -I. -DUNIT_TEST mem.c memtest.c

proctest:	proctest.c proc.c proc.h mem.c mem.h
	gcc -g -Wall -Werror -o proctest -I. -DUNIT_TEST mem.c proc.c proctest.c

clean:
	rm -f memtest proctest
