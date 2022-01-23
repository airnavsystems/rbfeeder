/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_anrb.h"


char txend[2] = {'~','*'};

pthread_mutex_t m_anrb_list;
pthread_t t_anrb;
pthread_t t_anrb_send;
pthread_mutex_t m_copy2; // Mutex copy

/*
 * Thread that wait for servers to connect
 */
void *anrb_threadWaitNewANRB(void *arg) {

    MODES_NOTUSED(arg);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    int socket_desc, client_sock, c, slot;
    struct sockaddr_in server, client;
    
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    // Set reusable option
    int iSetOption = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (char*) &iSetOption, sizeof (iSetOption));

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(anrb_port);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
        airnav_log("Bind failed on ANRB channel. Port: %d\n", anrb_port);
        return NULL;
        //  exit(EXIT_FAILURE);
    }

    //Listen
    listen(socket_desc, MAX_ANRB);

    if (fcntl(socket_desc, F_GETFL) & O_NONBLOCK) {
        // socket is non-blocking
        airnav_log_level(2, "Socket is in non-blocking mode!\n");
    } else {
        airnav_log_level(2, "Socket is in blocking mode!\n");
    }

    //Accept and incoming connection
    airnav_log("Socket for ANRB created. Waiting for connections on port %d\n", anrb_port);
    c = sizeof (struct sockaddr_in);

    fd_set readSockSet;
    struct timeval timeout;
    int retval = -1;

    while (!Modes.exit) {


        FD_ZERO(&readSockSet);
        FD_SET(socket_desc, &readSockSet);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;


        retval = select(socket_desc + 1, &readSockSet, NULL, NULL, &timeout);
        if (retval > 0) {
            if (FD_ISSET(socket_desc, &readSockSet)) {
                airnav_log("TCP Client (ANRB) requesting to connect...\n");

                client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) & c);
                slot = anrb_getNextFreeANRBSlot();

                if (slot > -1) {
                    pthread_mutex_lock(&m_anrb_list);
                    *anrbList[slot].socket = client_sock;
                    anrbList[slot].active = 1;
                    pthread_mutex_unlock(&m_anrb_list);
                    airnav_log("[Slot %d] New ANRB connection from IP %s, remote port %d, socket: %d\n", slot, inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_sock);
                    net_enable_keepalive(client_sock);

                    if (pthread_create(&anrbList[slot].s_thread, NULL, anrb_threadhandlerANRBData, (void*) anrbList[slot].socket) < 0) {
                        perror("could not create [anrb] thread");
                    }

                } else {
                    airnav_log("No more slot for anrb connection.\n");
                }


            }

        } else if (retval < 0) {
            airnav_log("Unknow error while waiting for connection...\n");
        }
        usleep(1000);
    }

    airnav_log_level(1, "Exited WaitNewANRB Successfull!\n");
    pthread_exit(EXIT_SUCCESS);

}


short anrb_getNextFreeANRBSlot(void) {

    for (int i = 0; i < MAX_ANRB; i++) {
        if (anrbList[i].active == 0) {
            return i;
        }
    }

    return -1;
}


/*
 * This will handle server connections
 * */
void *anrb_threadhandlerANRBData(void *socket_desc) {

    //Get the socket descriptor
    int *sock = NULL;
    sock = socket_desc;
    int read_size = 0;
    //char temp_char[4096];
    char *temp_char = malloc(4097);
    int slot = -1;
    int abort = 0;
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);

    
    // Find which slot we are!
    airnav_log_level(2, "Sock no data handler: %d\n", *sock);
    pthread_mutex_lock(&m_anrb_list);
    for (int j = 0; j < MAX_ANRB; j++) {
        if (anrbList[j].socket != NULL) {
            if (*anrbList[j].socket == *sock) {
                slot = j;
                break;
            }
        }
    }
    pthread_mutex_unlock(&m_anrb_list);

    if (slot > -1) {

        airnav_log_level(5, "Slot for this anrb connection is: %d\n", slot);

        // Send initial info

        char *tmp_vers = calloc(70, sizeof (char));
        sprintf(tmp_vers, "$VER,%s", BDTIME);
        char *ver = anrb_xorencrypt(tmp_vers, xorkey);
        gchar *base = g_base64_encode((const guchar*) ver, strlen(ver));
        airnav_log_level(2, "Base64: %s\n", base);
        int tamanho = strlen(base);

        send(*sock, base, tamanho, 0);
        //send(*sock,(char*)"\n",1,0);
        send(*sock, &txend[0], 1, 0);
        send(*sock, &txend[1], strlen((char*) "*"), 0);
        free(tmp_vers);
        free(ver);
        g_free(base);


        // Initial com
        while (abort != 1) {

            if (Modes.exit) {
                abort = 1;
            }

            // Let's clear the incoming buffer, if exists
            read_size = recv(*sock, temp_char, 4096, MSG_DONTWAIT);
            if (read_size != 0) { // Connection still active
                usleep(10000);
            } else {
                abort = 1;
                break;
            }


        }
    } else {
        airnav_log("[anrb_handle] Invalid slot for this anrb (%d)\n", slot);
    }

    airnav_log_level(3, "ANRB [%d] disconnected. Cleaning lists...\n", slot);
    pthread_mutex_lock(&m_anrb_list);
    close(*anrbList[slot].socket);
    *anrbList[slot].socket = -1;
    // Let's clear some info and free memory
    anrbList[slot].active = 0;
    anrbList[slot].port = 0;

    airnav_log("ANRB at slot %d disconnected.\n", slot);
    pthread_mutex_unlock(&m_anrb_list);
    free(temp_char);
    pthread_exit(EXIT_SUCCESS);

}


char *anrb_xorencrypt(char * message, char * key) {
    size_t messagelen = strlen(message);
    size_t keylen = strlen(key);

    char * encrypted = malloc(messagelen + 1);

    unsigned int i;
    for (i = 0; i < messagelen; i++) {
        encrypted[i] = message[i] ^ key[i % keylen];
    }
    encrypted[messagelen] = '\0';

    return encrypted;
}


/*
 * Thread to send data
 */
void *anrb_threadSendDataANRB(void *argv) {
    MODES_NOTUSED(argv);
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);
    
    struct packet_list *tmp1, *local_list;
    uint64_t now = mstime();


    while (!Modes.exit) {

        pthread_mutex_lock(&m_copy2);
        local_list = flist2;
        flist2 = NULL;
        pthread_mutex_unlock(&m_copy2);
        now = mstime();

        while (local_list != NULL) {
            if (local_list->packet != NULL) {
                if ((now - local_list->packet->timestp) > 60000) {
                    airnav_log("Address %06X invalid (more than 60 seconds timestamp). Now: $llu, packet timestamp: %llu\n", local_list->packet->modes_addr,
                            now, local_list->packet->timestp);
                }
            }

            pthread_mutex_lock(&m_anrb_list);
            for (int i = 0; i < MAX_ANRB; i++) {
                if (anrbList[i].active == 1) {
                    if (local_list->packet != NULL) {
                        anrb_sendANRBPacket(anrbList[i].socket, local_list->packet);
                    }
                }
            }
            pthread_mutex_unlock(&m_anrb_list);

            if (local_list->packet != NULL) {
                free(local_list->packet);
            }

            tmp1 = local_list;
            local_list = local_list->next;
            FREE(tmp1);
        }

        sleep(1);
    }

    airnav_log_level(1, "Exited SendDataANRB Successfull!\n");
    pthread_exit(EXIT_SUCCESS);
 
}


/*
 * Send a packet to server socket (PIT/PTA format)
 */
int anrb_sendANRBPacket(void *socket_, struct p_data *pac) {

#define SP_BUF_SUZE 4096
    int sock = *(int*) socket_;
    char *buf_local = malloc(SP_BUF_SUZE);
    memset(buf_local, 0, SP_BUF_SUZE);

    char callsign[12] = {0};
    char altitude[20] = {0};
    char gnd_spd[10] = {0};
    char heading[10] = {0};
    char vrate[10] = {0};
    char lat[30] = {0};
    char lon[30] = {0};
    char ias[10] = {0};
    char squawk[10] = {0};
    char prefix[10] = {0};
    char modes[30] = {0};
    int sendd = 0;
    char p_airborne[2] = {""};
    if (pac->callsign_set == 1) {
        memcpy(&callsign, &pac->callsign, 9);
        sendd = 1;
    }

    if (pac->altitude_set == 1) {
        sprintf(altitude, "%d", pac->altitude);
        sendd = 1;
    }
    if (pac->gnd_speed_set == 1) {
        sprintf(gnd_spd, "%d", pac->gnd_speed * 10);
        sendd = 1;
    }

    if (pac->heading_set == 1) {
        sprintf(heading, "%d", pac->heading * 10);
        sendd = 1;
    }

    if (pac->vert_rate_set == 1) {
        sprintf(vrate, "%d", pac->vert_rate * 10);
        sendd = 1;
    }

    if (pac->position_set == 1) {
        sprintf(lat, "%.13f", pac->lat);
        sprintf(lon, "%.13f", pac->lon);
        sendd = 1;
    }

    if (pac->ias_set == 1) {
        sprintf(ias, "%d", pac->ias * 10);
        sendd = 1;
    }

    if (pac->squawk_set == 1) {
        sprintf(squawk, "%04x", pac->squawk);
        sendd = 1;
    }

#ifdef RBCS
    sprintf(prefix, "RBCS");
#elif RBLC
    sprintf(prefix, "RBLC");
#else
    sprintf(prefix, "RPI");
#endif


    if (pac->modes_addr_set == 1) {

        sendd = 1;
        if (pac->modes_addr & MODES_NON_ICAO_ADDRESS) {
            airnav_log_level(3, "Invalid ICAO code.\n");
        } else {
            sprintf(modes, "%06X", (pac->modes_addr & 0xffffff));

            if (strlen(modes) > 6) {
                airnav_log_level(3, "HEX bigger than 6. Size: %d, value: '%s'\n", strlen(modes), modes);
                memset(&modes, 0, 30);
            }
        }


    }

    if (pac->airborne_set == 1) {
        if (pac->airborne == 1) {
            sprintf(p_airborne, "1");
        } else {
            sprintf(p_airborne, "0");
        }
    }

    // Timestamp packet
    time_t timer;
    //char buffer[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(pac->p_timestamp, 20, "%Y%m%d%H%M%S", tm_info);
    pac->p_timestamp[20] = '\0';

    sprintf(buf_local,
            "$PTA,%06X,%s," //
            "%-s," // Callsign
            "%s," // Altitude
            "%s," // ground Speed
            "%s," // Heading
            "%s,," // VRate
            "%s," // Lat
            "%s," // Lon
            "%s," // IAS
            "%s,," // Squawk
            "%s,," // Airborne
            ""
            ,
            pac->modes_addr, pac->p_timestamp,
            callsign,
            altitude,
            gnd_spd,
            heading,
            vrate,
            lat,
            lon,
            ias,
            squawk,
            p_airborne
            );



    if (sendd == 1) {
        // airnav_log("Sending.....\n");
        int tamanho = strlen(buf_local);
        char *abc = anrb_xorencrypt(buf_local, xorkey);
        gchar *base = g_base64_encode((const guchar*) abc, tamanho);

        send(sock, base, strlen(base), 0);

        send(sock, &txend[0], 1, 0);
        send(sock, &txend[1], strlen((char*) "*"), 0);
        //send(sock,(char*)"\n",1,0);

        airnav_log_level(2, "Sent: %s\n", base);
        free(abc);

        g_free(base);

    }
    free(buf_local);
    return 1;
}
