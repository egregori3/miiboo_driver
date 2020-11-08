# Building the shared library
# gcc -c -Wall -Werror -fpic miiboo_driver.c
# gcc -shared -o libmiiboo.so miiboo_driver.o

# Linking with the shared library
# gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./



IDIR=./
CC=gcc
CFLAGS=-Wall -Werror

SLIBS=-lpthread
SODIR=sobj
SOLIB=libmiiboo.so

LIBS=-lmiiboo


# Shared Library
_SDEPS = miiboo_driver.h
SDEPS = $(patsubst %,$(IDIR)/%,$(_SDEPS))

_SOBJ = miiboo_driver.o
SOBJ = $(patsubst %,$(SODIR)/%,$(_SOBJ))

# Test application
_OBJ = test_so.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Build shared library
$(SODIR)/%.o: %.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -fpic

$(SOLIB): $(SOBJ)
	$(CC) -shared -o $(SOLIB) $^ $(CFLAGS)

# Build test application
test_so:
	$(CC) -o $@ test_so.c $(LIBS) $(SLIBS) -L./
export LD_LIBRARY_PATH=/Development/miiboo_controller


.PHONY: clean

clean:
	rm -f $(SODIR)/*.o
	rm $(SOLIB)
	rm test_so

