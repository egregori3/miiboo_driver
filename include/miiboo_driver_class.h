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
//cos,sin$
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
        miiboo_driver(char *serialPort);
        // Destructor
        ~miiboo_driver();
        void move(unsigned char *cmd);

    private:
        // Open the comm port and configure for miiboo hardware communications
        void ComSetup(char *COMName);

        //thread: read odom from serial-com
        void myreadframe_thread(void);

        //thread: write motor control to serial-com
        void mywriteframe_thread(void);

        void write_to_motor(int32_t left, int32_t right);
}; // class
