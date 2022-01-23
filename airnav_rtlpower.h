/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_RTLPOWER_H
#define AIRNAV_RTLPOWER_H

//#include "airnav.h"
#include "rbfeeder.h"
#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

    /****** Defines ******/
#define DEFAULT_RTLPOWER_URL "https://rfsurvey.rb24.com/upload_rtlpower.php"
#define DEFAULT_RTLPOWER_FILE "/tmp/survey.csv"
#define DEFAULT_RTLPOWER_SAMPLERATE "2M"

    /****** Variables ******/    
    extern int rfsurvey_execute;
    extern unsigned int rfsurvey_dongle;
    extern unsigned int rfsurvey_min;
    extern unsigned int rfsurvey_max;
    extern pid_t p_rtlpower;
    extern char *rtlpower_cmd;
    extern char *rfsurvey_url;
    extern char *rfsurvey_file;
    extern double rfsurvey_gain;
    extern char rfsurvey_key[501];
    extern char *rfsurvey_samplerate;

    

    /****** Functions ******/
    void rtlpower_stopRtlpower(void);
    void rtlpower_startRtlpower(void);
    int rtlpower_checkRtlpowerRunning(void);
    
    int rtlpower_curl_send_rtlpower(char *url, char *username, char *userpwd, char *rtl_file);
    size_t rtlpower_read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
    curlioerr rtlpower_my_ioctl(CURL *handle, curliocmd cmd, void *userp);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_RTLPOWER_H */

