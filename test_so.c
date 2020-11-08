// Building the shared library
// gcc -c -Wall -Werror -fpic miiboo_driver.c
// gcc -shared -o libmiiboo.so miiboo_driver.o

// Linking with the shared library
// gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./
#include <unistd.h>
#include <stdio.h>

extern int Close(void);
extern int Read(char *result);
extern int Write(char *cmd);
extern int Open( char *COMName);


// Testing the driver
int main(int argc, char **argv)
{
    char cmd[] = {'f', 'r', 'b', 'l', 's', 0};
    char data[32];

    // Verify command line
    if( argc != 2 )
    {
        printf("\n\nCommand line\n\n");
        printf("%s %s", argv[0], "/dev/ttyUSB?\n\n");
        return -1;
    }

    // Instantiate driver on the heap

    printf("\n\nInstantiating driver\n");
    if( Open(argv[1]) )
    {
        printf("ERROR! opening");
        goto done;
    }

    for(int index=0; cmd[index]; ++index)
    {
        for(int i=0; i<5; ++i)
        {
            if(Write((char *)&cmd[index]))
            {
                printf("ERROR! writing");
                goto done;
            }        
            sleep(1);
            if(Read(data))
            {
                printf("ERROR! reading");
                goto done;
            }
            printf("%s", data);
        }
    }

done:
    Close();
} // main()
