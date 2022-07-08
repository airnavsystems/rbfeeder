/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_net.h"
#include "airnav_proc_packets.h"
#include "airnav_utils.h"

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "airnav_sk.h"


char *mac_a = NULL;
char txstart[2] = {'~','#'};
unsigned long global_data_sent;
int airnav_socket = -1;
struct sockaddr_in addr_airnav;


char *airnav_host;
int airnav_port;
int airnav_port_v2;
int airnav_com_inited = 0; // Global variable to say if init comunication is stabilished or not
pthread_mutex_t m_socket; // Mutex socket
unsigned long data_received = 0;
pthread_mutex_t m_packets_counter; //
long packets_total = 0;
long packets_last = 0;
pthread_mutex_t m_cmd; // Mutex copy
ServerReply__ReplyStatus expected;
char expected_arrived;
int expected_id;
char last_cmd;
char *beast_out_port;
char *raw_out_port;
char *beast_in_port;
char *sbs_out_port;
int external_port;
char *external_host;
char *local_input_port;
int anrb_port;

pthread_t t_waitcmd;



/*
 * Connect to airnav server (socket)
 */
int net_connect(void) {
    //
    signal(SIGPIPE, net_sigpipe_handler);
    if (airnav_socket != -1) {
        close(airnav_socket);
    }

    airnav_socket = socket(AF_INET, SOCK_STREAM, 0);
    char *hostname = malloc(strlen(airnav_host) + 1);
    strcpy(hostname, airnav_host);

    char ip[100] = {0};

    if (net_hostname_to_ip(hostname, ip)) { // Error
        airnav_log_level(2, "Could not resolve hostname....using default IP.\n");
        strcpy(ip, "45.63.1.41"); // Default IP
    }
    airnav_log_level(3, "Host %s resolved as %s\n", hostname, ip);
    free(hostname);

    addr_airnav.sin_family = AF_INET;
    addr_airnav.sin_port = htons(airnav_port);
    inet_pton(AF_INET, ip, &(addr_airnav.sin_addr));

    net_enable_keepalive(airnav_socket);

    if (connect(airnav_socket, (struct sockaddr *) &addr_airnav, sizeof (addr_airnav)) != -1) {
        /* Success */
        airnav_log("Connection established.\n");
        airnav_log_level(3, "Connected to %s on port %d\n", airnav_host, airnav_port);
        return 1;
    } else {
        airnav_log("Can't connect to AirNav Server. Retry in %d seconds.\n", AIRNAV_MONITOR_SECONDS);
        airnav_log_level(3, "Can't connect to %s on port %d\n", airnav_host, airnav_port);
        close(airnav_socket);
        airnav_socket = -1;
        return 0;
    }


}

/*
 * Avoid program crash while using invalid socket
 */
void net_sigpipe_handler() {
    airnav_log_level(2, "SIGPIPE caught......but not crashing!\n");
    if (airnav_socket < 0) {
        close(airnav_socket);
        airnav_com_inited = 0;
        airnav_socket = -1;
    }

}

/*
 * Enable keep-alive and other socket options
 */
void net_enable_keepalive(int sock) {
    MODES_NOTUSED(sock);

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof (int));

    int idle = 120;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof (int));

    int interval = 15;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof (int));

    int maxpkt = 20;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof (int));

    int reusable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof (int));

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof (timeout)) < 0)
        airnav_log("Error: setsockopt failed for SO_RCVTIMEO (sock: %d)\n", sock);

    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof (timeout)) < 0)
        airnav_log("Error: setsockopt failed for SO_SNDTIMEO (sock: %d)\n", sock);


}

/*
 * Thread that will wait for any incoming packets
 */
void *net_thread_WaitCmds(void * argv) {
    MODES_NOTUSED(argv);
    signal(SIGPIPE, net_sigpipe_handler);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);


    int read_size;
    char temp_char;
    int buf_idx = 0;
    unsigned short temp_size = 0;
    unsigned short packet_size = 0;
    long long unsigned bytes_garbage = 0;
    char buf[BUFFLEN] = {0};

    while (!Modes.exit) {


        if (airnav_socket != -1) {

            pthread_mutex_lock(&m_socket);
            read_size = recv(airnav_socket, &temp_char, 1, MSG_PEEK | MSG_DONTWAIT);
            pthread_mutex_unlock(&m_socket);


            if (read_size > 0) { // There's data!

                data_received = data_received + read_size;

                pthread_mutex_lock(&m_socket);
                if (buf_idx < BUFFLEN && (read_size = recv(airnav_socket, &temp_char, 1, 0) > 0)) {
                    pthread_mutex_unlock(&m_socket);
                    //last_success_received = (int) time(NULL); // Reset receiving timeout timer
                    buf[buf_idx] = temp_char;
                    buf_idx++;

                    //airnav_log("Received: (dec: %d, Char: %c, buf_idx: %d)\n", temp_char, temp_char, buf_idx);












                    // Check if we have at least 2 bytes in buffer (start of TX)
                    if (buf_idx >= 2) {

                        // check if first 2 bytes is start of tx
                        if (buf[0] == txstart[0] && buf[1] == txstart[1]) {

                            // Do things when TXStarted is already received

                            // check if this is the final byte for packet size (byte number 4)
                            if (buf_idx == 4) {
                                airnav_log_level(6,"Packet Size Received.\n");
                                airnav_log_level(6,"First byte value (unsigned int: %u).\n", buf[buf_idx - 2]);
                                airnav_log_level(6,"Second byte value (unsigned int: %u).\n", buf[buf_idx - 1]);
                                temp_size = (((unsigned short) buf[buf_idx - 2]) << 8) | (0x00ff & buf[buf_idx - 1]);
                                // Check if we have valid packet size. At least 2 bytes of data
                                if (temp_size <= 2) {

                                    airnav_log_level(6,"Invalid packet size received (too small, %u). Reseting buffer.\n", temp_size);
                                    // Increment garbage count for this client
                                    bytes_garbage = bytes_garbage + buf_idx - 1;
                                    memset(&buf, 0, sizeof (buf)); // Clear the buffer
                                    buf_idx = 0;

                                } else {
                                    packet_size = temp_size;
                                    airnav_log_level(6,"Valid packet size received. Value: %u\n", temp_size);
                                }

                            }

                            // If we past the packet size index, start checking total received buffer
                            if (buf_idx > 6) { // start of tx (2-bytes) + packet size (2-bytes) + 2 bytes of data (at least)
                                //airnav_log("Data count in buffer: %d\n", c_list[slot].buf_idx - 4);

                                // Check if we have all needed data and procced to decoding
                                if (packet_size == (buf_idx - 4)) {
                                    airnav_log_level(6,"All data received!! Let's decode...\n");
                                    airnav_log_level(6,"Garbage from this client: %llu\n", bytes_garbage);
                                    // Copy buffer to temporary pointer and send to proccess
                                    char *tmp = NULL;
                                    tmp = malloc(packet_size + 4);
                                    memcpy(tmp, buf, packet_size + 4);
                                    proccess_packet(tmp, packet_size + 4);
                                    // Clear memory buffer
                                    memset(&buf, 0, sizeof (buf)); // Clear the buffer
                                    buf_idx = 0;
                                    temp_char = 0;
                                    packet_size = 0;
                                    temp_size = 0;
                                }

                            }


                        } else {

                            airnav_log_level(6,"Checking for start of packet...start0=>%d, start1=>%d...buffer idx: %d, -1 => %d, -2 => %d\n", txstart[0], txstart[1], buf_idx, buf[buf_idx - 1], buf[buf_idx - 2]);
                            if (buf[buf_idx - 1] == txstart[1] && buf[buf_idx - 2] == txstart[0]) {
                                airnav_log_level(5,"Found start of Transmission bytes!! Position of first start byte: %d\n", buf_idx - 2);

                                // Check if there's garbage before this start of TX
                                if ((buf_idx - 2) > 0) {
                                    // Increment garbage count for this client
                                    bytes_garbage = bytes_garbage + (buf_idx - 2);
                                }

                                // Clear buffer and set first two bytes as tx start
                                memset(&buf, 0, sizeof (buf)); // Clear the buffer
                                buf_idx = 2;
                                buf[0] = txstart[0];
                                buf[1] = txstart[1];


                            }


                        }

                    }


















                } else {
                    pthread_mutex_unlock(&m_socket);
                    airnav_log_level(6, "Buffer is full!\n");
                    memset(&buf, 0, sizeof (buf)); // Clear the buffer
                    buf_idx = 0;
                    temp_char = 0;
                    packet_size = 0;
                    temp_size = 0;
                }

            } else { // No data.....sleep for 100 miliseconds to avoid 100% CPU usage
                usleep(100000);
            }

        } else {
            //airnav_log("Socket inválido!\n");
            usleep(100000);
        }

    }


    airnav_log_level(1, "Exited WaitCmds Successfull!\n");
    pthread_exit(EXIT_SUCCESS);
    //return 0;
}

/*
 * Function that send packets to clients
 */
int net_send_packet(struct prepared_packet *packet) {


    //airnav_log_level(0,"Packet size: %u\n",packet->len);    
    char size[2] = {0};
    int sent_size = 0;
    int total_sent = 0;

    // +1 is for the type of message that is not calculated before
    size[0] = (packet->len + 1) >> 8;
    size[1] = (packet->len + 1) & 0x00ff;
    
    if (packet->type == PINGPONG) {
        airnav_log_level(5, "Sending ping packet.\n");
    }
    
    if ((sent_size = send(airnav_socket, txstart, 2, MSG_NOSIGNAL | MSG_DONTWAIT)) > -1) {
        total_sent = total_sent + sent_size;
    } else {
        airnav_log_level(3, "Error sending data to server\n");
        free(packet->buf);
        free(packet);
        net_force_disconnect();
        return -1;
    }

    if ((sent_size = send(airnav_socket, size, 2, MSG_NOSIGNAL | MSG_DONTWAIT)) > -1) {
        total_sent = total_sent + sent_size;
    } else {
        airnav_log_level(3, "Error sending data to server\n");
        free(packet->buf);
        free(packet);
        net_force_disconnect();
        return -1;
    }

    if ((sent_size = send(airnav_socket, &packet->type, 1, MSG_NOSIGNAL | MSG_DONTWAIT)) > -1) {
        total_sent = total_sent + sent_size;
    } else {
        airnav_log_level(3, "Error sending data to server\n");
        free(packet->buf);
        free(packet);
        net_force_disconnect();
        return -1;
    }

    if ((sent_size = send(airnav_socket, packet->buf, packet->len, MSG_NOSIGNAL | MSG_DONTWAIT)) > -1) {
        total_sent = total_sent + sent_size;
    } else {
        airnav_log_level(3, "Error sending data to server\n");
        free(packet->buf);
        free(packet);
        net_force_disconnect();
        return -1;
    }


    global_data_sent = global_data_sent + total_sent;
    pthread_mutex_lock(&m_packets_counter);
    packets_total++;
    packets_last++;
    pthread_mutex_unlock(&m_packets_counter);
    free(packet->buf);
    free(packet);

    return 1;
}

/*
 * Wait for specific CMD on socket
 */
int net_waitCmd(ServerReply__ReplyStatus cmd, int id) {

    signal(SIGPIPE, net_sigpipe_handler);
    static uint64_t next_update;
    uint64_t now = mstime();
    int timeout = 0;
    int abort = 0;
    next_update = now + 1000;

    if (airnav_socket == -1) {
        airnav_log("Not connected to AirNAv Server\n");
        return 0;
    }

    pthread_mutex_lock(&m_cmd);
    expected = cmd;
    expected_arrived = 0;
    if (id > 0) {
        expected_id = id;
    }
    pthread_mutex_unlock(&m_cmd);

    // Initial com
    while (!abort) {

        if (Modes.exit) {
            abort = 1;
        }

        now = mstime();
        if (now >= next_update) {
            next_update = now + 1000;
            timeout++;
            airnav_log_level(3, "Timeout...%d (of %d)\n", timeout, AIRNAV_WAIT_PACKET_TIMEOUT);
        }

        if (timeout >= AIRNAV_WAIT_PACKET_TIMEOUT) {
            abort = 1;
        }

        pthread_mutex_lock(&m_cmd);
        if (expected_arrived == 1) {
            airnav_log_level(3, "Expected CMD has arrived!\n");
            expected_arrived = 0;
            expected = 0;
            expected_id = 0;
            pthread_mutex_unlock(&m_cmd);
            return cmd;
        }
        pthread_mutex_unlock(&m_cmd);
        usleep(100000);

    }
    airnav_log_level(3, "Expected packet did not arrived :(\n");

    return 0;
}

/*
 * Send ACK and wait for reply from server
 * ACK CMD = 4
 */
int sendPing(void) {
    if (airnav_socket == -1) {
        airnav_log_level(7, "Socket not created!\n");
        return 0;
    }

    struct prepared_packet *ping_packet = create_packet_Ping(1);

    if (!net_send_packet(ping_packet)) {
        airnav_log_level(5, "Error sending ACK packet.\n");
        return -1;
    }

    return 1;

}

/*
 * Force disconnect and reset connection status
 */
void net_force_disconnect(void) {
    close(airnav_socket);
    airnav_socket = -1;
    airnav_com_inited = 0;
    airnav_log_level(3, "Forced disconnection done.\n");
}

/*
 * Prepare a new packet
 */
struct p_data *net_preparePacket_v2(void) {
    struct p_data *pakg = malloc(sizeof (struct p_data));

    pakg->timestp = 0;
    memset(pakg->p_timestamp, 0, 25);
    pakg->cmd = 0;
    pakg->c_version = 0;
    pakg->c_type = 0;
    pakg->c_type_set = 0;
    memset(pakg->c_sn, 0, 30);
    memset(pakg->c_key, 0, 33);
    pakg->c_key_set = 0;
    pakg->c_version_set = 0;
    pakg->modes_addr = 0;
    pakg->modes_addr_set = 0;
    memset(pakg->callsign, 0, 9);
    pakg->callsign_set = 0;
    pakg->altitude = 0;
    pakg->altitude_set = 0;
    pakg->altitude_geo = 0;
    pakg->altitude_geo_set = 0;
    pakg->lat = 0;
    pakg->lon = 0;
    pakg->position_set = 0;
    pakg->heading = 0;
    pakg->heading_set = 0;
    pakg->gnd_speed = 0;
    pakg->gnd_speed_set = 0;
    pakg->ias = 0;
    pakg->ias_set = 0;
    pakg->vert_rate = 0;
    pakg->vert_rate_set = 0;
    pakg->squawk = 0;
    pakg->squawk_set = 0;
    pakg->airborne = 0;
    pakg->airborne_set = 0;
    memset(pakg->payload, 0, 501); // Payload for error and messages
    pakg->payload_set = 0;
    memset(pakg->c_ip, 0, 20);
    pakg->extra_flags = 0;
    pakg->extra_flags_set = 0;
    pakg->is_mlat = 0;
    pakg->is_978 = 0;
    pakg->nav_altitude_fms_set = 0;
    pakg->nav_altitude_fms = 0;
    pakg->nav_altitude_mcp = 0;
    pakg->nav_altitude_mcp_set = 0;
    pakg->nav_heading = 0;
    pakg->nav_heading_set = 0;
    pakg->nav_qnh = 0;
    pakg->nav_qnh_set = 0;
    pakg->nav_altitude_src = 0;
    

    // Weather
    pakg->weather_source = 0;
    pakg->weather_source_set = 0;
    pakg->wind_dir = 0;
    pakg->wind_dir_set = 0;
    pakg->wind_speed = 0;
    pakg->wind_speed_set = 0;
    pakg->temperature = 0;
    pakg->temperature_set = 0;
    pakg->pressure = 0;
    pakg->pressure_set = 0;
    pakg->humidity = 0;
    pakg->humidity_set = 0;


    // MRAR    
    pakg->mrar_wind_speed = 0;
    pakg->mrar_wind_speed_set = 0;
    pakg->mrar_wind_dir = 0;
    pakg->mrar_wind_dir_set = 0;
    pakg->mrar_pressure = 0;
    pakg->mrar_pressure_set = 0;
    pakg->mrar_turbulence = 0;
    pakg->mrar_turbulence_set = 0;
    pakg->mrar_humidity = 0;
    pakg->mrar_humidity_set = 0;
    pakg->mrar_temperature = 0;
    pakg->mrar_temperature_set = 0;
        

    // Nav Modes    
    pakg->nav_modes_autopilot_set = 0;
    pakg->nav_modes_vnav_set = 0;
    pakg->nav_modes_alt_hold_set = 0;
    pakg->nav_modes_aproach_set = 0;
    pakg->nav_modes_lnav_set = 0;
    pakg->nav_modes_tcas_set = 0;

    // New fields - 24-08-2021
    pakg->pos_nic = 0;
    pakg->pos_nic_set = 0;    
    pakg->nic_baro = 0;
    pakg->nic_baro_set = 0;    
    pakg->nac_p = 0;
    pakg->nac_p_set = 0;   
    pakg->nac_v = 0;
    pakg->nac_v_set = 0;
    pakg->sil = 0;
    pakg->sil_set = 0;    
    pakg->sil_type = 0;
    pakg->sil_type_set = 0;
    pakg->c_type = getClientType();
    pakg->gnd_speed_full = 0;
    pakg->gnd_speed_full_set = 0;
    pakg->vert_rate_full = 0;
    pakg->vert_rate_full_set = 0;
    pakg->ias_full = 0;
    pakg->ias_full_set = 0;    
    pakg->heading_full = 0;
    pakg->heading_full_set = 0;
    
    // New fields - 07-07-2022
    pakg->track = 0;
    pakg->track_set = 0;
    pakg->true_heading = 0;
    pakg->true_heading_set = 0;
    pakg->adsb_version = 0;
    pakg->adsb_version_set = 0;
    pakg->adsr_version = 0;
    pakg->adsr_version_set = 0;
    pakg->tisb_version = 0;
    pakg->tisb_version_set = 0;
    pakg->roll = 0;
    pakg->roll_set = 0;
    
    return pakg;
}

/*
 * Return my IP
 */
char *net_getLocalIp() {

    if (airnav_socket >= 0) {
        int err = 0;
        AN_NOTUSED(err);

        struct sockaddr_in name;
        memset(&name, 0, sizeof(struct sockaddr_in));
        
        socklen_t namelen = sizeof (name);
        err = getsockname(airnav_socket, (struct sockaddr*) &name, &namelen);

        char buffer[100] = {0};
        const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

        if (p != NULL) {
            
            //airnav_log_level(1,"Local ip is : %s \n", buffer);
            char *out = malloc(strlen(buffer)+1);
            strcpy(out,buffer);
            return out;
        } else {
            //Some error
            return NULL;
            //printf("Error number : %d . Error message : %s \n", errno, strerror(errno));
        }

    }

    return NULL;

}

/*
 * Initial communication with AirNavServer
 */
int net_initial_com(void) {

    airnav_log_level(7, "Starting initial protocol...\n");
    signal(SIGPIPE, net_sigpipe_handler);
    int reply = -2;

    //sharing_key = ini_getString("client", "key", "");
    ini_getString(&sharing_key, configuration_file, "client", "key", "");

    // Check if sharing-key is valid
    if (getArraySize((char*) sharing_key) > 0 && getArraySize((char*) sharing_key) < 32) {
        airnav_log("Error: invalid sharing-key. Check your key and try again.\n");
        airnav_log("If you don't have a sharing-key, leave field 'key' empty (in rbfeeder.ini) and the feeder will try to auto-generate a new sharing key.\n");
        airnav_com_inited = 0;
        close(airnav_socket);
        airnav_socket = -1;
        return 0;
    }

    if (net_connect() != 1) {
        return 0;
    }

    if (getArraySize((char*) sharing_key) == 0) {


        //#ifndef RBCS
        airnav_log("Empty sharing key. We will try to create a new one for you!\n");
        reply = sendKeyRequest();
        if (reply == 1) {
            ini_getString(&sn, configuration_file, "client", "sn", NULL);
            ini_getString(&sharing_key, configuration_file, "client", "key", NULL);
            //airnav_log_level(5, "New key generated! This is the key: %s\n", sharing_key);            
            if (strlen(sharing_key) == 32) { // Check if is exactly 32 chars on key
                airnav_log("Your new key is %s. Please save this key for future use. You will have to know this key to link this receiver to your account in RadarBox24.com. This key is also saved in configuration file (%s)\n", sharing_key, configuration_file);
                airnav_com_inited = 0;
                close(airnav_socket);
                airnav_socket = -1;
                return 1;
            } else {
                airnav_log("Key received from server is invalid.\n");
                airnav_com_inited = 0;
                close(airnav_socket);
                airnav_socket = -1;
                return 0;
            }

        } else if (reply == 0) {
            airnav_log("Timeout waiting for new key. Will try again in 30 seconds.\n");
            airnav_com_inited = 0;
            close(airnav_socket);
            airnav_socket = -1;
            return 0;
        } else {
            airnav_log("Could not generate new key. Error from server: \n");
            airnav_com_inited = 0;
            close(airnav_socket);
            airnav_socket = -1;
            return 0;
        }

    } else {

        airnav_log_level(7, "Sending sharing key to server...\n");
        reply = sendKey();
        if (reply == 1) {
            airnav_com_inited = 1;
            airnav_log("Connection with RadarBox24 server OK! Key accepted by server.\n");
            if (sn != NULL) {
                if (strlen(sn) > 10) {
                    airnav_log("This is your station serial number: %s\n", sn);
                }
            }

            // Send our system version
            net_sendSystemVersion();

            return 1;
        } else {
            airnav_log("Could not start connection. Timeout.\n");
            if (last_cmd == 3) {
                airnav_log("Last server error: \n");
            }
            airnav_com_inited = 0;
            close(airnav_socket);
            airnav_socket = -1;
            return 0;
        }


    }


    return 1;
}


char *net_get_mac_address(char format_output) {

    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        /* handle error*/
    };

    ifc.ifc_len = sizeof (buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        /* handle error */
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq * const end = it + (ifc.ifc_len / sizeof (struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        } else {
            /* handle error */
        }
    }

    unsigned char *mac_address = malloc(6);

    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

    
    char *out = NULL;
    if (format_output == 1) {
        out = malloc(18);
        memset(out, 0, 18);
        sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    } else {
        out = malloc(13);
        memset(out, 0, 13);
        sprintf(out, "%02x%02x%02x%02x%02x%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    }

    free(mac_address);
    
    return out;

}


/*
 * Resolve hostname to IP
 */
int net_hostname_to_ip(char *hostname, char *ip) {
    //int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, "http", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        //strcpy(ip, inet_ntoa(h->sin_addr));
        if (strcmp(inet_ntoa(h->sin_addr), "0.0.0.0") != 0) {
            strcpy(ip, inet_ntoa(h->sin_addr));
        }
    }

    freeaddrinfo(servinfo); // all done with this structure
    return 0;
}


/*
 * Send system version
 */
void net_sendSystemVersion(void) {

    if (airnav_com_inited != 1) {
        return;
    }

    struct utsname *buf = malloc(sizeof (struct utsname));

    int res = uname(buf);
    if (res != 0) {
        return;
    }

    // Create packet for sysinfo
    struct prepared_packet *sinfo = create_packet_SysInfo(buf);

    net_send_packet(sinfo);


}


/*
 * Function to send statistics
 */
int net_sendStats(void) {

    if (airnav_com_inited == 0) {
        return 0;
    }

    struct stats *st = &Modes.stats_1min[Modes.stats_newest_1min];

    ClientStats cst = CLIENT_STATS__INIT;
    void *buf; // Buffer to store serialized data
    unsigned len = 0; // Length of serialized data

    if (st->samples_processed > 0) {
        cst.samples_processed = st->samples_processed;
        cst.has_samples_processed = 1;
    }
    if (st->messages_total > 0) {
        cst.messages_total = st->messages_total;
        cst.has_messages_total = 1;
    }
    if (st->unique_aircraft > 0) {
        cst.unique_aircraft = st->unique_aircraft;
        cst.has_unique_aircraft = 1;
    }
    if (st->single_message_aircraft > 0) {
        cst.single_message_aircraft = st->single_message_aircraft;
        cst.has_single_message_aircraft = 1;
    }


    //    st = &Modes.stats_1min[Modes.stats_latest_1min];
    airnav_log_level(3, "\n");
    airnav_log_level(3, "************ STATS ************\n");

    airnav_log_level(3, "Local receiver:\n");
    airnav_log_level(3, "  %llu samples processed\n", (unsigned long long) st->samples_processed);
    airnav_log_level(3, "%u total usable messages\n", st->messages_total);
    airnav_log_level(3, "%u unique aircraft tracks\n", st->unique_aircraft);
    airnav_log_level(3, "%u aircraft tracks where only one message was seen\n", st->single_message_aircraft);


    uint64_t demod_cpu_millis = (uint64_t) st->demod_cpu.tv_sec * 1000UL + st->demod_cpu.tv_nsec / 1000000UL;
    uint64_t reader_cpu_millis = (uint64_t) st->reader_cpu.tv_sec * 1000UL + st->reader_cpu.tv_nsec / 1000000UL;
    uint64_t background_cpu_millis = (uint64_t) st->background_cpu.tv_sec * 1000UL + st->background_cpu.tv_nsec / 1000000UL;
    cst.cpu_load = 100.0 * (demod_cpu_millis + reader_cpu_millis + background_cpu_millis) / (st->end - st->start + 1);
    cst.has_cpu_load = 1;

    airnav_log_level(3, "CPU load: %.1f%%\n", 100.0 * (demod_cpu_millis + reader_cpu_millis + background_cpu_millis) / (st->end - st->start + 1));
    airnav_log_level(3, "  %llu ms for demodulation\n", (unsigned long long) demod_cpu_millis);
    airnav_log_level(3, "  %llu ms for reading from USB\n", (unsigned long long) reader_cpu_millis);
    airnav_log_level(3, "  %llu ms for network input and background tasks\n", (unsigned long long) background_cpu_millis);


#ifndef RBCSRBLC
    // Remote - only valid for Client
    if (st->remote_accepted[0] > 0) {
        cst.remote_accepted = st->remote_accepted[0];
        cst.has_remote_accepted = 1;
    }
    airnav_log_level(3, "[remote] %u accepted messages\n", st->remote_accepted[0]);

    if (st->remote_received_modeac > 0) {
        cst.remote_received_modeac = st->remote_received_modeac;
        cst.has_remote_received_modeac = 1;
    }
    airnav_log_level(3, "[remote] %u Mode AC messages\n", st->remote_received_modeac);

    if (st->remote_received_modes > 0) {
        cst.remote_received_modes = st->remote_received_modes;
        cst.has_remote_received_modes = 1;
    }
    airnav_log_level(3, "[remote] %u Mode S messages\n", st->remote_received_modes);

    if (st->remote_rejected_bad > 0) {
        cst.remote_rejected_bad = st->remote_rejected_bad;
        cst.has_remote_rejected_bad = 1;
    }
    airnav_log_level(3, "[remote] %u Rejected messages (bad)\n", st->remote_rejected_bad);

    if (st->remote_rejected_unknown_icao > 0) {
        cst.remote_rejected_unknown_icao = st->remote_rejected_unknown_icao;
        cst.has_remote_rejected_unknown_icao = 1;
    }
    airnav_log_level(3, "[remote] %u unknow ICAO\n", st->remote_rejected_unknown_icao);
#endif

    cst.net_mode = net_mode;
    if (net_mode == 1) {
        cst.has_net_mode = 1;
    }
    airnav_log_level(3, "%u Network mode\n", net_mode);

    cst.cpu_temp = getCPUTemp();
    cst.has_cpu_temp = 1;
    airnav_log_level(3, "%.2f CPU Temp\n", getCPUTemp());

#ifdef RBCSRBLC    
    cst.pmu_temp = getPMUTemp();
    cst.has_pmu_temp = 1;
    airnav_log_level(3, "%.2f PMU Temp\n", getPMUTemp());
#endif

    int vr = checkVhfRunning();
    if (vr == 1) {
        cst.vhf_running = vr;
        cst.has_vhf_running = 1;
    }
    airnav_log_level(3, "VHF Is running: %d\n", vr);

    int mr = mlat_checkMLATRunning();    
    if (mr == 1) {
        cst.mlat_running = mr;
        cst.has_mlat_running = 1;
    }
    airnav_log_level(3, "MLAT Is running: %d\n", mr);

    int dr = uat_check978Running();    
    if (dr == 1) {
        cst.dump978_running = dr;
        cst.has_dump978_running = 1;
    }
    airnav_log_level(3, "Dump978 Is running: %d\n", dr);

    //int ar = acars_checkACARSRunning();
    int ar = 0;
    if (ar == 1) {
        cst.acars_running = ar;
        cst.has_acars_running = 1;
    }
    airnav_log_level(3, "ACARS Is running: %d\n", ar);



    len = client_stats__get_packed_size(&cst);

    buf = malloc(len);
    client_stats__pack(&cst, buf);

    struct prepared_packet *packet = malloc(sizeof (struct prepared_packet));

    packet->buf = buf;
    packet->len = len;
    packet->type = CLIENT_STATS;

    net_send_packet(packet);


    return 1;

}

//
//=========================================================================
//
// Turn an hex digit into its 4 bit decimal value.
// Returns -1 if the digit is not in the 0-F range.
//
static int hexDigitVal(int c) {
    c = tolower(c);
    if (c >= '0' && c <= '9') return c-'0';
    else if (c >= 'a' && c <= 'f') return c-'a'+10;
    else return -1;
}

//
//=========================================================================
//
// This function decodes a string representing message in raw hex format
// like: *8D4B969699155600E87406F5B69F; The string is null-terminated.
//
// The message is passed to the higher level layers, so it feeds
// the selected screen output, the network output and so forth.
//
// If the message looks invalid it is silently discarded.
//
// The function always returns 0 (success) to the caller as there is no
// case where we want broken messages here to close the client connection.
//
static int decodeHexMessage(struct client *c, char *hex) {
    int l = strlen(hex), j;
    unsigned char msg[MODES_LONG_MSG_BYTES];
    struct modesMessage mm;
    static struct modesMessage zeroMessage;
    
    MODES_NOTUSED(c);
    mm = zeroMessage;

    // Mark messages received over the internet as remote so that we don't try to
    // pass them off as being received by this instance when forwarding them
    mm.remote      =    1;
    mm.signalLevel =    0;

    // Remove spaces on the left and on the right
    while(l && isspace(hex[l-1])) {
        hex[l-1] = '\0'; l--;
    }
    while(isspace(*hex)) {
        hex++; l--;
    }

    // Turn the message into binary.
    // Accept *-AVR raw @-AVR/BEAST timeS+raw %-AVR timeS+raw (CRC good) <-BEAST timeS+sigL+raw
    // and some AVR records that we can understand
    if (hex[l-1] != ';') {return (0);} // not complete - abort

    switch(hex[0]) {
        case '<': {
            mm.signalLevel = ((hexDigitVal(hex[13])<<4) | hexDigitVal(hex[14])) / 255.0;
            mm.signalLevel = mm.signalLevel * mm.signalLevel;
            hex += 15; l -= 16; // Skip <, timestamp and siglevel, and ;
            break;}

        case '@':     // No CRC check
        case '%': {   // CRC is OK
            hex += 13; l -= 14; // Skip @,%, and timestamp, and ;
            break;}

        case '*':
        case ':': {
            hex++; l-=2; // Skip * and ;
            break;}

        default: {
            return (0); // We don't know what this is, so abort
            break;}
    }

    if ( (l != (MODEAC_MSG_BYTES      * 2))
      && (l != (MODES_SHORT_MSG_BYTES * 2))
      && (l != (MODES_LONG_MSG_BYTES  * 2)) )
        {return (0);} // Too short or long message... broken

    if ( (0 == Modes.mode_ac)
      && (l == (MODEAC_MSG_BYTES * 2)) )
        {return (0);} // Right length for ModeA/C, but not enabled

    for (j = 0; j < l; j += 2) {
        int high = hexDigitVal(hex[j]);
        int low  = hexDigitVal(hex[j+1]);

        if (high == -1 || low == -1) return 0;
        msg[j/2] = (high << 4) | low;
    }

    // record reception time as the time we read it.
    mm.sysTimestampMsg = mstime();

    if (l == (MODEAC_MSG_BYTES * 2)) {  // ModeA or ModeC
        Modes.stats_current.remote_received_modeac++;
        decodeModeAMessage(&mm, ((msg[0] << 8) | msg[1]));
    } else {       // Assume ModeS
        int result;

        Modes.stats_current.remote_received_modes++;
        result = decodeModesMessage(&mm, msg);
        if (result < 0) {
            if (result == -1)
                Modes.stats_current.remote_rejected_unknown_icao++;
            else
                Modes.stats_current.remote_rejected_bad++;
            return 0;
        } else {
            Modes.stats_current.remote_accepted[mm.correctedbits]++;
        }
    }

    useModesMessage(&mm);
    return (0);
}


struct net_service *makeRawInputService(void)
{
    return serviceInit("Raw TCP input", NULL, NULL, READ_MODE_ASCII, "\n", decodeHexMessage);
}

void decodeRaw(char *hex) {
    decodeHexMessage(NULL,hex);
}