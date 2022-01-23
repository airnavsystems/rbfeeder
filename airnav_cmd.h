/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_CMD_H
#define AIRNAV_CMD_H
#include <stdio.h>
#include <stdint.h>

#include "rbfeeder.h"


#ifdef __cplusplus
extern "C" {
#endif


    
    /****** Functions ******/
    void cmd_proccess_ctr_cmd_packet(uint8_t *packet, unsigned p_size);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_CMD_H */

