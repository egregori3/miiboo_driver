// Written by Eric Gregori
// Building the shared library
// gcc -c -Wall -Werror -fpic miiboo_driver.c
// gcc -shared -o libmiiboo.so miiboo_driver.o

// Linking with the shared library
// gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./

int Close(void);
int Read( char *result);
int Write(char *cmd);
int Open( char *COMName);
