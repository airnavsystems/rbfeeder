/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_MLAT_H
#define AIRNAV_MLAT_H

#include "rbfeeder.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    /****** Defines *******/
    #define DEFAULT_MLAT_SERVER "mlat1.rb24.com:40900"

    /****** Variables ******/
    // MLAT
    extern pid_t p_mlat;
    extern char *mlat_cmd;
    extern char *mlat_server;
    extern char *mlat_pidfile;
    extern int autostart_mlat;
    extern char *mlat_config;
    extern char *mlat_input_type;

    
    
    /****** Functions ******/
    int mlat_checkMLATRunning(void);
    void mlat_stopMLAT(void);
    void mlat_startMLAT(void);
    void mlat_restartMLAT();
    void mlat_sendMLATConfig(void);
    int mlat_saveMLATConfig(void);
    int mlat_check_is_mlat(struct aircraft *a);



#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_MLAT_H */

