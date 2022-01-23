/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */

#ifndef AIRNAV_ACARS_H
#define AIRNAV_ACARS_H

#include "rbfeeder.h"

#ifdef __cplusplus
extern "C" {
#endif

    /******* Variables ******/
    extern pid_t p_acars;
    extern char *acars_pidfile;
    extern char *acars_cmd;
    extern char *acars_server;
    extern int acars_device;
    extern char *acars_freqs;
    extern int autostart_acars;



    /****** Functions ******/
    void acars_restartACARS();
    void acars_stopACARS(void);
    void acars_startACARS(void);
    int acars_checkACARSRunning(void);
    void acars_sendACARSConfig(void);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_ACARS_H */

