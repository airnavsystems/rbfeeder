/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#include "rbfeeder.h"

pid_t p_978;
char *dump978_cmd;
int autostart_978;
int dump978_enabled;
int dump978_port;
char *dump978_soapy_params;

pthread_t t_dump978;

void uat_loadUatConfig(void) {
    ini_getString(&dump978_cmd, configuration_file, "dump978", "dump978_cmd", NULL);
    if (dump978_cmd == NULL) {

        airnav_log_level(3, "No 978 cmd defined! Let's try default....\n");
        if (file_exist("/usr/bin/dump978-rb")) {
            ini_getString(&dump978_cmd, configuration_file, "dump978", "dump978_cmd", "/usr/bin/dump978-rb");
            dump978_enabled = ini_getBoolean(configuration_file, "dump978", "dump978_enabled", 1);
        } else {
            airnav_log_level(3, "No 978 binary found\n");
            dump978_enabled = ini_getBoolean(configuration_file, "dump978", "dump978_enabled", 0);
        }

    } else {
        dump978_enabled = ini_getBoolean(configuration_file, "dump978", "dump978_enabled", 1);
    }


    autostart_978 = ini_getBoolean(configuration_file, "dump978", "autostart_dump978", 0);    
}

/*
 * Check if dump978 is running
 */
int uat_check978Running(void) {
    if (p_978 <= 0) {
        return 0;
    }

    if (kill(p_978, 0) == 0) {
        return 1;
    } else {
        p_978 = 0;
        return 0;
    }
}

/*
 * Start dump978, if not running
 */
void uat_start978(void) {

    if (uat_check978Running() != 0) {
        airnav_log_level(1, "Looks like dump978 is already running.\n");
        return;
    }


    if (dump978_cmd == NULL) {
        airnav_log_level(1, "dump978 command line not defined.\n");
        return;
    }


    char *tmp_cmd = malloc(300);
    memset(tmp_cmd, 0, 300);

    sprintf(tmp_cmd, "%s %s --json-port %d", dump978_cmd, dump978_soapy_params, dump978_port);

    airnav_log_level(1, "Starting dump978 with this command: '%s'\n", tmp_cmd);

    p_978 = run_cmd3(tmp_cmd);
    free(tmp_cmd);

    sleep(3);

    if (uat_check978Running() != 0) {
        airnav_log_level(1, "Ok, dump978 started! Pid is: %i\n", p_978);
    } else {
        airnav_log_level(1, "Error starting dump978\n");
    }


    return;
}

/*
 * Stop dump978
 */
void uat_stop978(void) {

    if (uat_check978Running() == 0) {
        airnav_log_level(1, "dump978 is not running.\n");
        return;
    }
    if (kill(p_978, SIGTERM) == 0) {
        airnav_log_level(1, "Succesfully stopped dump978!\n");
        sleep(2);
        net_sendStats();
        return;
    } else {
        airnav_log_level(1, "Error stopping dump978.\n");
        return;
    }

    return;
}

/*
 * Stop and start UAT, if running
 */
void uat_restart978() {

    if (uat_check978Running() == 1) {

        uat_stop978();
        sleep(3);
        uat_start978();
    } else {
        uat_start978();
    }

}

/*
 *  Thread that connect to dump978, read
 * data and send to RB Servers
 */
void *uat_airnav_ext978(void *arg) {
    MODES_NOTUSED(arg);

    int sock_978;
    struct sockaddr_in addr_978;
    int * p_int;

START_EXT:
    sock_978 = socket(AF_INET, SOCK_STREAM, 0);


    addr_978.sin_family = AF_INET;
    addr_978.sin_port = htons(dump978_port);
    inet_pton(AF_INET, "127.0.0.1", &(addr_978.sin_addr));


    p_int = (int*) malloc(sizeof (int));
    *p_int = 1;
    if ((setsockopt(sock_978, SOL_SOCKET, SO_REUSEADDR, (char*) p_int, sizeof (int)) == -1) ||
            (setsockopt(sock_978, SOL_SOCKET, SO_KEEPALIVE, (char*) p_int, sizeof (int)) == -1)) {
        airnav_log("Error setting options %d\n", errno);
        free(p_int);
    }
    free(p_int);

    sleep(3);

    int res = -1;
    res = connect(sock_978, (struct sockaddr *) &addr_978, sizeof (addr_978));
    while (res != 0 && Modes.exit != 1) {
        airnav_log("Can't connect to 978 source (127.0.0.1:%d). Waiting 5 second...\n", dump978_port);
        sleep(5);
        res = connect(sock_978, (struct sockaddr *) &addr_978, sizeof (addr_978));
    }

    airnav_log_level(1, "dump978 connected!\n");

    int buffer_size = 4096;
    int r = 0;
    char buf[buffer_size];

    memset(&buf, 0, 4096);

    while (!Modes.exit) {

        r = recv(sock_978, &buf, buffer_size, MSG_DONTWAIT);
        if (r > 0) {

            r = 0;
            airnav_log_level(4, "Received data from dump978: %s\n", buf);
            uat_store978data(buf);
            memset(&buf, 0, 4096);
            usleep(40000);
        } else if (r == 0) {
            close(sock_978);
            airnav_log_level(4, "Disconnected from dump978\n");
            //close(sock_int);
            sleep(5);
            goto START_EXT;

        } else {
            usleep(40000);
        }


    }

    return NULL;

}

int uat_store978data(char packet[4096]) {

    int send = 0;
    json_t *root = load_json(packet);

    if (root) { // IF a valid json is received


        struct p_data *acf;
        acf = net_preparePacket_v2();
        acf->timestp = mstime();
        acf->cmd = 5;
        acf->is_978 = 1;

        // Check if ADDRESS is filled
        json_t *address = json_object_get(root, "address");
        if (address != NULL && json_typeof(address) == JSON_STRING) {

            const char *address_s = json_string_value(address);
            int num = (int) strtol(address_s, NULL, 16);

            acf->modes_addr = num;
            acf->modes_addr_set = 1;
            send = 1;

        }


        // Check if POSITION is filled
        json_t *position_item = json_object_get(root, "position");
        if (position_item != NULL && json_typeof(position_item) == JSON_OBJECT) {

            // Now try to get lat/lon fields
            json_t *lat_item = json_object_get(position_item, "lat");
            json_t *lon_item = json_object_get(position_item, "lon");
            if (lat_item != NULL && json_typeof(lat_item) == JSON_REAL && lon_item != NULL && json_typeof(lon_item) == JSON_REAL) {
                double lat = json_real_value(lat_item);
                double lon = json_real_value(lon_item);
                acf->lat = lat;
                acf->lon = lon;
                acf->position_set = 1;
                send = 1;
            }


        }


        // Check if ground_speed is filled
        json_t *ground_speed_item = json_object_get(root, "ground_speed");
        if (ground_speed_item != NULL && json_typeof(ground_speed_item) == JSON_INTEGER) {

            int ground_speed = json_integer_value(ground_speed_item);

            acf->gnd_speed = (ground_speed / 10);
            acf->gnd_speed_set = 1;
            send = 1;

        }


        // Check if ground_speed is filled
        json_t *true_track_item = json_object_get(root, "true_track");
        if (true_track_item != NULL && json_typeof(true_track_item) == JSON_REAL) {

            double true_track = json_real_value(true_track_item);
            acf->heading = (true_track / 10);
            acf->heading_set = 1;
            send = 1;

        }


        // Check if vertical_velocity_geometric is filled
        json_t *vertical_velocity_geometric_item = json_object_get(root, "vertical_velocity_geometric");
        if (vertical_velocity_geometric_item != NULL && json_typeof(vertical_velocity_geometric_item) == JSON_INTEGER) {

            int vertical_velocity_geometric = json_integer_value(vertical_velocity_geometric_item);
            acf->vert_rate = (vertical_velocity_geometric / 10);
            acf->vert_rate_set = 1;
            send = 1;
        }


        // Check if vertical_velocity_barometric is filled
        json_t *vertical_velocity_barometric_item = json_object_get(root, "vertical_velocity_barometric");
        if (vertical_velocity_barometric_item != NULL && json_typeof(vertical_velocity_barometric_item) == JSON_INTEGER) {

            int vertical_velocity_barometric = json_integer_value(vertical_velocity_barometric_item);
            acf->vert_rate = (vertical_velocity_barometric / 10);
            acf->vert_rate_set = 1;
            send = 1;
        }

        if (send == 1) {

            airnav_log_level(4, "Sending UAT packet...\n");
            pthread_mutex_lock(&m_copy);
            //pthread_mutex_lock(&Modes.data_mutex);

            struct packet_list *tmp;
            tmp = malloc(sizeof (struct packet_list));
            tmp->next = flist;
            tmp->packet = acf;
            flist = tmp;

            pthread_mutex_unlock(&m_copy);
            //pthread_mutex_unlock(&Modes.data_mutex);


        } else {
            free(acf);
        }

        json_decref(root);

    } else {
        airnav_log_level(1, "Invalod JSON packet.\n");
    }



    return -1;

}
