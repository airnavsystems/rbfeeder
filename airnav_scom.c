/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */

#include "airnav_scom.h"
//#include "dump1090.h"
#include "rbfeeder.h"

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "airnav_net.h"
#include "airnav_utils.h"

void serial_loadSerialConfig(void) {
    ini_getString(&serial_device, configuration_file, "client", "serial_device", NULL);    
    serial_speed = ini_getInteger(configuration_file,"client","serial_speed",921600);
    
    if (serial_device != NULL) {
        airnav_log("Using local serial device at '%s', baud rate '%d'\n", serial_device, serial_speed);
    }
    
}

int serial_set_interface_attribs(int fd, int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }
    
    
    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    tty.c_lflag |= ICANON | ISIG;  /* canonical input */
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN);

    tty.c_iflag &= ~IGNCR;  /* preserve carriage return */
    tty.c_iflag &= ~INPCK;
    tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* no SW flowcontrol */

    tty.c_oflag &= ~OPOST;

    tty.c_cc[VEOL] = 0;
    tty.c_cc[VEOL2] = 0;
    tty.c_cc[VEOF] = 0x04;

    //tty.c_cc[VMIN] = 0;
    //tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}



void *serial_threadGetSerialData(void *argv) {

    AN_NOTUSED(argv);
    
        // signal handlers:
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);
    
    if (serial_device == NULL) {
        airnav_log("Can't start serial communication. Device is null.\n");
        return NULL;
    }
        
    //char *portname = {"/dev/ttyUSB1"};
    int fd;
    

    fd = open(serial_device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        airnav_log("Error opening %s: %s\n", serial_device, strerror(errno));
        return NULL;
    }

    switch (serial_speed) {
        
        case 57600:            
            serial_set_interface_attribs(fd, B57600);
            airnav_log_level(2,"Serial device speed set to 57600\n");
            break;

        case 115200:            
            serial_set_interface_attribs(fd, B115200);
            airnav_log_level(2,"Serial device speed set to 115200\n");
            break;

        case 230400:            
            serial_set_interface_attribs(fd, B230400);
            airnav_log_level(2,"Serial device speed set to 230400\n");
            break;

        case 460800:            
            serial_set_interface_attribs(fd, B460800);
            airnav_log_level(2,"Serial device speed set to 460800\n");
            break;

        case 500000:            
            serial_set_interface_attribs(fd, B500000);
            airnav_log_level(2,"Serial device speed set to 500000\n");
            break;
         
        case 576000:            
            serial_set_interface_attribs(fd, B576000);
            airnav_log_level(2,"Serial device speed set to 576000\n");
            break;
            
        case 921600:            
            serial_set_interface_attribs(fd, B921600);
            airnav_log_level(2,"Serial device speed set to 921600\n");
            break;    
        
            
        default:
            serial_set_interface_attribs(fd, B921600);
            airnav_log_level(2,"Serial device speed set to 921600\n");
            
    }
    

    /* simple output */
    //wlen = write(fd, "Hello!\n", 7);
    //if (wlen != 7) {
    //    printf("Error from write: %d, %d\n", wlen, errno);
    //}
    //tcdrain(fd);    /* delay for output */
    
    /* simple canonical input */
    do {
        char buf[150];
        //unsigned char *p;
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            buf[rdlen] = 0;
            //printf("Read %d bytes: ", rdlen);
            /* first display as hex numbers then ASCII */
            //for (p = buf; rdlen-- > 0; p++) {
            //    printf(" 0x%x", *p);
            //    if (*p < ' ')
            //        *p = '.';   /* replace any control chars */
            //}
            //printf("\n    \"%s\"\n\n", buf);
            //printf("%s", buf);
            
            decodeRaw(buf);
            
            
        } else if (rdlen < 0) {
            airnav_log("Error from read: %d: %s\n", rdlen, strerror(errno));
        } else {  /* rdlen == 0 */
            //printf("Nothing read. EOF?\n");
            usleep(100000);
        }               
        /* repeat read */
    } while (!Modes.exit);    
        
    if (fd > -1) {
        close(fd);
    }
    
    pthread_exit(EXIT_SUCCESS);
    
}

