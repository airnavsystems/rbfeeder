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
    ini_getString(&serial_device, configuration_file, "serial", "serial_device", NULL);
    serial_old_firmware = ini_getBoolean(configuration_file,"serial","old_firmware",0);
    serial_speed = ini_getInteger(configuration_file, "serial", "serial_speed", 921600);
    serial_use_att = ini_getBoolean(configuration_file, "serial", "attenuator", 0);
    serial_bias_t = ini_getBoolean(configuration_file, "serial", "biast", 0);

    if (serial_device != NULL) {
        airnav_log("Using local serial device at '%s', baud rate '%d'\n", serial_device, serial_speed);
    }

}

int serial_set_interface_attribs(int fd, int canonical) {
    pthread_mutex_lock(&m_serial);
    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        pthread_mutex_unlock(&m_serial);
        return -1;
    }


    switch (serial_speed) {

        case 57600:
            //serial_set_interface_attribs(fd_serial, B57600, 1);
            cfsetospeed(&tty, (speed_t) B57600);
            cfsetispeed(&tty, (speed_t) B57600);
            airnav_log_level(2, "Serial device speed set to 57600\n");
            break;

        case 115200:
            //serial_set_interface_attribs(fd_serial, B115200, 1);
            cfsetospeed(&tty, (speed_t) B115200);
            cfsetispeed(&tty, (speed_t) B115200);
            airnav_log_level(2, "Serial device speed set to 115200\n");
            break;

        case 230400:
            //serial_set_interface_attribs(fd_serial, B230400, 1);
            cfsetospeed(&tty, (speed_t) B230400);
            cfsetispeed(&tty, (speed_t) B230400);
            airnav_log_level(2, "Serial device speed set to 230400\n");
            break;

        case 460800:
            //serial_set_interface_attribs(fd_serial, B460800, 1);
            cfsetospeed(&tty, (speed_t) B460800);
            cfsetispeed(&tty, (speed_t) B460800);
            airnav_log_level(2, "Serial device speed set to 460800\n");
            break;

        case 500000:
            //serial_set_interface_attribs(fd_serial, B500000, 1);
            cfsetospeed(&tty, (speed_t) B500000);
            cfsetispeed(&tty, (speed_t) B500000);
            airnav_log_level(2, "Serial device speed set to 500000\n");
            break;

        case 576000:
            //serial_set_interface_attribs(fd_serial, B576000, 1);
            cfsetospeed(&tty, (speed_t) B576000);
            cfsetispeed(&tty, (speed_t) B576000);
            airnav_log_level(2, "Serial device speed set to 576000\n");
            break;

        case 921600:
            //serial_set_interface_attribs(fd_serial, B921600, 1);
            cfsetospeed(&tty, (speed_t) B921600);
            cfsetispeed(&tty, (speed_t) B921600);
            airnav_log_level(2, "Serial device speed set to 921600\n");
            break;


        default:
            //serial_set_interface_attribs(fd_serial, B921600, 1);
            cfsetospeed(&tty, (speed_t) B921600);
            cfsetispeed(&tty, (speed_t) B921600);
            airnav_log_level(2, "Serial device speed set to 921600\n");

    }



    //cfsetospeed(&tty, (speed_t)speed);
    //cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; /* 8-bit characters */
    tty.c_cflag &= ~PARENB; /* no parity bit */
    tty.c_cflag &= ~CSTOPB; /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

    if (canonical == 1) {
        airnav_log_level(1, "Interface in canonical mode\n");
        //sertial_setCanonical(fd, 1,&tty);
        //tty.c_lflag |= ICANON | ISIG; /* canonical input */
        tty.c_lflag |= ICANON; /* canonical input */
        tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN | ISIG);
    } else {
        airnav_log_level(1, "Interface in non-canonical mode\n");
        //sertial_setCanonical(fd, 0,&tty);
        //Disable ECHO
        tty.c_lflag &= ~(ICANON);
        tty.c_lflag &= ~(ECHO | ECHOE | ISIG);
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 10;

    }

    //tty.c_iflag &= ~IGNCR;  /* preserve carriage return - estava comentado */ 
    tty.c_iflag &= ~INPCK;
    tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* no SW flowcontrol */

    tty.c_oflag &= ~OPOST;

    tty.c_cc[VEOL] = 0;
    tty.c_cc[VEOL2] = 0;
    tty.c_cc[VEOF] = 0x04;



    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        pthread_mutex_unlock(&m_serial);
        return -1;
    }
    pthread_mutex_unlock(&m_serial);
    return 0;
}

void *serial_threadGetSerialData(void *argv) {

    AN_NOTUSED(argv);
    // signal handlers:
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    int s;
    /* Set cancel type to asynchronous
     * This will enable thread cancel at any time
     */
    s = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (s != 0) {
        airnav_log("Can't set thread cancel type\n");
    }


    if (pthread_mutex_init(&m_serial, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    if (serial_device == NULL) {
        airnav_log("Can't start serial communication. Device is null.\n");
        return NULL;
    }


    fd_serial = open(serial_device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_serial < 0) {
        airnav_log("Error opening %s: %s\n", serial_device, strerror(errno));
        return NULL;
    }

    if (serial_old_firmware == 0) {
        airnav_log_level(1, "Disabling serial dongle data\n");
        serial_disableData();

        // Configue LED and other settings 
        airnav_log_level(1, "Getting serial dongle FW version.\n");
        struct sdongle_version *tmp = serial_getDongleVersion(0);
        if (tmp == NULL) {
            airnav_log_level(1, "Function to get dongle FW version returned NULL!\n");
        } else {
            airnav_log("Serial dongle FW version: %d.%d\n", tmp->major, tmp->minor);
        }

        int ret = -1;
        airnav_log_level(1, "Setting serial dongle LED to 3.\n");
        ret = serial_setLedstatus(3, 0);
        if (ret != 3) {
            airnav_log("Error setting serial dongle LED (%d)\n", ret);
        }

        if (serial_use_att == 1) {
            airnav_log_level(1, "Setting serial dongle attenuator to ON\n");
            if (serial_setAttstatus(2, 0) != 2) {
                airnav_log("Error setting serial dongle attenuator.\n");
            }
        } else {
            airnav_log_level(1, "Setting serial dongle attenuator to OFF\n");
            if (serial_setAttstatus(0, 0) != 0) {
                airnav_log("Error setting serial dongle attenuator.\n");
            }
        }

        if (serial_bias_t == 1) {
            airnav_log_level(1, "Setting serial dongle Bias-T do ON\n");
            if (serial_setBiaststatus(1, 0) != 1) {
                airnav_log("Error setting serial dongle Bias-T.\n");
            }
        } else {
            airnav_log_level(1, "Setting serial dongle Bias-T do OFF\n");
            if (serial_setBiaststatus(0, 0) != 0) {
                airnav_log("Error setting serial dongle Bias-T.\n");
            }
        }

        airnav_log_level(1, "Enabling serial dongle data\n");
        serial_enableData();
    }

    /* simple canonical input */
    do {
        char buf[150];
        //unsigned char *p;
        int rdlen;

        rdlen = read(fd_serial, buf, sizeof (buf) - 1);
        if (rdlen > 0) {
            buf[rdlen] = 0;
            //printf("Read (main) %d bytes: ", rdlen);
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
        } else { /* rdlen == 0 */
            //printf("(main)Nothing read. EOF?\n");            
        }
        /* repeat read */
    } while (!Modes.exit);

    if (serial_old_firmware == 0) {
        // Disable data
        serial_disableData();

        airnav_log_level(1, "Reseting Serial Dongle options.\n");
        if (serial_setLedstatus(0, 0) != 0) {
            airnav_log("Error reseting serial dongle LED status\n");
        }
        if (serial_setAttstatus(0, 0) != 0) {
            airnav_log("Error setting serial dongle attenuator.\n");
        }
        if (serial_setBiaststatus(0, 0) != 0) {
            airnav_log("Error setting serial dongle Bias-T.\n");
        }
    
        serial_enableData();
        
    }


    if (fd_serial > -1) {
        close(fd_serial);
    }

    pthread_exit(EXIT_SUCCESS);

}

int serial_disableData(void) {

    if (fd_serial < 0) {
        airnav_log("Error disabling serial data: invalid serial connection.\n");
        return -1;
    }

    char disable_data[] = {0xFF, 0x55, 0x09, 0x01};

    serial_set_interface_attribs(fd_serial, 0);
    airnav_log_level(1, "Canonical mode set to 0\n");
    pthread_mutex_lock(&m_serial);

    int wlen = 0;

    wlen = write(fd_serial, disable_data, 4);
    if (wlen != 4) {
        airnav_log("Error from write: %d, %d\n", wlen, errno);
        pthread_mutex_unlock(&m_serial);
        return -1;
    }
    tcdrain(fd_serial); /* delay for output */

    usleep(100000);

    // Clear current buffer
    char buf[9000] = {0};
    int total_cleared = 0;

    int rdlen = 0;
    rdlen = read(fd_serial, buf, sizeof (buf) - 1);
    total_cleared = rdlen;
    airnav_log_level(1, "Clearing buffer. Current counter: %d. Returned: %d\n", total_cleared, rdlen);
    while (rdlen != 0) {
        rdlen = read(fd_serial, buf, sizeof (buf) - 1);
        total_cleared = total_cleared + rdlen;
        usleep(100000);
        airnav_log_level(1, "Clearing buffer. Current counter: %d. Returned: %d\n", total_cleared, rdlen);
    }

    airnav_log_level(1, "Finished sending 'disable_log' command do serial device! Data length in buffer: '%d'\n", total_cleared);
    pthread_mutex_unlock(&m_serial);
    return 1;

}

int serial_enableData(void) {

    if (fd_serial < 0) {
        airnav_log("Error enabling serial data: invalid serial connection.\n");
        return -1;
    }

    serial_set_interface_attribs(fd_serial, 1);
    pthread_mutex_lock(&m_serial);
    char disable_data[] = {0xFF, 0x55, 0x09, 0x00};

    int wlen = 0;

    wlen = write(fd_serial, disable_data, 4);
    if (wlen != 4) {
        airnav_log("Error from write: %d, %d\n", wlen, errno);
        pthread_mutex_unlock(&m_serial);
        return -1;
    }
    tcdrain(fd_serial); /* delay for output */

    airnav_log_level(1, "Finished sending 'enable_log' command do serial device!\n");

    pthread_mutex_unlock(&m_serial);
    return 1;

}

struct sdongle_version *serial_getDongleVersion(char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return NULL;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return NULL;
        }
    }

    char disable_data_data[] = {0xFF, 0xAA, 0x0F, 0xFF};
    int wlen = 0;

    wlen = write(fd_serial, disable_data_data, 4);
    if (wlen != 4) {
        airnav_log("Error from write: %d, %d\n", wlen, errno);
        return NULL;
    }
    tcdrain(fd_serial); /* delay for output */

    char buf[50];
    //char *p;
    int rdlen;

    while ((rdlen = read(fd_serial, buf, sizeof (buf) - 1)) < 4 || !Modes.exit) {
        buf[rdlen] = 0;
        usleep(100000);
        //printf("(Version) Read '%d' bytes: ", rdlen);
        if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
            airnav_log_level(1, "Version string found!\n");
            int minor = buf[1] << 2 >> 2;
            int major = buf[1] >> 6;
            airnav_log_level(1, "Minor version: %d, Major version: %d\n", minor, major);

            struct sdongle_version *ver = malloc(sizeof (struct sdongle_version));
            ver->major = major;
            ver->minor = minor;
            return ver;
        }

        /* first display as hex numbers then ASCII */
        //for (p = buf; rdlen-- > 0; p++) {
        //printf(" 0x%x", *p);            
        //}
        //printf("\n    (version)Value: \"%s\"\n\n", buf);

    }

    return NULL;

}

int serial_getLedstatus(char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char led_status_data[] = {0xFF, 0xAA, 0x8, 0xFF};

    int wlen = 0;

    wlen = write(fd_serial, led_status_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    char buf[50] = {0};
    //    char *p = NULL;
    int rdlen = read(fd_serial, buf, sizeof (buf) - 1);
    int tries = 0;
    while (rdlen != 4 && tries <= 5) {
        tries++;
        usleep(100000);

        memset(buf, 0, sizeof (buf));
        rdlen = read(fd_serial, buf, sizeof (buf) - 1);

        // Force rwrite command
        if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
            return buf[1];
        }

    }

    if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
        return buf[1];
    }

    return -1;

}

int serial_setLedstatus(char led_status, char disable_data) {


    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char led_data[] = {0xFF, 0x55, 0x08, led_status};
    int wlen = 0;
    wlen = write(fd_serial, led_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    return serial_getLedstatus(disable_data);
}

int serial_getAttstatus(char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char att_status_data[] = {0xFF, 0xAA, 0x06, 0xFF};

    int wlen = 0;

    wlen = write(fd_serial, att_status_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    char buf[50];
    int rdlen = read(fd_serial, buf, sizeof (buf) - 1);
    int tries = 0;

    while (rdlen != 4 && tries <= 5) {
        tries++;
        usleep(100000);
        memset(buf, 0, sizeof (buf));
        rdlen = read(fd_serial, buf, sizeof (buf) - 1);

        // Force rwrite command
        if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
            return buf[1];
        }
    }

    if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
        return buf[1];
    }
    return -1;

}

int serial_setAttstatus(char att_status, char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char att_data[] = {0xFF, 0x55, 0x06, att_status};
    int wlen = 0;
    wlen = write(fd_serial, att_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    return serial_getAttstatus(disable_data);

}

int serial_getBiaststatus(char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char bias_status_data[] = {0xFF, 0xAA, 0x07, 0xFF};

    int wlen = 0;

    wlen = write(fd_serial, bias_status_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    char buf[50];
    int rdlen = read(fd_serial, buf, sizeof (buf) - 1);
    int tries = 0;

    while (rdlen != 4 && tries <= 5) {
        tries++;
        usleep(100000);
        memset(buf, 0, sizeof (buf));
        rdlen = read(fd_serial, buf, sizeof (buf) - 1);

        // Force rwrite command
        if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
            return buf[1];
        }
    }

    if (rdlen == 4 && buf[0] == 0x23 && buf[2] == 0x3b && buf[3] == 0xa) {
        return buf[1];
    }
    return -1;

}

int serial_setBiaststatus(char biast_status, char disable_data) {

    if (fd_serial < 0) {
        airnav_log("Error getting serial data: invalid serial connection.\n");
        return -1;
    }

    if (disable_data == 1) {
        if (serial_disableData() < 0) {
            airnav_log("Could not disable data\n");
            return -1;
        }
    }

    char biast_data[] = {0xFF, 0x55, 0x07, biast_status};
    int wlen = 0;
    wlen = write(fd_serial, biast_data, 4);
    if (wlen != 4) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd_serial); /* delay for output */

    return serial_getBiaststatus(disable_data);

}
