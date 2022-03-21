/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "rbfeeder.h"
#include "airnav_mlat.h"

pid_t p_mlat;
char *mlat_cmd;
char *mlat_server;
char *mlat_pidfile;
int autostart_mlat;
char *mlat_config;
char *mlat_input_type;

void mlat_loadMlatConfig(void) {
    ini_getString(&mlat_cmd, configuration_file, "mlat", "mlat_cmd", NULL);
    if (mlat_cmd == NULL) {

        if (file_exist("/usr/bin/mlat-client")) {
            ini_getString(&mlat_cmd, configuration_file, "mlat", "mlat_cmd", "/usr/bin/mlat-client");
        }

    }

    ini_getString(&mlat_input_type, configuration_file, "mlat", "input_type", "dump1090");
    
    ini_getString(&mlat_server, configuration_file, "mlat", "server", DEFAULT_MLAT_SERVER);
    ini_getString(&mlat_pidfile, configuration_file, "mlat", "pid", NULL);
    if (mlat_pidfile == NULL) {

        if (file_exist("/usr/bin/mlat-client") && file_exist("/etc/default/mlat-client-config-rb")) {
            ini_getString(&mlat_pidfile, configuration_file, "mlat", "pid", "/run/mlat-client-config-rb.pid");
        }

    }

    autostart_mlat = ini_getBoolean(configuration_file, "mlat", "autostart_mlat", 1);
    ini_getString(&mlat_config, configuration_file, "mlat", "config", NULL);
    airnav_log_level(3, "MLAT Configuration file: %s\n", mlat_config);
    if (mlat_config == NULL) {

        if (file_exist("/etc/default/mlat-client-config-rb")) {
            ini_getString(&mlat_config, configuration_file, "mlat", "config", "/etc/default/mlat-client-config-rb");
            airnav_log_level(3, "MLAT Configuration file(2): %s\n", mlat_config);
        }
    }
    
}

/*
 * Check if MLAT is running
 */
int mlat_checkMLATRunning(void) {
    if (p_mlat <= 0) {
        return 0;
    }

    if (kill(p_mlat, 0) == 0) {
        return 1;
    } else {
        p_mlat = 0;
        return 0;
    }
}

/*
 * Start MLAT, if not running
 */
void mlat_startMLAT(void) {

    if (mlat_checkMLATRunning() != 0) {
        airnav_log_level(3, "Looks like MLAT is already running.\n");
        return;
    }


    if (mlat_cmd == NULL) {
        airnav_log_level(3, "MLAT command line not defined.\n");
        return;
    }

    if (g_lat != 0 && g_lon != 0 && g_alt != -999 && sn != NULL) {

        char *tmp_cmd = malloc(300);
        memset(tmp_cmd, 0, 300);

        sprintf(tmp_cmd, "%s --input-type %s --input-connect 127.0.0.1:%s --server %s --lat %f --lon %f --alt %d --user %s --results beast,connect,127.0.0.1:%s", mlat_cmd, mlat_input_type, beast_out_port, mlat_server, g_lat, g_lon, g_alt, sn, beast_in_port);

        airnav_log_level(3, "Starting MLAT with this command: '%s'\n", tmp_cmd);

        p_mlat = run_cmd3(tmp_cmd);
        free(tmp_cmd);

        sleep(3);

        if (mlat_checkMLATRunning() != 0) {
            airnav_log_level(3, "Ok, MLAT started! Pid is: %i\n", p_mlat);
            net_sendStats();
        } else {
            airnav_log_level(3, "Error starting MLAT\n");
            net_sendStats();
        }
    } else {
        airnav_log_level(2, "Can't start MLAT. Missing parameters.\n");
    }

    return;
}

/*
 * Stop MLAT
 */
void mlat_stopMLAT(void) {

    if (mlat_checkMLATRunning() == 0) {
        airnav_log_level(3, "MLAT is not running.\n");
        return;
    }
    if (kill(p_mlat, SIGTERM) == 0) {
        airnav_log_level(3, "Succesfully stopped MLAT!\n");
        sleep(2);
        net_sendStats();
        return;
    } else {
        airnav_log_level(3, "Error stopping MLAT.\n");
        return;
    }

    return;
}

/*
 * Stop and start VHF, if running
 */
void mlat_restartMLAT() {

    if (mlat_checkMLATRunning() == 1) {
        mlat_stopMLAT();
        sleep(3);
        mlat_startMLAT();
    } else {
        mlat_startMLAT();
    }

}

/*
 * Send MLAT configuration to server
 */
void mlat_sendMLATConfig(void) {
    
}

/*
 * Save MLAT configuration to file
 */
int mlat_saveMLATConfig(void) {

    char *template = "\n# MLAT - RadarBoxPi\n"
            "\n"
            "START_CLIENT=\"yes\"\n"
            "RUN_AS_USER=\"mlat\"\n"
            "SERVER_USER=\"%s\"\n"
            "LOGFILE=\"/var/log/mlat-client-config-rb.log\"\n"
            "INPUT_TYPE=\"dump1090\"\n"
            "INPUT_HOSTPORT=\"127.0.0.1:%d\"\n"
            "SERVER_HOSTPORT=\"%s\"\n"
            "LAT=\"%f\"\n"
            "LON=\"%f\"\n"
            "ALT=\"%d\"\n"
            "RESULTS=\"beast,connect,127.0.0.1:%s\"\n"
            "EXTRA_ARGS=\"\"\n"
            "\n";



    if (g_lat != 0 && g_lon != 0 && g_alt != -999 && sn != NULL && mlat_config != NULL) {

        FILE *f = fopen(mlat_config, "w");
        if (f == NULL) {
            airnav_log("Cannot write MLAT Config file (%s): %s\n", mlat_config, strerror(errno));
            return 0;
        } else {


            fprintf(f, template, sn, beast_out_port, mlat_server, g_lat, g_lon, g_alt, beast_in_port);
            fclose(f);

            return 1;
        }


    }


    return 0;
}

// check if this aircraft is mlat or not
int mlat_check_is_mlat(struct aircraft *a) {

    if (a->callsign_valid.source == SOURCE_MLAT)
        return 1;
    if (a->altitude_baro_valid.source == SOURCE_MLAT)
        return 1;
    if (a->altitude_geom_valid.source == SOURCE_MLAT)
        return 1;
    if (a->gs_valid.source == SOURCE_MLAT)
        return 1;
    if (a->ias_valid.source == SOURCE_MLAT)
        return 1;
    if (a->tas_valid.source == SOURCE_MLAT)
        return 1;
    if (a->mach_valid.source == SOURCE_MLAT)
        return 1;
    if (a->track_valid.source == SOURCE_MLAT)
        return 1;
    if (a->track_rate_valid.source == SOURCE_MLAT)
        return 1;
    if (a->roll_valid.source == SOURCE_MLAT)
        return 1;
    if (a->mag_heading_valid.source == SOURCE_MLAT)
        return 1;
    if (a->true_heading_valid.source == SOURCE_MLAT)
        return 1;
    if (a->baro_rate_valid.source == SOURCE_MLAT)
        return 1;
    if (a->geom_rate_valid.source == SOURCE_MLAT)
        return 1;
    if (a->squawk_valid.source == SOURCE_MLAT)
        return 1;
    if (a->emergency_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nav_qnh_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nav_altitude_mcp_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nav_altitude_fms_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nav_heading_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nav_modes_valid.source == SOURCE_MLAT)
        return 1;
    if (a->position_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nic_baro_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nac_p_valid.source == SOURCE_MLAT)
        return 1;
    if (a->nac_v_valid.source == SOURCE_MLAT)
        return 1;
    if (a->sil_valid.source == SOURCE_MLAT)
        return 1;
    if (a->gva_valid.source == SOURCE_MLAT)
        return 1;
    if (a->sda_valid.source == SOURCE_MLAT)
        return 1;

    return 0;
}
