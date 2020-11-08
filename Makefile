# Building the shared library
# gcc -c -Wall -Werror -fpic miiboo_driver.c
# gcc -shared -o libmiiboo.so miiboo_driver.o

# Linking with the shared library
# gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./



IDIR=./
CC=gcc
CLAGS=-Wall -Werror

SODIR=sobj
SLDIR =./
SOLIB=libmiiboo.so

ODIR=obj
LDIR=./
LIBS=-lpthread -lmiiboo


# Shared Library
_SDEPS = miiboo_driver.h
SDEPS = $(patsubst %,$(IDIR)/%,$(_SDEPS))

_SOBJ = miiboo_driver.o
_SOBJ = $(patsubst %,$(SODIR)/%,$(_SOBJ))

# Test application
_OBJ = test_so.o
_OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Build shared library
$(SODIR)/%.o: %.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -fpic

miiboo_driver: $(OBJ)
	$(CC) -shared -o $(SOLIB) $^ $(CFLAGS) $(LIBS)

# Build test application
$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -fpic

test_so: $(OBJ)
	$(CC) -shared -o $(SOLIB) $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
	rm -f $(SODIR)/*.o
	rm $(SOLIB)


