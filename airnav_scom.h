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
    int serial_set_interface_attribs(int fd, int canonical);    
    void *serial_threadGetSerialData(void *argv);
    int serial_disableData(void);
    int serial_enableData(void);
    struct sdongle_version *serial_getDongleVersion(char disable_data);
    int serial_getLedstatus(char disable_data);
    int serial_setLedstatus(char led_status, char disable_data);
    int serial_getAttstatus(char disable_data);
    int serial_setAttstatus(char att_status, char disable_data);
    int serial_getBiaststatus(char disable_data);
    int serial_setBiaststatus(char biast_status, char disable_data);
    


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_SCOM_H */

