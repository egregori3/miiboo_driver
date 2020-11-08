/******************************************************************

Based on the miiboo ROS driver from Hiram Zhang.
https://files.cnblogs.com/files/hiram-zhang/miiboo_bringup.zip
gcc miiboo_driver.c -lpthread

Building the shared library
gcc -c -Wall -Werror -fpic miiboo_driver.c
gcc -shared -o libmiiboo.so miiboo_driver.o

Linking with the shared library
gcc -Wall -o test test_so.c -lmiiboo -lpthread -L./

Running
export LD_LIBRARY_PATH=/Development/miiboo_controller
sudo chmod 777 /dev/ttyUSB0
./test /dev/ttyUSB0

********************************************************************
###serial-com interface (USB-DATA)：
1.send to serial-com
(1)bps: 115200
(2)data_frame_freq: <200hz
(3)protocol
top  top  sig1    enc1        sig2    enc2        checksum
0xff 0xff 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x??
2.recieve from serial-com
(1)bps: 115200
(2)data_frame_freq:reference by encode_sampling_time
(3)protocol
top  top  sig1    enc1        sig2    enc2        checksum
0xff 0xff 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x??

###motor params:
speed_ratio:          (unit: m/encode)    
wheel_distance:       (unit: m)
encode_sampling_time: (unit: s)
*******************************************************************/
/******************************************************************
###serial-com interface (USB-DEBUG):
1.send to serial-com
(1)bps: 115200
(2)data_frame_freq: <200hz
(3)protocol
top  top  sig_Kp       Kp       sig_Ki       Ki       sig_Kd       Kd       checksum
0xff 0xff 0x??   0x?? 0x?? 0x?? 0x??   0x?? 0x?? 0x?? 0x??   0x?? 0x?? 0x?? 0x??
(
case1:version and pid_params request(0x00 0x00 ... 0x00)
case2:reset pid_params to default request(0xff 0xff ... 0xff)
case3:set pid_params(0x?? 0x?? ... 0x??)
)
2.recieve from serial-com
(1)bps: 115200
(2)data_frame_freq: none
(3)protocol
char printf() one-by-one

*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>


static void *myreadframe_thread();
static void write_to_motor(int32_t left, int32_t right);

// Driver Data
typedef struct data
{
    // Serial com
    int SerialCom;
    unsigned char insert_buf;
    // Buffers
    unsigned char readbuff[11];
    unsigned char writebuff[11];
    unsigned char recv_update_flag;
    unsigned char send_update_flag;
    int32_t       left_distance;
    int32_t       right_distance;
    // Thread
    unsigned char running;
    pthread_t rx_thread;
    pthread_mutex_t rx_data_lock;
} miiboo_driver;

static miiboo_driver miiboo_data;

//
//
//                   Public API
//
//

// Open motor controller
int Open( char *COMName)
{
    // Serial com
    miiboo_data.SerialCom        = 0;
    miiboo_data.insert_buf       = 0;
    miiboo_data.recv_update_flag = 0;
    miiboo_data.send_update_flag = 0;
    miiboo_data.running          = 1;

    printf("Opening the comm port: %s\n", COMName);
    miiboo_data.SerialCom = open(COMName, O_RDWR); //set name of serial-com
    if(miiboo_data.SerialCom == -1)
    {
        printf("Can't open serial port!\n");
        return -1;
    }

    printf("Read the current serial port configuration\n");
    struct termios options;
    if(tcgetattr(miiboo_data.SerialCom, &options) != 0)
    {
        printf("Can't get serial port sets!\n");
        return -2;
    }

    printf("Modify the options required for the hardware\n");
    // https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
    tcflush(miiboo_data.SerialCom, TCIFLUSH);
    cfsetispeed(&options, B115200);   //set recieve bps of serial-com
    cfsetospeed(&options, B115200);   //set send bps of serial-com
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    options.c_oflag &= ~(ONLCR | OCRNL);

    printf("Update the serial port configuration with the new options\n");
    if(tcsetattr(miiboo_data.SerialCom, TCSANOW, &options) != 0)
    {
        printf("Can't set serial port options!\n");
        return -3;
    }

    (void)pthread_create( &miiboo_data.rx_thread, NULL, myreadframe_thread, NULL);

    return 0;
}

// Write to motor controller
int Write(char *cmd)
{
    switch(cmd[0])
    {
        case 'f':
            write_to_motor(15, 15);
            return 0;
        case 'b':
            write_to_motor(-15, -15);
            return 0;
        case 'r':
            write_to_motor(15, -15);
            return 0;
        case 'l':
            write_to_motor(-15, 15);
            return 0;
        case 's':
            write_to_motor(0, 0);
            return 0;
        default:
            write_to_motor(0, 0);
    }

    return -1;
}

// Read encoder data from motor controller
int Read(char *result)
{
    pthread_mutex_lock (&miiboo_data.rx_data_lock);
    sprintf(result, "left:%d right:%d", miiboo_data.left_distance, miiboo_data.right_distance);
    pthread_mutex_unlock (&miiboo_data.rx_data_lock);
    return 0;
}

// Close motor controller
int Close(void)
{
    miiboo_data.running = 0;
    close(miiboo_data.SerialCom);
    pthread_join( miiboo_data.rx_thread, NULL);
    return 0;
}


//
//
//                   Static Functions
//
//
static void write_to_motor(int32_t left, int32_t right)
{
    unsigned char check_sum=0;
    unsigned char lsign, rsign;

    // Init 
    lsign = 1;
    rsign = 1;
    // Header
    miiboo_data.writebuff[0] = 0xff;
    miiboo_data.writebuff[1] = 0xff;

    // Left
    if( left < 0 )
    {
        lsign = 0;
        left = -1 * left;
    }
    miiboo_data.writebuff[2] = lsign;
    miiboo_data.writebuff[3] = ((left>>16)&0xff);
    miiboo_data.writebuff[4] = ((left>>8)&0xff);
    miiboo_data.writebuff[5] = (left&0xff);

    // Right
    if( right < 0 )
    {
        rsign = 0;
        right = -1 * right;
    }
    miiboo_data.writebuff[6] = rsign;
    miiboo_data.writebuff[7] = ((right>>16)&0xff);
    miiboo_data.writebuff[8] = ((right>>8)&0xff);
    miiboo_data.writebuff[9] = (right&0xff);

    // Checksum
    for(int i=0;i<10;i++)
        check_sum+=miiboo_data.writebuff[i];
    miiboo_data.writebuff[10] = check_sum;

    // Write to serial port
    write(miiboo_data.SerialCom,(const void *)&miiboo_data.writebuff,11);

    //debug
    printf("\n TX:");
    for(int i=0; i<10; ++i)
        printf("%02x,", miiboo_data.writebuff[i]);
}


//thread: read odom from serial-com
static void *myreadframe_thread(void)
{
    int32_t left, right;

    while( miiboo_data.running && (read(miiboo_data.SerialCom,&(miiboo_data.insert_buf),1)>0) ) //get 1byte by 1byte from serial buffer 
    {
        //debug:print recieved data 1byte by 1byte
        // printf("%02x\n",insert_buf);
      
        //FIFO queue cache
        for(int i=0;i<10;i++)
        {
            miiboo_data.readbuff[i]=miiboo_data.readbuff[i+1];
        }
        miiboo_data.readbuff[10]=miiboo_data.insert_buf;

        //data analysis
        if(miiboo_data.readbuff[0]==0xff && miiboo_data.readbuff[1]==0xff) //top of frame
        {
            //check sum
            unsigned char check_sum=0;
            for(int i=0;i<10;i++)
              check_sum+=miiboo_data.readbuff[i];
            if(check_sum==miiboo_data.readbuff[10])
            {
                /*
                printf("\nRX: ");
                for(int i=0; i<10; ++i)
                    printf("%02x", miiboo_data.readbuff[i]);
                */
                right =  miiboo_data.readbuff[3]*0x00010000;
                right += miiboo_data.readbuff[4]*0x00000100;
                right += miiboo_data.readbuff[5]*0x00000001;
                if(!miiboo_data.readbuff[2])
                    right *= -1;
                left =  miiboo_data.readbuff[7]*0x00010000;
                left += miiboo_data.readbuff[8]*0x00000100;
                left += miiboo_data.readbuff[9]*0x00000001;
                if(!miiboo_data.readbuff[6])
                    left *= -1;
                pthread_mutex_lock (&miiboo_data.rx_data_lock);
                miiboo_data.left_distance = left;
                miiboo_data.right_distance = right;
                pthread_mutex_unlock (&miiboo_data.rx_data_lock);
            }
        }
    }

    pthread_exit((void*) 0);
} // void myreadframe_thread(void) 



#if 0
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
            if(Write((unsigned char *)&cmd[index]))
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
#endif
