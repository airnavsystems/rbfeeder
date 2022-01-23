/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_dumprb.h"

char *dumprb_cmd;
pid_t p_dumprb;
int dump_agc;
double dump_gain;

/*
 * Check if dump1090-rb is running
 */
int dumprb_checkDumprbRunning(void) {
    if (p_dumprb <= 0) {
        return 0;
    }

    if (kill(p_dumprb, 0) == 0) {
        return 1;
    } else {
        p_dumprb = 0;
        return 0;
    }
}

/*
 * Start DumpRB, if not running
 */
void dumprb_startDumprb(void) {

    if (dumprb_checkDumprbRunning() != 0) {
        airnav_log_level(3, "Looks like dump1090-rb is already running.\n");
        return;
    }


    if (dumprb_cmd == NULL) {
        airnav_log_level(3, "dump1090-rb command line not defined.\n");
        return;
    }


    char *dcmd = calloc(4096, sizeof (char));


    dcmd = airnav_concat(dcmd, "%s --write-json %s", dumprb_cmd, Modes.json_dir);

    // Requires parameters
    dcmd = airnav_concat(dcmd, " --net --quiet --mlat --forward-mlat --net-bo-port %d", (external_port + 100));


    // User location
    if (Modes.fUserLat != 0.0 && Modes.fUserLon != 0.0) {
        dcmd = airnav_concat(dcmd, " --lat %.6f --lon %.6f", Modes.fUserLat, Modes.fUserLon);
    }

    if (dump_gain != -10) {
        dcmd = airnav_concat(dcmd, " --gain %.2f", dump_gain);
    }

    if (dump_agc == 1) {
        dcmd = airnav_concat(dcmd, " --enable-agc");
    }

    if (device_n != -1) {
        dcmd = airnav_concat(dcmd, " --device %d", device_n);
    }

    int mode_ac = ini_getBoolean(configuration_file, "client", "dump_mode_ac", 1);
    if (mode_ac) {
        dcmd = airnav_concat(dcmd, " --modeac");
    }

    if (ini_getBoolean(configuration_file, "client", "dump_fix", 1)) {
        dcmd = airnav_concat(dcmd, " --fix");
    }
    
    //if (ini_getBoolean(configuration_file, "client", "dump_dc_filter", 1)) {
    //    dcmd = airnav_concat(dcmd, " --dcfilter");
    //}

    if (ini_getBoolean(configuration_file, "client", "dump_adaptive_burst", 1)) {
        dcmd = airnav_concat(dcmd, " --adaptive-burst");
    }

    if (ini_getBoolean(configuration_file, "client", "dump_adaptive_range", 1)) {
        dcmd = airnav_concat(dcmd, " --adaptive-range");
    }
    
    
    if (use_gnss == 1) {
        dcmd = airnav_concat(dcmd, " --gnss");
    }

    airnav_log_level(3, "Final dump1090-rb command: %s\n", dcmd);

    airnav_log_level(3, "Starting dump1090-rb with this command: '%s'\n", dcmd);

    p_dumprb = run_cmd3(dcmd);
    free(dcmd);

    //sleep(3);

    if (dumprb_checkDumprbRunning() != 0) {
        airnav_log_level(3, "Ok, dump1090-rb started! Pid is: %i\n", p_dumprb);
    } else {
        airnav_log_level(3, "Error starting dump1090-rb\n");
    }


    return;
}

/*
 * Stop dump1090-rb
 */
void dumprb_stopDumprb(void) {

    if (dumprb_checkDumprbRunning() == 0) {
        airnav_log_level(3, "dump1090-rb is not running.\n");
        return;
    }
    if (kill(p_dumprb, SIGTERM) == 0) {
        airnav_log_level(3, "Succesfully stopped dump1090-rb!\n");
        sleep(2);
        return;
    } else {
        airnav_log_level(3, "Error stopping dump1090-rb.\n");
        return;
    }

    return;
}

/*
 * Stop and start Dump1090, if running
 */
void dumprb_restartDump() {

    if (dumprb_checkDumprbRunning() == 1) {

        dumprb_stopDumprb();
        sleep(3);
        dumprb_startDumprb();
    } else {
        dumprb_startDumprb();
    }

}

/*
 * Send dump configuration to server
 */
void dumprb_sendDumpConfig(void) {
    
}
