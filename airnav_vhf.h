/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */

#ifndef AIRNAV_VHF_H
#define AIRNAV_VHF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rbfeeder.h"


    /****** Variables ******/
    extern char *vhf_pidfile;
    extern char *vhf_config_file;
    extern char *ice_host;
    extern char *ice_mountpoint;
    extern int ice_port;
    extern char *ice_user;
    extern char *ice_pwd;
    extern int vhf_device;
    extern double vhf_gain;
    extern int vhf_squelch;
    extern int vhf_correction;
    extern int vhf_afc;
    extern char *vhf_mode;
    extern char *vhf_freqs;
    extern int autostart_vhf;
    extern pid_t p_vhf;
    extern char *vhf_cmd;
    extern char *vhf_stop_cmd;
    extern char *vhf_dongle_serial;
    extern char *liveatc_mount;
    extern char *liveatc_user;
    extern char *liveatc_pwd;
    // Custom audio feed
    extern char *ice_custom_url;
    extern int ice_custom_port;
    extern char *ice_custom_user;
    extern char *ice_custom_pwd;
    extern char *ice_custom_mount;



    // Functions
    int generateVHFConfig();
    void stopVhf(void);
    void startVhf(void);
    int checkVhfRunning(void);
    void restartVhf();
    int loadVhfConfig();
    int saveVhfConfig(void);
    void sendVhfConfig(void);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_VHF_H */

