/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_NET_H
#define AIRNAV_NET_H

#include "rbfeeder.h"
#include "airnav_proc_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

    #define AIRNAV_MONITOR_SECONDS 60 // Check is connection is valid every X seconds
    #define AIRNAV_WAIT_PACKET_TIMEOUT 10 // Wwait X seconds for a packet from server (waiting response)
    #define DEFAULT_AIRNAV_HOST "rpiserver-ng.rb24.com"

    /****** Variables ******/    
    extern char *mac_a;
    extern char txstart[2];
    extern unsigned long global_data_sent;
    extern int airnav_socket;
    extern struct sockaddr_in addr_airnav;

    
    extern char *airnav_host;
    extern int airnav_port;
    extern int airnav_port_v2;
    extern int airnav_com_inited;
    extern pthread_mutex_t m_socket;
    extern unsigned long data_received;
    extern pthread_mutex_t m_packets_counter;
    extern long packets_total;
    extern long packets_last;
    extern pthread_mutex_t m_cmd;
    extern ServerReply__ReplyStatus expected;
    extern char expected_arrived;
    extern int expected_id;
    extern char last_cmd;
    
    extern char *beast_out_port;
    extern char *raw_out_port;
    extern char *beast_in_port;
    extern char *sbs_out_port;
    
    extern int external_port;
    extern char *external_host;
    extern char *local_input_port;
    extern int anrb_port;
    extern pthread_t t_waitcmd;





    /*** Functions *****/
    int net_connect(void);
    void net_enable_keepalive(int sock);
    void net_sigpipe_handler();
    void *net_thread_WaitCmds(void * argv);
    int net_send_packet(struct prepared_packet *packet);
    int net_waitCmd(ServerReply__ReplyStatus cmd, int id);
    int sendPing(void);
    void net_force_disconnect(void);
    struct p_data *net_preparePacket_v2(void);
    char *net_getLocalIp();
    int net_initial_com(void);
    char *net_get_mac_address(char format_output);
    int net_hostname_to_ip(char *hostname, char *ip);
    void net_sendSystemVersion(void);
    int net_sendStats(void);
    struct net_service *makeRawInputService(void);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_NET_H */

