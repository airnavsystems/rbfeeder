/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_SK_H
#define AIRNAV_SK_H
#include "rbfeeder.h"
#include "mode_s.h"

#ifdef __cplusplus
extern "C" {
#endif


    /**** Functions *****/
    int sendKey(void);
    int sendKeyRequest(void);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_SK_H */

