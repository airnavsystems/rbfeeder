/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_UAT_H
#define AIRNAV_UAT_H
#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif

    /****** Variables ******/
    extern pid_t p_978;
    extern char *dump978_cmd;
    extern int autostart_978;
    extern int dump978_enabled;
    extern int dump978_port;
    extern char *dump978_soapy_params;


    /****** Functions ******/
    int uat_check978Running(void);
    void uat_start978(void);
    void uat_stop978(void);
    int uat_store978data(char packet[4096]);
    void *uat_airnav_ext978(void *arg);
    void uat_restart978();


    /****** Threads ******/
    extern pthread_t t_dump978;

#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_UAT_H */

