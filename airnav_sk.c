/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_sk.h"
#include "airnav_utils.h"
#include "rbfeeder.pb-c.h"
#include "airnav_net.h"

/*
 * Send key and version to server.
 */
int sendKey(void) {
    if (airnav_socket == -1) {
        airnav_log_level(5, "Socket not created!\n");
        return 0;
    }

    airnav_log_level(3, "Sending key....\n");

    ClientType client_type = getClientType();
    char *cserial = NULL;
    if (client_type == CLIENT_TYPE__RPI || client_type == CLIENT_TYPE__RBLC2 || client_type == CLIENT_TYPE__GENERIC_ARM_32 || client_type == CLIENT_TYPE__GENERIC_ARM_64) {
        long long unsigned int cpuserial = getRpiSerial();
        if (cpuserial != 0) {
            cserial = malloc(60);
            memset(cserial, 0, 60);
            sprintf(cserial, "%016llx", cpuserial);
        } else {
            airnav_log("CPU Serial empty.\n");
            return 0;
        }
    } else if (client_type == CLIENT_TYPE__PC_X64 || client_type == CLIENT_TYPE__PC_X86) {
        cserial = net_get_mac_address(0);
    } else {
        airnav_log("Could not get any type of serial for SK validation\n");
        return 0;
    }

    if (cserial == NULL) {
        airnav_log("Could not get any type of serial for SK validation\n");
        return 0;
    } else if (strlen(cserial) < 10) {
        airnav_log("Invalid CPU Serial size\n");
        return 0;
    }



    struct prepared_packet *auth = create_packet_AuthFeederRequest(sharing_key, client_type, cserial);
    net_send_packet(auth);
    if (cserial != NULL) {
        free(cserial);
        cserial = NULL;
    }

    airnav_log_level(3, "Step 1 of sending key\n");

    // Wait for reply from server
    //
    int reply = net_waitCmd(SERVER_REPLY__REPLY_STATUS__AUTH_OK, 2);
    airnav_log_level(3, "WaitCMD done!\n");
    if (reply == SERVER_REPLY__REPLY_STATUS__AUTH_OK) {
        airnav_log_level(7, "Got OK from server!\n");
        return 1;
    } else if (reply == 0) {
        airnav_log_level(7, "Got no response from server\n");
        return 0;
    } else if (reply == 3) {
        airnav_log("Could not authenticate sharing key: \n");
        return -1;
    } else {
        airnav_log_level(7, "Got another cmd from server :( (%d)\n", reply);
        return -1;
    }

    airnav_log_level(3, "Something wrong....\n");
    return 1;

}

/*
 * Send key request
 */
int sendKeyRequest(void) {
    if (airnav_socket == -1) {
        airnav_log_level(5, "Socket not created!\n");
        return 0;
    }

    airnav_log_level(3, "Requesting new key!\n");
    ClientType client_type = getClientType();

    // Get CPU Serial
    char *cserial = NULL;
    if (client_type == CLIENT_TYPE__RPI || client_type == CLIENT_TYPE__RBLC2 || client_type == CLIENT_TYPE__GENERIC_ARM_32 || client_type == CLIENT_TYPE__GENERIC_ARM_64) {

        long long unsigned int cpuserial = getRpiSerial();
        if (cpuserial != 0) {
            cserial = malloc(60);
            memset(cserial, 0, 60);
            sprintf(cserial, "%016llx", cpuserial);
        } else {
            airnav_log("CPU Serial empty.\n");
        }

    } else if (client_type == CLIENT_TYPE__PC_X64 || client_type == CLIENT_TYPE__PC_X86) {
        cserial = net_get_mac_address(0);
    } else {
        airnav_log("Could not get any type of serial for SK request\n");
    }

    struct prepared_packet *request = create_packet_SK_Request(getClientType(), cserial);
    net_send_packet(request);
    if (cserial != NULL) {
        free(cserial);
        cserial = NULL;
    }

    // Wait for reply from server
    int reply = net_waitCmd(SERVER_REPLY__REPLY_STATUS__SK_CREATE_OK, 3);

    if (reply == SERVER_REPLY__REPLY_STATUS__SK_CREATE_OK) {
        airnav_log_level(3, "SK Created!\n");
        return 1;
    } else if (reply == 0) {
        airnav_log_level(3, "No response from server\n");
        return 0;
    } else if (reply == 3) {
        airnav_log_level(3, "Error creating SK:\n");
        return -1;
    } else {
        airnav_log_level(3, "Another reply from server\n");
        return -1;
    }

}
