/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_proc_packets.h"
#include "dump1090.h"
#include "rbfeeder.pb-c.h"
#include "airnav_utils.h"
#include <sys/utsname.h>
#include <time.h>
#include "airnav_cmd.h"
#include "airnav_net.h"

/*
 * Proccess and identify packet type
 */
void proccess_packet(char *packet, unsigned short p_size) {

    enum messageTypes type = packet[4];

    // Get packet data
    uint8_t *data_buf = malloc(p_size - 5);

    for (int i = 5; i < p_size; i++) {
        //airnav_log("Packet value: %d => %d (%c)\n", i-5, packet[i], packet[i] );
        data_buf[i - 5] = packet[i];
    }

    free(packet);
    airnav_log_level(6, "Packet type: %u\n", type);

    switch (type) {

        case SERVER_REPLY_STATUS:
            airnav_log_level(6,"Packet type: SERVER_REPLY_STATUS.\n");
            proccess_ServerReplyPacket(data_buf, p_size - 5);
            break;

        case CTR_CMD:
            airnav_log_level(6,"Packet type: CTR_CMD.\n");
            cmd_proccess_ctr_cmd_packet(data_buf, p_size - 5);
            break;
            
            
        default:
            airnav_log("Type not identified.\n");

    }


}

/*
 * Proccess server reply packet
 */
void proccess_ServerReplyPacket(uint8_t *packet, unsigned p_size) {
    
    ServerReply *reply;

    reply = server_reply__unpack(NULL, p_size, packet);

    if (reply == NULL) {
        airnav_log("Invalid packet data for ServerReply type.\n");
        net_force_disconnect();
        return;
    }

    airnav_log_level(6,"Server Reply Status received: %d\n", reply->status);

    
    // Valid sharing-key created. Let's store it
    if (reply->status == SERVER_REPLY__REPLY_STATUS__SK_CREATE_OK) {
        if (reply->sk != NULL && reply->sn != NULL) {
            ini_saveGeneric(configuration_file,"client","key",reply->sk);
            ini_saveGeneric(configuration_file,"client","sn",reply->sn);
        } else {
            airnav_log("Missing information from server.\n");
            net_force_disconnect();
        }
    }
    
    // Proc waitCmd
    pthread_mutex_lock(&m_cmd);
    if (expected_id > 0 && reply->has_id) {

        if (reply->status == expected && expected_id == reply->id) {
            expected_arrived = 1;
        }

    } else {
        if (reply->status == expected) {
            expected_arrived = 1;
        }
    }
    pthread_mutex_unlock(&m_cmd);
    

    // Check if ServerReply is AUTH OK
    if (reply->status == SERVER_REPLY__REPLY_STATUS__AUTH_OK) {
        // Save SN into rbfeeder.ini
        if (reply->sn != NULL && strlen(reply->sn) > 10) {
            ini_saveGeneric(configuration_file, "client", "sn", reply->sn);           
            ini_getString(&sn, configuration_file, "client", "sn", NULL);
        }
        
        if (reply->has_client_type) {
            c_type = reply->client_type;            
            // Just for information
            if (reply->client_type == CLIENT_TYPE__RBCS) {
                airnav_log("Client type: RBCS\n");
            } else if (reply->client_type == CLIENT_TYPE__RBLC) {
                airnav_log("Client type: XRange\n");
            } else if (reply->client_type == CLIENT_TYPE__RBLC2) {
                airnav_log("Client type: XRange2\n");
            } else if (reply->client_type == CLIENT_TYPE__RPI) {
                airnav_log("Client type: Raspberry Pi\n");
            } else if (reply->client_type == CLIENT_TYPE__OTHER) {
                airnav_log("Client type: Other\n");    
            } else if (reply->client_type == CLIENT_TYPE__PC_X86) {
                airnav_log("Client type: PC/x86\n");    
            } else if (reply->client_type == CLIENT_TYPE__PC_X64) {
                airnav_log("Client type: PC/x64\n");    
            }
            
        }

        // Set time, if RBCS/RBLC
        if (is_airnav_product() == 1 && reply->has_time == 1) {

            time_t now2 = time(NULL);
            airnav_log_level(3, "Date/Time received: %lu\n", (unsigned long) reply->time);
            airnav_log_level(3, "Current timestamp: %lu\n", (unsigned long) now2);
            int dif = 0;
            dif = (unsigned long) now2 - (unsigned long) reply->time;
            if (dif > 300 || dif < -300) {
                airnav_log("Time difference from server is more than 5 minutes (Dif=%d, Server=%lu, Local=%lu). Setting new local time.\n", dif, (unsigned long) reply->time, (unsigned long) now2);
                char d_cmd[100] = {0};
                sprintf(d_cmd, "date -s \"@%lu\"", (unsigned long) reply->time);
                if (system(d_cmd) == -1) {
					airnav_log("Could not set system time\n");
				}
                if (system("hwclock -uw") == -1) {
					airnav_log("Could not set system time\n");
				}
                airnav_log("Done setting date.\n");
            }

        }


    }

    // Invalid sharing-key
    if (reply->status == SERVER_REPLY__REPLY_STATUS__AUTH_ERROR) {
        if (reply->error_text != NULL) {
            airnav_log("Error authenticating Sharing-Key: %s\n", reply->error_text);
        } else {
            airnav_log("Error authenticating Sharing-Key\n");
        }
        net_force_disconnect();
    }

    // Invalid client-version
    if (reply->status == SERVER_REPLY__REPLY_STATUS__AUTH_CLIENT_MIN_VERSION_ERROR) {
        if (reply->error_text != NULL) {
            airnav_log("Invalid client version: %s\n", reply->error_text);
        } else {
            airnav_log("Invalid client version\n");
        }
        net_force_disconnect();
    }

    
    if (reply->status == SERVER_REPLY__REPLY_STATUS__SK_CREATE_ERROR) {
        if (reply->error_text != NULL) {
            airnav_log("Error creating new Sharing-Key: %s\n", reply->error_text);
        } else {
            airnav_log("Error creating new Sharing-Key\n");
        }
        net_force_disconnect();
    }
    
    
    free(packet);
    server_reply__free_unpacked(reply, NULL);

}

/*
 * Create a packet for feeder authentication request
 */
struct prepared_packet *create_packet_AuthFeederRequest(char *sk, ClientType client_type, char *serial) {

    AuthFeeder auth = AUTH_FEEDER__INIT;
    void *buf; // Buffer to store serialized data
    unsigned len; // Length of serialized data

    auth.sk = sk;
    auth.client_type = client_type;
    //AN_NOTUSED(serial);
    auth.serial = serial;

    // Optional
    auth.client_version = c_version_int;
    auth.has_client_version = 1;

    len = auth_feeder__get_packed_size(&auth);

    buf = malloc(len);
    auth_feeder__pack(&auth, buf);

    struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));

    packet->buf = buf;
    packet->len = len;
    packet->type = AUTH_FEEDER;

    return packet;
}


/*
 * Create packet to request new SK
 */
struct prepared_packet *create_packet_SK_Request(ClientType client_type, char *serial) {
 
    RequestSK request = REQUEST_SK__INIT;
    void *buf; // Buffer to store serialized data
    unsigned len; // Length of serialized data
        
    request.client_type = client_type;

    request.serial = serial;    

    len = request_sk__get_packed_size(&request);

    buf = malloc(len);
    request_sk__pack(&request, buf);

    struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));

    packet->buf = buf;
    packet->len = len;
    packet->type = SK_REQUEST;

    return packet;
}

/*
 * Create ping packet
 */
struct prepared_packet *create_packet_Ping(int ping_id) {

    PingPong ping = PING_PONG__INIT;
    void *buf; // Buffer to store serialized data
    unsigned len; // Length of serialized data

    ping.ping_id = ping_id;

    len = ping_pong__get_packed_size(&ping);

    buf = malloc(len);
    ping_pong__pack(&ping, buf);

    struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));

    packet->buf = buf;
    packet->len = len;
    packet->type = PINGPONG;

    return packet;
}

/*
 * Create SysInfo Packet
 */
struct prepared_packet *create_packet_SysInfo(struct utsname *sysinfo) {

    SysInformation ipacket = SYS_INFORMATION__INIT;
    void *buf; // Buffer to store serialized data
    unsigned len; // Length of serialized data

    ipacket.machine = sysinfo->machine;
    ipacket.nodename = sysinfo->nodename;
    ipacket.release = sysinfo->release;
    ipacket.sysname = sysinfo->sysname;
    ipacket.version = sysinfo->version;

    len = sys_information__get_packed_size(&ipacket);

    buf = malloc(len);
    sys_information__pack(&ipacket, buf);

    struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));

    packet->buf = buf;
    packet->len = len;
    packet->type = SYSINFO;

    free(sysinfo);

    return packet;
}

/*
 * Test function
 */
void sendMultipleFlights(packet_list *flights, unsigned qtd) {

    MODES_NOTUSED(flights);


    FlightPacket fpacket = FLIGHT_PACKET__INIT;
    FlightData **subs;
    void *buf;

    unsigned len, i, number_of_flights = 0;
    subs = malloc(sizeof (FlightData*) * qtd);
    i = 0;
    
    //struct packet_list *first = flights;
    
    while (flights != NULL) {
        
        
        number_of_flights++;
        subs[i] = malloc(sizeof (FlightData));
        flight_data__init(subs[i]);        
        subs[i]->addr = flights->packet->modes_addr;

        if (flights->packet->callsign_set == 1) {
            subs[i]->callsign = strdup(flights->packet->callsign);
        }

        if (flights->packet->altitude_set == 1) {
            subs[i]->altitude = flights->packet->altitude;
            subs[i]->has_altitude = 1;
        }
        
        if (flights->packet->altitude_geo_set == 1) {            
            subs[i]->altitude_geo = flights->packet->altitude_geo;
            subs[i]->has_altitude_geo = 1;
        }

        if (flights->packet->position_set == 1) {
            subs[i]->latitude = flights->packet->lat;
            subs[i]->longitude = flights->packet->lon;
            subs[i]->has_latitude = 1;
            subs[i]->has_longitude = 1;
        }

        if (flights->packet->heading_set == 1) {
            subs[i]->heading = flights->packet->heading;
            subs[i]->has_heading = 1;
        }

        if (flights->packet->gnd_speed_set == 1) {
            subs[i]->gnd_speed = flights->packet->gnd_speed;
            subs[i]->has_gnd_speed = 1;
        }

        if (flights->packet->ias_set == 1) {
            subs[i]->ias = flights->packet->ias;
            subs[i]->has_ias = 1;
        }

        if (flights->packet->vert_rate_set == 1) {
            subs[i]->vert_rate = flights->packet->vert_rate;
            subs[i]->has_vert_rate = 1;
        }

        if (flights->packet->squawk_set == 1) {
            subs[i]->squawk = flights->packet->squawk;
            subs[i]->has_squawk = 1;
        }

        if (flights->packet->airborne_set == 1) {
            subs[i]->airborne = flights->packet->airborne;
            subs[i]->has_airborne = 1;
        }

        if (flights->packet->is_978 == 1) {
            subs[i]->is_978 = 1;
            subs[i]->has_is_978 = 1;
        }

        if (flights->packet->is_mlat == 1) {
            subs[i]->is_mlat = 1;
            subs[i]->has_is_mlat = 1;
        }

        if (flights->packet->nav_altitude_fms_set == 1) {
            subs[i]->nav_altitude_fms = flights->packet->nav_altitude_fms;
            subs[i]->has_nav_altitude_fms = 1;
        }

        if (flights->packet->nav_altitude_mcp_set == 1) {
            subs[i]->nav_altitude_mcp = flights->packet->nav_altitude_mcp;
            subs[i]->has_nav_altitude_mcp = 1;
        }

        if (flights->packet->nav_qnh_set == 1) {
            subs[i]->nav_qnh = flights->packet->nav_qnh;
            subs[i]->has_nav_qnh = 1;
        }

        // Weather
        if (flights->packet->wind_dir_set == 1) {
            subs[i]->wind_dir = flights->packet->wind_dir;
            subs[i]->has_wind_dir = 1;
        }

        if (flights->packet->wind_speed_set == 1) {
            subs[i]->wind_speed = flights->packet->wind_speed;
            subs[i]->has_wind_speed = 1;
        }

        if (flights->packet->temperature_set == 1) {
            subs[i]->temperature = flights->packet->temperature;
            subs[i]->has_temperature = 1;
        }

        if (flights->packet->pos_nic_set == 1) {
            subs[i]->pos_nic = flights->packet->pos_nic;
            subs[i]->has_pos_nic = 1;
        }
        
        if (flights->packet->nic_baro_set == 1) {
            subs[i]->nic_baro = flights->packet->nic_baro;
            subs[i]->has_nic_baro = 1;            
        }
        
        if (flights->packet->nac_p_set == 1) {
            subs[i]->nac_p = flights->packet->nac_p;
            subs[i]->has_nac_p = 1;            
        }
        
        if (flights->packet->nac_v_set == 1) {
            subs[i]->nac_v = flights->packet->nac_v;
            subs[i]->has_nac_v = 1;            
        }
        
        if (flights->packet->sil_set == 1) {
            subs[i]->sil = flights->packet->sil;
            subs[i]->has_sil = 1;            
        }
        
        if (flights->packet->sil_type_set == 1) {
            subs[i]->sil_type = flights->packet->sil_type;
            subs[i]->has_sil_type = 1;            
        }
        
        free(flights->packet);
        struct packet_list *old = flights;
        flights = flights->next;
        free(old);
        i++;

    }

    fpacket.n_fdata = qtd;
    fpacket.fdata = subs;

    len = flight_packet__get_packed_size(&fpacket);
    
    
    if (airnav_com_inited == 1) {

        buf = malloc(len); // Allocate memory     
        flight_packet__pack(&fpacket, buf);
        
        struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));
        packet->buf = buf;
        packet->len = len;
        packet->type = FLIGHT_PACKET;

        if (net_send_packet(packet) == 1 ) {
            // Increase packet counter
            pthread_mutex_lock(&m_packets_counter);
            if (number_of_flights > 1) {
                packets_total = packets_total + (number_of_flights - 1);
                packets_last = packets_last + (number_of_flights - 1);
            } else {
                packets_total = packets_total + number_of_flights;
                packets_last = packets_last + number_of_flights;
            }
            pthread_mutex_unlock(&m_packets_counter);
        }

    }
    

    for (i = 0; i < qtd; i++){
        if (subs[i]->callsign != NULL) {
            free(subs[i]->callsign);
        }
        free(subs[i]);
    }
    free(subs);
}
