/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_MAIN_H
#define AIRNAV_MAIN_H
#include "rbfeeder.h"




#ifdef __cplusplus
extern "C" {
#endif

       
    
    /****** Functions ******/
    void airnav_loadConfig(int argc, char **argv);
    void airnav_showHelp(void);
    void airnav_main(void);
    void airnav_init_mutex(void);
    void airnav_create_thread(void);
    void *airnav_monitorConnection(void *arg);
    void *airnav_statistics(void *arg);
    void *airnav_send_stats_thread(void *argv);
    void *airnav_threadSendData(void *argv);
    void *airnav_prepareData(void *arg);
    char *airnav_generateStatusJson(const char *url_path, int *len);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_MAIN_H */

