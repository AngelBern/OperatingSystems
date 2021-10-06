#author Angel Delgado
#CSE 3320 Spring 2021



CC=       	gcc
CURRALG= LD_PRELOAD=lib/libmalloc-nf.so
CFLAGS= 	-g -gdwarf-2 -std=gnu99 -Wall
LDFLAGS=
LIBRARIES=      lib/libmalloc-ff.so \
		lib/libmalloc-nf.so \
		lib/libmalloc-bf.so \
		lib/libmalloc-wf.so

TESTS=		tests/test1 \
                tests/test2 \
                tests/test3 \
                tests/test4 \
                tests/bfwf \
                tests/ffnf \
                tests/realloc \
                tests/calloc

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:    $(LIBRARIES) $(TESTS)

lib/libmalloc-ff.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DFIT=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-nf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DNEXT=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-bf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DBEST=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-wf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DWORST=0 -o $@ $< $(LDFLAGS)

calloc:		all
	echo "calloc:"
	env $(CURRALG) tests/calloc


realloc:		all
	echo "realloc:"
	env $(CURRALG) tests/realloc

test1:		all
	echo "test1:"
	env $(CURRALG) tests/test1

test2:		all
	echo "test2:"
	env $(CURRALG) tests/test2

test3:		all
	echo "test3:"
	env $(CURRALG) tests/test3

test4:		all
	echo "test4:"
	env $(CURRALG) tests/test4

ffnf:		all
	echo "ffnf:"
	env $(CURRALG) tests/ffnf

bfwf:		all
	echo "bfwf:"
	env $(CURRALG) tests/bfwf

testAll: test1 test2 test3 test4 ffnf bfwf calloc realloc

clean:
	rm -f $(LIBRARIES) $(TESTS)

.PHONY: all clean
