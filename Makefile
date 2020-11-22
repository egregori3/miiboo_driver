# Building the shared library from C
# gcc -c -Wall -Werror -fpic miiboo_driver.c
# gcc -shared -o libmiiboo.so miiboo_driver.o

# Linking with the shared library
# gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./

# Building the archive from cpp
# ar -rc $(SOLIB).a $(SOBJ)

# g++ -I./ test_class.cpp miiboo_driver_class.cpp -lm -lpthread -std=c++11
#

all: libmiiboo.a libmiiboo.so test_so test_class


libmiiboo.a:
	g++ -c -std=c++11 -Wall -Werror -I./include miiboo_driver_class.cpp -o miiboo_driver_class.o
	ar -rc libmiiboo_class.a miiboo_driver_class.o

libmiiboo.so:
	gcc -c -Wall -Werror -fpic -I./include miiboo_driver.c -o miiboo_driver_c.o
	gcc -shared -o libmiiboo.so miiboo_driver_c.o

# Build test application
test_so:
	gcc -I./include -o test_so test_so.c -lmiiboo -lpthread -L./
# export LD_LIBRARY_PATH=/Development/miiboo_controller

# Build test application
test_class:
	g++ -I./include -o test_class test_class.cpp -lmiiboo_class -lpthread -L./
# export LD_LIBRARY_PATH=/Development/miiboo_controller

.PHONY: clean

clean:
	rm *.o || true
	rm *.so || true
	rm *.a || true
	rm test_so || true
	rm test_class

