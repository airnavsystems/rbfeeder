/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */

#include "airnav_acars.h"


pid_t p_acars;
char *acars_pidfile;
char *acars_cmd;
char *acars_server;
int acars_device;
char *acars_freqs;
int autostart_acars;

/*
 * Check if ACARS is running
 */
int acars_checkACARSRunning(void) {

    if (acars_pidfile == NULL) {
        return 0;
    }

    FILE *f = fopen(acars_pidfile, "r");
    if (f == NULL) {
        airnav_log_level(5, "ACARS pidfile (%s) does not exist.\n", acars_pidfile);
        return 0;
    } else {
        char tmp[20];
        memset(&tmp, 0, 20);
        if (fgets(tmp, 20, (FILE*) f) == NULL) {
            airnav_log_level(2, "Error checking ACARS\n");
        }
        fclose(f);
        airnav_log_level(5, "Pid no arquivo: %s\n", tmp);
        if (strlen(tmp) > 2) {
            char* endptr;
            endptr = "0123456789";
            p_acars = strtoimax(tmp, &endptr, 10);
            if (kill(p_acars, 0) == 0) {
                return 1;
            } else {
                p_acars = 0;
                remove(acars_pidfile);
                return 0;
            }

        } else {

            return 0;
        }

    }
}

/*
 * Start MLAT, if not running
 */
void acars_startACARS(void) {

    if (acars_pidfile == NULL) {
        airnav_log("ACARS PID file not defined.\n");
        return;
    }

    if (acars_checkACARSRunning() != 0) {
        airnav_log_level(3, "Looks like ACARS is already running.\n");
        return;
    }


    if (acars_cmd == NULL) {
        airnav_log_level(3, "ACARS command line not defined.\n");
        return;
    }


    char *tmp_cmd = malloc(300);
    memset(tmp_cmd, 0, 300);

#ifdef RBCS
    sprintf(tmp_cmd, "/sbin/start-stop-daemon --start --make-pidfile --background --pidfile %s --exec %s -- -n %s -r %d %s", acars_pidfile, acars_cmd, acars_server, acars_device, acars_freqs);
#else
    sprintf(tmp_cmd, "%s >/dev/null 2>&1 &", acars_cmd);
#endif

    airnav_log_level(3, "Starting ACARS with this command: '%s'\n", tmp_cmd);

    //system(tmp_cmd);
    run_cmd2(tmp_cmd);
    sleep(3);

    if (acars_checkACARSRunning() != 0) {
        airnav_log_level(3, "Ok, started! Pid is: %i\n", p_acars);
        net_sendStats();
    } else {

        airnav_log_level(3, "Error starting ACARS\n");
        p_acars = 0;
        net_sendStats();
    }

    return;
}

/*
 * Stop ACARS
 */
void acars_stopACARS(void) {

    if (acars_checkACARSRunning() == 0) {
        airnav_log_level(3, "ACARS is not running.\n");
        return;
    }
    if (kill(p_acars, SIGTERM) == 0) {
        acars_checkACARSRunning();
        airnav_log_level(3, "Succesfull kill ACARS!\n");
        sleep(2);
        net_sendStats();
        return;
    } else {
        airnav_log_level(3, "Error killing ACARS.\n");
        return;
    }

    return;
}

/*
 * Stop and start ACARS, if running
 */
void acars_restartACARS() {

    if (acars_checkACARSRunning() == 1) {
        acars_stopACARS();
        sleep(3);
        acars_startACARS();
    } else {
        acars_startACARS();
    }

}

/*
 * Send ACARS configuration to server
 */
void acars_sendACARSConfig(void) {
    
}
