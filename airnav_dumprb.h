/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#ifndef ARNAV_DUMPRB_H
#define ARNAV_DUMPRB_H
#include "rbfeeder.h"


#ifdef __cplusplus
extern "C" {
#endif

    /****** Variables ******/
    extern char *dumprb_cmd;
    extern pid_t p_dumprb;
    extern int dump_agc;
    extern double dump_gain;

    

    /****** Functions ******/
    int dumprb_checkDumprbRunning(void);
    void dumprb_stopDumprb(void);
    void dumprb_startDumprb(void);
    void dumprb_sendDumpConfig(void);
    void dumprb_restartDump();
    


#ifdef __cplusplus
}
#endif

#endif /* ARNAV_DUMPRB_H */

