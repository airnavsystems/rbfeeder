/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "rbfeeder.h"
#include "airnav_cmd.h"
#include "airnav_mlat.h"

/*
 * Proccess server control packet
 */
void cmd_proccess_ctr_cmd_packet(uint8_t *packet, unsigned p_size) {

    ControlCommand *ctr_cmd = control_command__unpack(NULL, p_size, packet);


    if (ctr_cmd == NULL) {
        airnav_log("Invalid packet data for STR_CMD type.\n");
        free(packet);
        return;
    }

    ProtobufCEnumDescriptor cmd_types = command_type__descriptor;

    //cmd_types.values[3].name

    airnav_log_level(3, "Control type: %d (%s)\n", ctr_cmd->type, cmd_types.values[ctr_cmd->type].name);


    // Set client location
    if (ctr_cmd->type == COMMAND_TYPE__SET_LOCATION) {

        if (ctr_cmd->has_latitude) {
            ini_saveDouble(configuration_file, "client", "lat", ctr_cmd->latitude);
            g_lat = ini_getDouble(configuration_file, "client", "lat", 0);
        }

        if (ctr_cmd->has_longitude) {
            ini_saveDouble(configuration_file, "client", "lon", ctr_cmd->longitude);
            g_lon = ini_getDouble(configuration_file, "client", "lon", 0);
        }

        if (ctr_cmd->has_altitude) {
            ini_saveInteger(configuration_file, "client", "alt", ctr_cmd->altitude);
            g_alt = ini_getDouble(configuration_file, "client", "alt", -999);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_MLAT_COMMAND) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "mlat", "mlat_cmd", ctr_cmd->value);
            ini_getString(&mlat_cmd, configuration_file, "mlat", "mlat_cmd", NULL);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_MLAT_AUTO_START_ON) {

        ini_saveGeneric(configuration_file, "mlat", "autostart_mlat", "true");
        autostart_mlat = 1;

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_MLAT_AUTO_START_OFF) {

        ini_saveGeneric(configuration_file, "mlat", "autostart_mlat", "false");
        autostart_mlat = 0;

    } else if (ctr_cmd->type == COMMAND_TYPE__START_MLAT) {

        mlat_startMLAT();

    } else if (ctr_cmd->type == COMMAND_TYPE__STOP_MLAT) {

        mlat_stopMLAT();

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_AUTO_START_ON) {

        ini_saveGeneric(configuration_file, "vhf", "autostart_vhf", "true");
        autostart_vhf = 1;

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_AUTO_START_OFF) {

        ini_saveGeneric(configuration_file, "vhf", "autostart_vhf", "false");
        autostart_vhf = 0;

    } else if (ctr_cmd->type == COMMAND_TYPE__START_VHF) {

        startVhf();

    } else if (ctr_cmd->type == COMMAND_TYPE__STOP_VHF) {

        stopVhf();

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_COMMAND) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "vhf", "vhf_cmd", ctr_cmd->value);
            ini_getString(&vhf_cmd, configuration_file, "vhf", "vhf_cmd", NULL);
            generateVHFConfig();
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_FREQS) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "vhf", "freqs", ctr_cmd->value);
            ini_getString(&vhf_freqs, configuration_file, "vhf", "freqs", "118000000");
            generateVHFConfig();
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_SQUELCH) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "vhf", "squelch", ctr_cmd->value);
            vhf_squelch = ini_getInteger(configuration_file, "vhf", "squelch", -1);
            generateVHFConfig();
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_GAIN) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "vhf", "gain", ctr_cmd->value);
            vhf_gain = ini_getInteger(configuration_file, "vhf", "gain", 42);
            generateVHFConfig();
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_VHF_DEVICE) {

        if (ctr_cmd->value != NULL) {
            airnav_log_level(0, "Value in packet: '%s'\n", ctr_cmd->value);
        }
        if (ctr_cmd->has_device) {
            airnav_log_level(0, "Device for VHF: '%u'\n", ctr_cmd->device);
            ini_saveInteger(configuration_file, "vhf", "device", ctr_cmd->device);
            vhf_device = ini_getInteger(configuration_file, "vhf", "device", 1);
            generateVHFConfig();
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__RUN_RF_SURVEY) {

        if (ctr_cmd->value != NULL) {

            if (ctr_cmd->has_min_freq) {
                rfsurvey_min = ctr_cmd->min_freq;
            }
            if (ctr_cmd->has_max_freq) {
                rfsurvey_max = ctr_cmd->max_freq;
            }
            if (ctr_cmd->has_device) {
                rfsurvey_dongle = ctr_cmd->device;
            }
            memset(rfsurvey_key,0,501);
            strcpy(rfsurvey_key,ctr_cmd->value);
            rfsurvey_execute = 1;

        }

    } else if (ctr_cmd->type == COMMAND_TYPE__START_ACARS) {

        acars_startACARS();

    } else if (ctr_cmd->type == COMMAND_TYPE__STOP_ACARS) {

        acars_stopACARS();

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_COMMAND) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "acars", "acars_cmd", ctr_cmd->value);
            ini_getString(&acars_cmd, configuration_file, "acars", "acars_cmd", NULL);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_DEVICE) {

        if (ctr_cmd->has_device) {
            ini_saveInteger(configuration_file, "acars", "device", ctr_cmd->device);
            acars_device = ini_getInteger(configuration_file, "acars", "device", 1);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_FREQS) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "acars", "freqs", ctr_cmd->value);
            ini_getString(&acars_freqs, configuration_file, "acars", "freqs", "131.550");
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_SERVER) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "acars", "server", ctr_cmd->value);
            ini_getString(&acars_server, configuration_file, "acars", "server", "airnavsystems.com:9743");
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_MLAT_SERVER) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "mlat", "server", ctr_cmd->value);
            ini_getString(&mlat_server, configuration_file, "mlat", "server", DEFAULT_MLAT_SERVER);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_PPM_ERROR) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_ppm_error", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_DEVICE) {

        if (ctr_cmd->has_device) {
            ini_saveInteger(configuration_file, "client", "dump_device", ctr_cmd->device);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_GAIN) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_gain", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_AGC) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_agc", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_DC_FILTER) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_dc_filter", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_FIX) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_fix", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_CHECK_CRC) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_check_crc", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_DUMP_MODE_AC) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_mode_ac", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_AUTO_START_ON) {

        ini_saveGeneric(configuration_file, "acars", "autostart_acars", "true");
        autostart_acars = 1;

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ACARS_AUTO_START_OFF) {

        ini_saveGeneric(configuration_file, "acars", "autostart_acars", "false");
        autostart_acars = 0;


    } else if (ctr_cmd->type == COMMAND_TYPE__RESTART_DUMP) {

        dumprb_restartDump();

    } else if (ctr_cmd->type == COMMAND_TYPE__RESTART_MLAT) {

        mlat_restartMLAT();

    } else if (ctr_cmd->type == COMMAND_TYPE__RESTART_VHF) {

        restartVhf();

    } else if (ctr_cmd->type == COMMAND_TYPE__RESTART_ACARS) {

        acars_restartACARS();

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ADAPTIVE_BURST) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_adaptive_burst", ctr_cmd->value);
        }

    } else if (ctr_cmd->type == COMMAND_TYPE__SET_ADAPTIVE_RANGE) {

        if (ctr_cmd->value != NULL) {
            ini_saveGeneric(configuration_file, "client", "dump_adaptive_range", ctr_cmd->value);
        }

    }



    control_command__free_unpacked(ctr_cmd, NULL);
    free(packet);

}