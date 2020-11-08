/******************************************************************

Based on the miiboo ROS driver from Hiram Zhang.
https://files.cnblogs.com/files/hiram-zhang/miiboo_bringup.zip
g++ miiboo_driver.cpp -lm -lpthread -std=c++11

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

#include <iostream>
#include <thread>
//serial com
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
//cos,sin
#include <math.h>

using namespace std;

// The driver is fully encapsulated in a class
class miiboo_driver
{
    private:
        // Serial com
        int SerialCom;
        unsigned char insert_buf;
        // Using C++11 arrays to take advantage of initialization
        array<unsigned char,11> readbuff;
        array<unsigned char,11> writebuff;
        array<unsigned char,11> stopbuff;
        int nread;
        int nwrite;
        unsigned char recv_update_flag;
        unsigned char send_update_flag;
        // Sensor data
        int delta_encode_left; 
        int delta_encode_right; 
        // motor params
        float speed_ratio; //unit: m/encode
        float wheel_distance; //unit: m
        float encode_sampling_time; //unit: s
        float cmd_vel_linear_max; //unit: m/s
        float cmd_vel_angular_max; //unit: rad/s
        // odom result
        float position_x; //unit: m
        float position_y; //unit: m
        float oriention; //unit: rad
        float velocity_linear; //unit: m/s
        float velocity_angular; //unit: rad/s
        // threads
        int running;
        thread readThread;
        thread writeThread;

    public:
        // Constructor
        miiboo_driver(char *serialPort)
        {
            // Serial com
            SerialCom               = 0;
            readbuff                = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            insert_buf              = 0;
            writebuff               = {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            stopbuff                = {0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
            nread                   = 0;
            nwrite                  = 0;
            recv_update_flag        = 0;
            send_update_flag = 0;
            // Sensor data
            delta_encode_left; 
            delta_encode_right; 
            //motor params
            speed_ratio             = 0.000176; //unit: m/encode
            wheel_distance          = 0.1495; //unit: m
            encode_sampling_time    = 0.04; //unit: s
            cmd_vel_linear_max      = 0.8; //unit: m/s
            cmd_vel_angular_max     = 1.0; //unit: rad/s
            //odom result
            position_x              = 0.0; //unit: m
            position_y              = 0.0; //unit: m
            oriention               = 0.0; //unit: rad
            velocity_linear         = 0.0; //unit: m/s
            velocity_angular        = 0.0; //unit: rad/s

            ComSetup(serialPort);
            if(running)
            {
                readThread = thread(&miiboo_driver::myreadframe_thread, this );
                writeThread = thread(&miiboo_driver::mywriteframe_thread, this );
            }
        }

        // Destructor
        ~miiboo_driver()
        {
            printf("Desctructor\n");
            running = 0;
            readThread.join();
            writeThread.join();
            close(SerialCom);
        }

        void move(unsigned char *cmd)
        {
            if(cmd[0] == 'f')
                write_to_motor(15, 15);
            if(cmd[0] == 'b')
                write_to_motor(-15, -15);
            if(cmd[0] == 'r')
                write_to_motor(15, -15);
            if(cmd[0] == 'l')
                write_to_motor(-15, 15);
            if(cmd[0] == 's')
                write_to_motor(0, 0);
        }

    private:
        // Open the comm port and configure for miiboo hardware communications
        void ComSetup(char *COMName)
        {
            printf("Opening the comm port: %s\n", COMName);
            SerialCom = open(COMName, O_RDWR); //set name of serial-com
            if(SerialCom == -1)
            {
                printf("Can't open serial port!\n");
                return;
            }
       
            printf("Read the current serial port configuration\n");
            struct termios options;
            if(tcgetattr(SerialCom, &options) != 0)
            {
                printf("Can't get serial port sets!\n");
                return;
            }

            printf("Modify the options required for the hardware\n");
            // https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
            tcflush(SerialCom, TCIFLUSH);
            cfsetispeed(&options, B115200);   //set recieve bps of serial-com
            cfsetospeed(&options, B115200);   //set send bps of serial-com
            options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            options.c_oflag &= ~OPOST;
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
            options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
            options.c_oflag &= ~(ONLCR | OCRNL);

            printf("Update the serial port configuration with the new options\n");
            if(tcsetattr(SerialCom, TCSANOW, &options) != 0)
            {
                printf("Can't set serial port options!\n");
                return;
            }

            running = 1;
        } // ComSetup

        //thread: read odom from serial-com
        void myreadframe_thread(void)
        {
                while( running && (nread=read(SerialCom,&(insert_buf),1)>0) ) //get 1byte by 1byte from serial buffer 
                {
                    //debug:print recieved data 1byte by 1byte
                    // printf("%02x\n",insert_buf);
                  
                    //FIFO queue cache
                    for(int i=0;i<10;i++)
                    {
                        readbuff[i]=readbuff[i+1];
                    }
                    readbuff[10]=insert_buf;

                    //data analysis
                    if(readbuff[0]==0xff && readbuff[1]==0xff) //top of frame
                    {
                        //check sum
                        unsigned char check_sum=0;
                        for(int i=0;i<10;i++)
                          check_sum+=readbuff[i];
                        if(check_sum==readbuff[10])
                        {       
                            printf("\nRX: ");
                            for(int i=0; i<10; ++i)
                                printf("%02x", readbuff[i]);
                        }
                    }
                }
        } // void myreadframe_thread(void) 

        //thread: write motor control to serial-com
        void mywriteframe_thread(void)
        {
            int i=0;
            //control freq: 100hz (10ms)
            while(running)
            {
                //control
                if(send_update_flag==1) //get flag
                {
                    nwrite=write(SerialCom,(const void *)&writebuff,11);
                    //debug
                    printf("\n TX:");
                    for(int i=0; i<10; ++i)
                        printf("%02x,", writebuff[i]);
                    send_update_flag=0; //clear flag
                    i=0; //clear stop count
                }
                else if(i==150) //if not input cmd_vel during 1.5s, stop motor
                {
                    //stop
                    printf("\n\nEmergency Stop\n\n");
                    nwrite=write(SerialCom,(const void *)&stopbuff,11);
                    i = 0;
                }

                ++i;
                usleep(10000); //delay 10ms
            }
        }

        void write_to_motor(int32_t left, int32_t right)
        {
            unsigned char check_sum=0;
            unsigned char lsign, rsign;

            if( send_update_flag == 0 )
            {
                // Init 
                lsign = 1;
                rsign = 1;
                // Header
                writebuff[0] = 0xff;
                writebuff[1] = 0xff;

                // Left
                if( left < 0 )
                {
                    lsign = 0;
                    left = -1 * left;
                }
                writebuff[2] = lsign;
                writebuff[3] = ((left>>16)&0xff);
                writebuff[4] = ((left>>8)&0xff);
                writebuff[5] = (left&0xff);

                // Right
                if( right < 0 )
                {
                    rsign = 0;
                    right = -1 * right;
                }
                writebuff[6] = rsign;
                writebuff[7] = ((right>>16)&0xff);
                writebuff[8] = ((right>>8)&0xff);
                writebuff[9] = (right&0xff);

                // Checksum
                for(int i=0;i<10;i++)
                    check_sum+=writebuff[i];
                writebuff[10] = check_sum;
                send_update_flag = 1;
            }
        }

}; // class

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

    delete miiboo_object;
} // main()
