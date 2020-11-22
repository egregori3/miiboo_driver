// Written by Eric Gregori

#include "miiboo_driver_class.h"

// Testing the driver
int main(int argc, char **argv)
{
    // Verify command line
    if( argc != 2 )
    {
        printf("\n\nCommand line\n\n");
        printf("%s %s", argv[0], "/dev/ttyUSB?\n\n");
        return -1;
    }

    // Instantiate driver on the heap

    printf("\n\nInstantiating driver\n");
    miiboo_driver *miiboo_object = new miiboo_driver(argv[1]);

    printf("Insert your code here\n");

#if 1
    for(int i=0; i<15; ++i)
    {
        miiboo_object->move((unsigned char *)"mGG");
        sleep(1);
    }
#endif
#if 0    
    for(int i=0; i<15; ++i)
    {
        miiboo_object->move((unsigned char *)"f");
        sleep(1);
    }
    for(int i=0; i<15; ++i)
    {
        miiboo_object->move((unsigned char *)"f");
        sleep(1);
    }
#endif

    miiboo_object->move((unsigned char *)"s");
 
    delete miiboo_object;
} // main()
