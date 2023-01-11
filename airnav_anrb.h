/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#ifndef AIRNAV_ANRB_H
#define AIRNAV_ANRB_H

#include "rbfeeder.h"



#ifdef __cplusplus
extern "C" {
#endif


    extern pthread_mutex_t m_anrb_list;
    extern pthread_t t_anrb;
    extern pthread_t t_anrb_send;
    extern char txend[2];
    extern pthread_mutex_t m_copy2; // Mutex copy
    
    void *anrb_threadWaitNewANRB(void *arg);
    short anrb_getNextFreeANRBSlot(void);
    void *anrb_threadhandlerANRBData(void *socket_desc);
    char *anrb_xorencrypt(char * message, char * key);
    void *anrb_threadSendDataANRB(void *argv);
    int anrb_sendANRBPacket(void *socket_, struct p_data *pac);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_ANRB_H */

