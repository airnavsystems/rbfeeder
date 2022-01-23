/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef PROC_PACKETS_H
#define PROC_PACKETS_H
//#include "airnav.h"
#include <sys/utsname.h>
#include "rbfeeder.pb-c.h"
//#include "rbfeeder.h"
//#include "airnav_net.h"
#include "airnav_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    enum messageTypes {
        AUTH_FEEDER = 1,
        SERVER_REPLY_STATUS = 2,
        PINGPONG = 3,
        FLIGHT_PACKET = 4,
        SYSINFO = 5,
        CLIENT_STATS = 6,
        SK_REQUEST = 7,
        CTR_CMD = 8
    };

    struct prepared_packet {
        char *buf;
        unsigned len;
        enum messageTypes type;
    };

    
    
    
    /****** Functions *******/
    void proccess_packet(char *packet, unsigned short p_size);
    struct prepared_packet *create_packet_AuthFeederRequest(char *sk, ClientType client_type, char *serial);
    struct prepared_packet *create_packet_SK_Request(ClientType client_type, char *serial);
    void proccess_ServerReplyPacket(uint8_t *packet, unsigned p_size);
    struct prepared_packet *create_packet_Ping(int ping_id);
    void sendMultipleFlights(packet_list *flights, unsigned qtd);
    struct prepared_packet *create_packet_SysInfo(struct utsname *sysinfo);


#ifdef __cplusplus
}
#endif

#endif /* PROC_PACKETS_H */

