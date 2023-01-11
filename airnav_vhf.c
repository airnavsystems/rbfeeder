/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */

#include "airnav_vhf.h"

char *vhf_pidfile;
char *vhf_config_file;
char *ice_host;
char *ice_mountpoint;
int ice_port;
char *ice_user;
char *ice_pwd;
int vhf_device;
double vhf_gain;
int vhf_squelch;
int vhf_correction;
int vhf_afc;
char *vhf_mode;
char *vhf_freqs;
int autostart_vhf;
pid_t p_vhf;
char *vhf_cmd;
char *vhf_stop_cmd;
char *vhf_dongle_serial;
char *liveatc_mount;
char *liveatc_user;
char *liveatc_pwd;
// Custom audio feed
char *ice_custom_url;
int ice_custom_port;
char *ice_custom_user;
char *ice_custom_pwd;
char *ice_custom_mount;

/*
 * Generate new VHF configuration file
 */
int generateVHFConfig() {

    loadVhfConfig();

    if (vhf_config_file == NULL) {
        airnav_log("No configuration file defined for VHF dongle.\n");
        return 0;
    }

    char *buf = (char *) malloc(4096);
    char *p = buf;
    int *len = malloc(sizeof (int));


    p += sprintf(p, "\n"
            "pidfile = \"/run/rtl_airband.pid\";\n"
            "devices = (\n"
            "  {\n"
            "      // Set device to scanning mode if you are scanning multiple frequencies\n"
            );

    if (vhf_dongle_serial != NULL) {
        p += sprintf(p, "      serial = \"%s\";\n", vhf_dongle_serial);
    } else if (ini_getInteger(configuration_file, "vhf", "device", -1) >= 0) {
        p += sprintf(p, "      index = %d;\n", vhf_device);
    } else {
        airnav_log("No device index or serial defined for VHF dongle.\n");
        free(buf);
        free(len);
        return 0;
    }


    p += sprintf(p, ""
            "      gain = %.2f;\n"
            "      mode = \"scan\";\n"
            "      channels = (\n"
            "        {\n"
            "          freqs = ( %s );\n",
            vhf_gain,
            vhf_freqs
            );

    if ((vhf_squelch > -1) && (vhf_squelch <= 255)) {
        p += sprintf(p, "           squelch = %d;\n", vhf_squelch);
    }

    p += sprintf(p, ""
            "   outputs = (\n"
            "          // Configuration for RadarBox\n"
            "          {\n"
            "            disable = false;\n"
            "            type = \"icecast\";\n"
            "            server = \"%s\";\n"
            "            port = %d;\n"
            "            mountpoint = \"%s\";\n"
            "            username = \"%s\";\n"
            "            password = \"%s\";\n"
            "            name = \"My Transmitter\";\n"
            "            genre = \"ATC\";\n"
            "            send_scan_freq_tags = true;\n"
            "          }\n",
            ice_host,
            ice_port,
            (ice_mountpoint == NULL ? sn : ice_mountpoint),
            ice_user,
            ice_pwd
            );



    // LiveATC
    if ((ini_getBoolean(configuration_file, "vhf", "liveatc_enabled", 0) == 1) && (liveatc_mount != NULL) && (liveatc_user != NULL) && (liveatc_pwd != NULL)) {

        p += sprintf(p, ",\n"
                "          // Configuration for LiveATC\n"
                "          {\n"
                "            disable = false;\n"
                "            type = \"icecast\";\n"
                "            server = \"audio-in.liveatc.net\";\n"
                "            port = 8010;\n"
                "            mountpoint = \"%s\";\n"
                "            username = \"%s\";\n"
                "            password = \"%s\";\n"
                "            name = \"My Transmitter\";\n"
                "            genre = \"ATC\";\n"
                "            send_scan_freq_tags = true;\n"
                "          }\n",
                liveatc_mount,
                liveatc_user,
                liveatc_pwd
                );

    }

    // Custom IceCast
    if ((ini_getBoolean(configuration_file, "vhf", "icecast_custom_enabled", 0) == 1) && (ice_custom_url != NULL) && (ice_custom_mount != NULL) && (ice_custom_user != NULL) && (ice_custom_pwd != NULL)) {

        p += sprintf(p, ",\n"
                "          // Custom configuration\n"
                "          {\n"
                "            disable = false;\n"
                "            type = \"icecast\";\n"
                "            server = \"%s\";\n"
                "            port = %d;\n"
                "            mountpoint = \"%s\";\n"
                "            username = \"%s\";\n"
                "            password = \"%s\";\n"
                "            name = \"My Transmitter\";\n"
                "            genre = \"ATC\";\n"
                "            send_scan_freq_tags = true;\n"
                "          }\n",
                ice_custom_url,
                ice_custom_port,
                ice_custom_mount,
                ice_custom_user,
                ice_custom_pwd
                );

    }




    // Finish configuration         
    p += sprintf(p, "\n"
            "        );\n"
            "      }\n"
            "    );\n"
            "  }\n"
            ");\n"
            );

    *len = (p - buf);

    // Save
    FILE *f = fopen(vhf_config_file, "w");
    if (f == NULL) {
        airnav_log("Cannot write VHF Config file: %s\n", strerror(errno));
        free(buf);
        free(len);
        return 0;
    } else {

        //fprintf(f, buf);
        fputs(buf, f);
        fclose(f);
        free(buf);
        free(len);
        return 1;
    }



}

/*
 * Check if vhf is running
 */
int checkVhfRunning(void) {

    if (vhf_pidfile == NULL) {
        //airnav_log_level(3,"Airband PID file not defined.\n");
        return 0;
    }

    FILE *f = fopen(vhf_pidfile, "r");
    if (f == NULL) {
        airnav_log_level(4, "Vhf pidfile (%s) does not exist.\n", vhf_pidfile);
        return 0;
    } else {
        char tmp[20];
        memset(&tmp, 0, 20);
        if (fgets(tmp, 20, (FILE*) f) == NULL) {
            airnav_log_level(4, "Error getting VHF status\n");
        }
        fclose(f);
        airnav_log_level(4, "Pid no arquivo: %s", tmp);
        if (strlen(tmp) > 2) {
            char* endptr;
            endptr = "0123456789";
            p_vhf = strtoimax(tmp, &endptr, 10);
            if (kill(p_vhf, 0) == 0) {
                airnav_log_level(4, "After testing PID, VHF is running!\n");
                return 1;
            } else {
                p_vhf = 0;
                airnav_log_level(4, "After testing PID, VHF is NOT running!\n");
                remove(vhf_pidfile);
                return 0;
            }

        } else {
            airnav_log_level(4, "VHF not running (PID check error 2)\n");
            return 0;
        }

    }
}

/*
 * Start vhf, if not running
 */
void startVhf(void) {

    if (vhf_pidfile == NULL) {
        airnav_log("VHF PID file not defined.\n");
        return;
    }

    checkVhfRunning();

    if (checkVhfRunning() != 0) {
        airnav_log_level(2, "Looks like vhf is already running.\n");
        return;
    }


    if (vhf_cmd == NULL) {
        airnav_log_level(2, "Vhf command line not defined.\n");
        return;
    }

    airnav_log_level(2, "Starting vhf with this command: '%s'\n", vhf_cmd);

    run_cmd2((char*) vhf_cmd);
    sleep(3);
    //  checkVhfRunning();

    if (checkVhfRunning() != 0) {
        airnav_log_level(2, "Ok, started! Pid is: %i\n", p_vhf);
        net_sendStats();
    } else {

        airnav_log_level(2, "Error starting vhf\n");
        p_vhf = 0;
        net_sendStats();
    }

    return;
}

/*
 * Stop vhf
 */
void stopVhf(void) {

    if (checkVhfRunning() == 0) {
        airnav_log_level(3, "Vhf is not running.\n");
        return;
    }



    if (vhf_stop_cmd == NULL) {

        if (kill(p_vhf, SIGTERM) == 0) {
            checkVhfRunning();
            airnav_log_level(3, "Succesfull kill vhf!\n");
            sleep(2);
            net_sendStats();
            return;
        } else {
            airnav_log_level(3, "Error killing vhf.\n");
            return;
        }

    } else {
        run_cmd2((char*) vhf_stop_cmd);
    }



    return;
}

/*
 * Stop and start VHF, if running
 */
void restartVhf() {

    if (checkVhfRunning() == 1) {

        stopVhf();
        sleep(3);
        startVhf();
    } else {
        startVhf();
    }

}

int loadVhfConfig() {


    // Load VHF configs
    ini_getString(&vhf_config_file, configuration_file, "vhf", "config_file", NULL);
    ini_getString(&ice_host, configuration_file, "vhf", "icecast_host", "audio.rb24.com");
    ini_getString(&ice_user, configuration_file, "vhf", "icecast_user", "source");
    ini_getString(&ice_pwd, configuration_file, "vhf", "icecast_pwd", "hackme");
    ice_port = ini_getInteger(configuration_file, "vhf", "icecast_port", 8000);
    ini_getString(&vhf_mode, configuration_file, "vhf", "mode", "scan");
    ini_getString(&vhf_freqs, configuration_file, "vhf", "freqs", "118000000");

    if (sn == NULL) {
        ini_getString(&ice_mountpoint, configuration_file, "vhf", "mountpoint", "010101");
    } else {
        ini_getString(&ice_mountpoint, configuration_file, "vhf", "mountpoint", sn);
    }

    vhf_device = ini_getInteger(configuration_file, "vhf", "device", 1);
    // New itens
    ini_getString(&vhf_dongle_serial, configuration_file, "vhf", "serial", NULL);
    ini_getString(&liveatc_mount, configuration_file, "vhf", "liveatc_mount", NULL);
    ini_getString(&liveatc_user, configuration_file, "vhf", "liveatc_user", NULL);
    ini_getString(&liveatc_pwd, configuration_file, "vhf", "liveatc_pwd", NULL);


    vhf_gain = ini_getDouble(configuration_file, "vhf", "gain", 42);
    vhf_squelch = ini_getInteger(configuration_file, "vhf", "squelch", -1);
    vhf_correction = ini_getInteger(configuration_file, "vhf", "correction", 0);
    vhf_afc = ini_getInteger(configuration_file, "vhf", "afc", 0);
    autostart_vhf = ini_getBoolean(configuration_file, "vhf", "autostart_vhf", 0);

    ini_getString(&vhf_pidfile, configuration_file, "vhf", "pid", NULL);
    if (vhf_pidfile == NULL) {
        ini_getString(&vhf_pidfile, configuration_file, "vhf", "pid", "/run/rtl_airband.pid");
    }

    ini_getString(&vhf_cmd, configuration_file, "vhf", "vhf_cmd", NULL);
    if (vhf_cmd == NULL) { // If is not set by user

        // Check if rtl-aorband-rb is installed
        if (file_exist("/usr/bin/rtl_airband") && file_exist("/lib/systemd/system/rtl-airband-rb.service")) {
            ini_getString(&vhf_cmd, configuration_file, "vhf", "vhf_cmd", "systemctl start rtl-airband-rb.service");
        }

    }

    // Command to stop VHF, if need
    ini_getString(&vhf_stop_cmd, configuration_file, "vhf", "stop_cmd", NULL);

    // Custom feed
    ini_getString(&ice_custom_url, configuration_file, "vhf", "icecast_custom_url", NULL);
    ini_getString(&ice_custom_user, configuration_file, "vhf", "icecast_custom_user", "source");
    ini_getString(&ice_custom_pwd, configuration_file, "vhf", "icecast_custom_pwd", "hackme");
    ini_getString(&ice_custom_mount, configuration_file, "vhf", "icecast_custom_mount", "xrange2");
    ice_custom_port = ini_getInteger(configuration_file, "vhf", "icecast_custom_port", 8000);


    return 1;

}

/*
 * Save VHF configuration to file
 */
int saveVhfConfig(void) {

    if ((strcmp(F_ARCH, "rbcs") != 0) && (strcmp(F_ARCH, "rblc") != 0)) {
        return 1;
    }

    char sq[30];
    memset(&sq, 0, 30);

    char af[30];
    memset(&af, 0, 30);

    char *template = "\ndevices:\n"
            "({\n"
            "  index = %d;\n"
            "  gain = %.0f;\n"
            "  correction = %d;\n"
            "  mode = \"%s\";\n"
            "  channels:\n"
            "  (\n"
            "    {\n"
            "      freqs = ( %s );\n%s%s"
            "      outputs: (\n"
            "        {\n"
            "	  type = \"icecast\";\n"
            "	  server = \"%s\";\n"
            "          port = %d;\n"
            "          mountpoint = \"%s\";\n"
            "          genre = \"ATC\";\n"
            "          username = \"%s\";\n"
            "          name = \"RadaBox24.com Live ATC\";\n"
            "          password = \"%s\";\n"
            "	  send_scan_freq_tags = false;\n"
            "        }\n"
            "      );\n"
            "    }\n"
            "  );\n"
            " }\n"
            ");\n";

    if (vhf_squelch > -1) {
        sprintf(sq, "      squelch = %d;\n", vhf_squelch);
    } else {
        sprintf(sq, " ");
    }

    if (vhf_afc > 0) {
        sprintf(af, "      afc = %d;\n", vhf_afc);
    } else {
        sprintf(af, " ");
    }

    FILE *f = fopen(vhf_config_file, "w");
    if (f == NULL) {
        airnav_log("Cannot write VHF Config file: %s\n", strerror(errno));
        return 0;
    } else {

        char mpoint[50] = {0};
        if (ice_mountpoint == NULL) {
            strcpy(mpoint, sn);
        } else {
            strcpy(mpoint, ice_mountpoint);
        }

        airnav_log_level(3, "Savinf VHF Config:\n");
        airnav_log_level(3, "Device: %.0f\n", vhf_device);
        airnav_log_level(3, "Gain: %.1f\n", vhf_gain);
        airnav_log_level(3, "Correction: %d\n", vhf_correction);
        airnav_log_level(3, "VHF Mode: %s\n", vhf_mode);
        airnav_log_level(3, "Freqs: %s\n", vhf_freqs);
        airnav_log_level(3, "SQ: %s\n", sq);
        airnav_log_level(3, "AF: %s\n", af);
        airnav_log_level(3, "Host: %s\n", ice_host);
        airnav_log_level(3, "Port: %d\n", ice_port);
        airnav_log_level(3, "Mount: %s\n", sn);
        airnav_log_level(3, "User: %s\n", ice_user);
        airnav_log_level(3, "Pwd: %s\n", ice_pwd);

        fprintf(f, template, vhf_device, vhf_gain, vhf_correction, vhf_mode, vhf_freqs, sq, af, ice_host, ice_port, sn, ice_user, ice_pwd);
        fclose(f);

        return 1;
    }


    return 1;
}

/*
 * Send Vhf configuration to server
 */
void sendVhfConfig(void) {

}
