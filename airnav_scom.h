/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#ifndef AIRNAV_SCOM_H
#define AIRNAV_SCOM_H

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


    void serial_loadSerialConfig(void);    
    int serial_set_interface_attribs(int fd, int speed);    
    void *serial_threadGetSerialData(void *argv);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_SCOM_H */

