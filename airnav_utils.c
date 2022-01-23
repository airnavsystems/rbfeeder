/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_utils.h"

void airnav_log_file_m(const char* fname, const char* filename, const char* format, ...) {
    AN_NOTUSED(format);

    char timebuf2[128];
    char msg2[1024];
    time_t now;
    struct tm local;
    va_list ap2;
    now = time(NULL);
    localtime_r(&now, &local);
    strftime(timebuf2, 128, "[%F %T]", &local);
    timebuf2[127] = 0;
    va_start(ap2, format);
    vsnprintf(msg2, 1024, format, ap2);
    va_end(ap2);
    msg2[1023] = 0;

    if (filename != NULL) {
        FILE * fp;
        fp = fopen(filename, "a");
        if (fp == NULL) {
            printf("Can't create log file %s\n", filename);
            return;
        }
        fprintf(fp, "%s [%s] %s\n", timebuf2, fname, msg2);
        fclose(fp);
    }

}

void airnav_log_file_pure_m(const char* filename, const char* format, ...) {

    char msg2[1024];
    va_list ap2;
    va_start(ap2, format);
    vsnprintf(msg2, 1024, format, ap2);
    va_end(ap2);
    msg2[1023] = 0;

    if (filename != NULL) {
        FILE * fp;
        fp = fopen(filename, "a");
        if (fp == NULL) {
            printf("Can't create log file %s\n", filename);
            return;
        }
        fprintf(fp, "%s", msg2);
        fclose(fp);
    }



}

void airnav_log(const char* format, ...) {

    if (disable_log == 1) {
        return;
    }

    char timebuf[128];
    char timebuf2[128];
    char msg[1024];
    char msg2[1024];
    time_t now;
    struct tm local;
    va_list ap, ap2;
    now = time(NULL);
    localtime_r(&now, &local);
    strftime(timebuf, 128, "\x1B[35m[\x1B[33m%F %T\x1B[35m]\x1B[0m", &local);
    strftime(timebuf2, 128, "[%F %T]", &local);
    timebuf[127] = 0;
    timebuf2[127] = 0;
    va_start(ap, format);
    va_start(ap2, format);
    vsnprintf(msg, 1024, format, ap);
    vsnprintf(msg2, 1024, format, ap2);
    va_end(ap);
    va_end(ap2);
    msg[1023] = 0;
    msg2[1023] = 0;
    if (daemon_mode == 0) {
        fprintf(stderr, "%s  %s", timebuf, msg);
    } else {
        syslog(LOG_NOTICE, "%s  %s", timebuf2, msg);
    }

    if (log_file != NULL) {
        FILE * fp;
        fp = fopen(log_file, "a");
        if (fp == NULL) {
            printf("Can't create log file %s\n", log_file);
            return;
        }
        fprintf(fp, "%s  %s", timebuf2, msg2);
        fclose(fp);
    }


}

void airnav_log_level_m(const char* fname, const int level, const char* format, ...) {
    MODES_NOTUSED(level);
    MODES_NOTUSED(format);

    if (disable_log == 1) {
        return;
    }

    if (debug_level >= level) {
        char timebuf[128];
        char timebuf2[128];
        char msg[1024];
        char msg2[1024];
        time_t now;
        struct tm local;
        va_list ap, ap2;
        now = time(NULL);
        localtime_r(&now, &local);
        strftime(timebuf, 128, "\x1B[35m[\x1B[33m%F %T\x1B[35m]\x1B[0m", &local);
        strftime(timebuf2, 128, "[%F %T]", &local);
        timebuf[127] = 0;
        timebuf2[127] = 0;
        va_start(ap, format);
        va_start(ap2, format);
        vsnprintf(msg, 1024, format, ap);
        vsnprintf(msg2, 1024, format, ap2);
        va_end(ap);
        va_end(ap2);
        msg[1023] = 0;
        msg2[1023] = 0;
        if (daemon_mode == 0) {
            fprintf(stdout, "%s \x1B[35m[\x1B[31m%s\x1B[35m]\x1B[0m  %s", timebuf, fname, msg);
        } else {
            syslog(LOG_NOTICE, "%s [%s] %s", timebuf2, fname, msg);
        }

        if (log_file != NULL) {
            FILE * fp;
            fp = fopen(log_file, "a");
            if (fp == NULL) {
                printf("Can't create log file %s\n", log_file);
                return;
            }
            fprintf(fp, "%s [%s] %s", timebuf2, fname, msg2);
            fclose(fp);
        }

    }


}

/*
 * New function to read string from
 * ini file using GLIB
 */
void ini_getString(char **item, char *ini_file, char *section, char *key, char *def_value) {
    GKeyFile *localini;
    GError *error = NULL;
    localini = g_key_file_new();

    if (*item != NULL) {
        free(*item);
    }

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file (%s).\n", ini_file);
        g_key_file_free(localini);

        if (error != NULL) {
            g_error_free(error);
        }

        //*item =  def_value;
        if (def_value != NULL) {
            *item = malloc(strlen(def_value) + 1);
            strcpy(*item, def_value);
        }

        return;
    }


    *item = g_key_file_get_string(localini, section, key, &error);
    g_key_file_free(localini);

    if (error != NULL) {
        g_error_free(error);
    }


    if (*item == NULL) {
        if (def_value != NULL) {
            *item = malloc(strlen(def_value) + 1);
            strcpy(*item, def_value);
        }
    }
    airnav_log_level(4, "Returning value: %s\n", *item);
    return;

}

/*
 * New function to read integer from
 * ini file using GLIB
 */
int ini_getInteger(char *ini_file, char *section, char *key, int def_value) {
    GKeyFile *localini;
    GError *error = NULL;
    int abc = 0;

    localini = g_key_file_new();

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file.\n");
        g_key_file_free(localini);
        if (error != NULL) {
            g_error_free(error);
        }
        return def_value;
    }

    abc = g_key_file_get_integer(localini, section, key, &error);
    g_key_file_free(localini);

    if (error != NULL) {
        if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || error->code == G_KEY_FILE_ERROR_INVALID_VALUE || error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            g_error_free(error);
            return def_value;
        } else {
            g_error_free(error);
            return abc;
        }
    } else {
        return abc;
    }


}

/*
 * New function to read boolnea from
 * ini file using GLIB
 */
int ini_getBoolean(char *ini_file, char *section, char *key, int def_value) {
    GKeyFile *localini;
    GError *error = NULL;
    int abc = 0;

    localini = g_key_file_new();

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file.\n");
        g_key_file_free(localini);
        if (error != NULL) {
            g_error_free(error);
        }
        return def_value;
    }

    abc = g_key_file_get_boolean(localini, section, key, &error);
    g_key_file_free(localini);

    if (error != NULL) {
        if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || error->code == G_KEY_FILE_ERROR_INVALID_VALUE || error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            g_error_free(error);
            return def_value;
        } else {
            g_error_free(error);
            return abc;
        }
    } else {
        return abc;
    }
}

/*
 * New function to save ini files
 */
int ini_saveGeneric(char *ini_file, char *section, char *key, char *value) {

    GKeyFile *localini;
    localini = g_key_file_new();
    GError *error = NULL;
    gsize length;

    if (access(ini_file, F_OK) == -1) {


        FILE *fp = NULL;
        fp = fopen(ini_file, "w");
        if (fp != NULL) {

            fprintf(fp, "\n");
            fclose(fp);
        }
    }

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file for save.\n");
        g_key_file_free(localini);

        if (error != NULL) {
            g_error_free(error);
        }
        return 0;
    }

    g_key_file_set_string(localini, section, key, value);

    gchar *tmpdata;
    tmpdata = g_key_file_to_data(localini, &length, &error);


    FILE *inif;
    if ((inif = fopen(ini_file, "w")) == NULL) {
        airnav_log("Can't open ini file for writting\n");
        if (tmpdata != NULL) {
            free(tmpdata);
        }
        return 0;
    }
    fprintf(inif, "%s", tmpdata);
    fclose(inif);


    g_key_file_free(localini);
    if (error != NULL) {
        g_error_free(error);
    }

    if (tmpdata != NULL) {
        free(tmpdata);
    }

    return 1;
}

int ini_saveDouble(char *ini_file, char *section, char *key, double value) {

    GKeyFile *localini;
    localini = g_key_file_new();
    GError *error = NULL;
    gsize length;

    if (access(ini_file, F_OK) == -1) {


        FILE *fp = NULL;
        fp = fopen(ini_file, "w");
        if (fp != NULL) {

            fprintf(fp, "\n");
            fclose(fp);
        }
    }


    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file for save.\n");
        g_key_file_free(localini);

        if (error != NULL) {
            g_error_free(error);
        }
        return 0;
    }

    g_key_file_set_double(localini, section, key, value);

    gchar *tmpdata;
    tmpdata = g_key_file_to_data(localini, &length, &error);



    FILE *inif;
    if ((inif = fopen(ini_file, "w")) == NULL) {
        airnav_log("Can't open ini file for writting\n");
        return 0;
    }
    fprintf(inif, "%s", tmpdata);
    fclose(inif);


    g_key_file_free(localini);
    if (error != NULL) {
        g_error_free(error);
    }
    return 1;
}

int ini_saveInteger(char *ini_file, char *section, char *key, int value) {

    GKeyFile *localini;
    localini = g_key_file_new();
    GError *error = NULL;
    gsize length;

    if (access(ini_file, F_OK) == -1) {


        FILE *fp = NULL;
        fp = fopen(ini_file, "w");
        if (fp != NULL) {

            fprintf(fp, "\n");
            fclose(fp);
        }
    }


    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file for save.\n");
        g_key_file_free(localini);

        if (error != NULL) {
            g_error_free(error);
        }
        return 0;
    }

    g_key_file_set_integer(localini, section, key, value);

    gchar *tmpdata;
    tmpdata = g_key_file_to_data(localini, &length, &error);

    FILE *inif;
    if ((inif = fopen(ini_file, "w")) == NULL) {
        airnav_log("Can't open ini file for writting\n");
        if (tmpdata != NULL) {
            free(tmpdata);
        }
        return 0;
    }
    fprintf(inif, "%s", tmpdata);
    fclose(inif);


    g_key_file_free(localini);
    if (error != NULL) {
        g_error_free(error);
    }
    if (tmpdata != NULL) {
        free(tmpdata);
    }
    return 1;
}

/*
 * New function to read double from
 * ini file using GLIB
 */
double ini_getDouble(char *ini_file, char *section, char *key, double def_value) {
    GKeyFile *localini;
    GError *error = NULL;
    double abc = 0;

    localini = g_key_file_new();

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file.\n");
        g_key_file_free(localini);
        if (error != NULL) {
            g_error_free(error);
        }
        return def_value;
    }

    abc = g_key_file_get_double(localini, section, key, &error);
    g_key_file_free(localini);

    if (error != NULL) {
        if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || error->code == G_KEY_FILE_ERROR_INVALID_VALUE || error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            g_error_free(error);
            return def_value;
        } else {
            g_error_free(error);
            return abc;
        }
    } else {
        return abc;
    }
}

/*
 * Return client type
 */
ClientType getClientType(void) {

    if (strcmp(F_ARCH, "raspberry") == 0) {
        return CLIENT_TYPE__RPI;
    } else if (strcmp(F_ARCH, "rblc") == 0) {
        return CLIENT_TYPE__RBLC;
    } else if (strcmp(F_ARCH, "rblc2") == 0) {
        return CLIENT_TYPE__RBLC2;
    } else if (strcmp(F_ARCH, "rbcs") == 0) {
        return CLIENT_TYPE__RBCS;
    } else if (strcmp(F_ARCH, "pc_x86") == 0) {
        return CLIENT_TYPE__PC_X86;
    } else if (strcmp(F_ARCH, "pc_x64") == 0) {
        return CLIENT_TYPE__PC_X64;
    } else {

        airnav_log_level(1, "Entering else....\n");
#ifdef __arm__        
        //airnav_log_level(1,"Client type: GENERIC_ARM_32\n");    
        return CLIENT_TYPE__GENERIC_ARM_32;
#endif

#ifdef __aarch64__
        //airnav_log_level(1,"Client type: GENERIC_ARM_64\n");    
        return CLIENT_TYPE__GENERIC_ARM_64;
#endif    

#ifdef __i386__    
        //airnav_log_level(1,"Client type: PX_X86\n");
        return CLIENT_TYPE__PC_X86;
#endif      

#ifdef __amd64__    
        //airnav_log_level(1,"Client type: PC_X64\n");
        return CLIENT_TYPE__PC_X64;
#endif    
        //  return CLIENT_TYPE__OTHER;


    }


}

/*
 * Return the number of strings in char array
 */
int getArraySize(char *array) {
    int i = 0;
    if (array != NULL) {
        while (array[i] != '\0') {
            i++;
        }
    }
    return i;
}

/*
 * Return RPi CPU Serial Number
 */
long long unsigned getRpiSerial(void) {
    static long long unsigned serial = 0;

    FILE *filp;
    char buf[512];
    //char term;

    filp = fopen("/proc/cpuinfo", "r");

    if (filp != NULL) {
        while (fgets(buf, sizeof (buf), filp) != NULL) {
            if (!strncasecmp("serial\t\t:", buf, 9)) {
                sscanf(buf + 9, "%Lx", &serial);
            }
        }

        fclose(filp);
    }

    return serial;
}

/*
 * Return if this device is property of Airnav or not
 */
int is_airnav_product(void) {

    if ((strcmp(F_ARCH, "rbcs") == 0) || (strcmp(F_ARCH, "rblc") == 0) || (strcmp(F_ARCH, "rblc2") == 0) || (c_type == CLIENT_TYPE__RBCS) || (c_type == CLIENT_TYPE__RBLC) || (c_type == CLIENT_TYPE__RBLC2)) {
        return 1;
    } else {
        return 0;
    }


}

void run_cmd2(char *cmd) {
    pid_t pid;
    char *argv[] = {"sh", "-c", cmd, NULL};
    int status;
    airnav_log_level(2, "Run command: %s\n", cmd);
    status = posix_spawn(&pid, "/bin/sh", NULL, NULL, argv, environ);
    if (status == 0) {
        airnav_log_level(2, "Child pid: %i\n", pid);
    } else {
        airnav_log_level(2, "posix_spawn: %s\n", strerror(status));
    }
}

/*
 * Get CPU Temperature
 */
float getCPUTemp(void) {

#ifdef __arm__

#ifndef RBCSRBLC
    float systemp, millideg;
    FILE *thermal;
    //    int n;

    thermal = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    //n =
    if (fscanf(thermal, "%f", &millideg) < 1) {
        airnav_log_level(1, "Error getting thermal info\n");
    }
    fclose(thermal);
    systemp = millideg / 1000;

    if (systemp > max_cpu_temp) {
        max_cpu_temp = systemp;
    }
    airnav_log_level(4, "CPU temperature is %.2f degrees C\n", systemp);
    return systemp;
#else

    double temp_cpu = 0;
    double temp = 0;
    temp_cpu = (float) mmio_read(0x01c25020);

    temp = (double) (((double) temp_cpu - (double) 1447) / (double) 10);
    if (temp < 0 || temp > 250) {
        temp = 0;
    }


    if (temp > max_cpu_temp) {
        max_cpu_temp = temp;
    }

    //    airnav_log_level(3, "CPU temp.: %.2fC (%.2fC Max)\n", temp, max_cpu_temp);

    return temp;


#endif



#else
    return 0;
#endif

    return 0;
}

int file_exist(char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

pid_t run_cmd3(char *cmd) {
    pid_t pid;
    char *argv[30];
    int i = 0;
    int status;
    char path[1000];

    airnav_log_level(2, "Run command: %s\n", cmd);
    argv[0] = strtok(cmd, " ");
    strcpy(path, argv[0]);

    while (argv[i] != NULL) {
        argv[++i] = strtok(NULL, " ");
    }

    airnav_log_level(2, "cmd: %s\n", path);
    for (i = 0; argv[i] != NULL; i++) {
        airnav_log_level(2, "cmd arg %d: %s\n", i, argv[i]);
    }

    status = posix_spawn(&pid, path, NULL, NULL, argv, environ);

    if (status == 0) {
        airnav_log_level(2, "Child pid: %i\n", pid);
    } else {
        airnav_log_level(2, "posix_spawn: %s\n", strerror(status));
        pid = -1;
    }

    return pid;
}

char *airnav_concat(char* ori, const char* format, ...) {
    MODES_NOTUSED(ori);
    MODES_NOTUSED(format);
    unsigned int buff_size = 4096;

    char *msg = calloc(buff_size, sizeof (char));
    va_list ap;

    if (ori == NULL) {
        ori = calloc(buff_size, sizeof (char));
    }

    va_start(ap, format);
    vsnprintf(msg, buff_size, format, ap);
    va_end(ap);
    char *out = calloc(strlen(ori) + (strlen(msg) + 1), sizeof (char));

    sprintf(out, "%s%s", ori, msg);
    free(ori);
    free(msg);
    return out;

}

/*
 * Parse text into a JSON object. If text is valid JSON, returns a
 * json_t structure, otherwise prints and error and returns null.
 */
json_t *load_json(const char *text) {
    json_t *root;
    json_error_t error;

    root = json_loads(text, 0, &error);

    if (root) {
        return root;
    } else {
        //fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return (json_t *) 0;
    }
}

/*
 * New function check if section exists in .ini file
 */
int ini_hasSection(char *ini_file, char *section) {
    GKeyFile *localini;
    GError *error = NULL;
    localini = g_key_file_new();

    if (g_key_file_load_from_file(localini, ini_file, G_KEY_FILE_KEEP_COMMENTS, &error) == FALSE) {
        airnav_log("Error loading ini file (%s).\n", ini_file);
        g_key_file_free(localini);

        if (error != NULL) {
            g_error_free(error);
        }

        return 0;
    }

    if (g_key_file_has_group(localini, section)) {
        g_key_file_free(localini);
        return 1;
    } else {
        g_key_file_free(localini);
        return 0;
    }

    g_key_file_free(localini);


    return 0;
}

long fseek_filesize(const char *filename) {
    FILE *fp = NULL;
    long off;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        airnav_log_level(2, "failed to fopen %s\n", filename);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) == -1) {
        airnav_log_level(2, "failed to fseek %s\n", filename);
        return -1;
    }

    off = ftell(fp);
    if (off == (long) - 1) {
        airnav_log_level(2, "failed to ftell %s\n", filename);
        return -1;
    }

    airnav_log_level(2, "[*] fseek_filesize - file: %s, size: %ld\n", filename, off);

    if (fclose(fp) != 0) {
        airnav_log_level(2, "failed to fclose %s\n", filename);
        return -1;
    }

    return off;

}

/*
 * Function to remove PID file
 */
void removePidFile(void) {
    if (remove(pidfile) != 0) {
        airnav_log("Error removing PID file.\n");
    }
    return;
}

/*
 * Function to create PID file
 */
void createPidFile(void) {
    char *dir_name;
    char *dirc;

    dirc = strdup(pidfile);
    dir_name = dirname(dirc);
    airnav_log_level(3, "Dir name for pidfile: %s\n", dir_name);

    // check if dir exists, or create
    DIR* dir = opendir(dir_name);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
        // Create PID file
        FILE *f = fopen(pidfile, "w");
        if (f == NULL) {
            airnav_log_level(1, "Cannot write pidfile: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            fprintf(f, "%ld\n", (long) getpid());
            fclose(f);
        }

    } else {
        airnav_log_level(3, "PID Directory does not exist\n");
        free(dir_name);
        //free(dirc);
    }

    return;

}

/* function to check whether the position is set to 1 or not */
int check_bit(int number, int position) {
    return (number >> position) & 1;
}

void set_bit(uint32_t *number, int position) {
    *number |= 1 << position;
}

void clear_bit(uint32_t *number, int position) {
    *number &= ~(1 << position);
}
