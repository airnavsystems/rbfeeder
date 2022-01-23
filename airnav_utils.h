/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_UTILS_H
#define AIRNAV_UTILS_H

//#include "track.h"
#include "dump1090.h"
#include "rbfeeder.pb-c.h"
#include "rbfeeder.h"
#include <glib.h>
#include <spawn.h>

#define airnav_log_level(...) airnav_log_level_m( __FUNCTION__ , __VA_ARGS__)
#define airnav_log_file(...) airnav_log_file_m( __FUNCTION__ , __VA_ARGS__)
#define airnav_log_file_pure(...) airnav_log_file_pure_m( __VA_ARGS__)


#ifdef __cplusplus
extern "C" {
#endif

    extern char **environ;

    void airnav_log_file_m(const char* fname, const char* filename, const char* format, ...);
    void airnav_log_file_pure_m(const char* filename, const char* format, ...);
    void airnav_log(const char* format, ...);
    void airnav_log_level_m(const char* fname, const int level, const char* format, ...);
    void ini_getString(char **item, char *ini_file, char *section, char *key, char *def_value);
    int ini_getInteger(char *ini_file, char *section, char *key, int def_value);
    int ini_getBoolean(char *ini_file, char *section, char *key, int def_value);
    int ini_saveGeneric(char *ini_file, char *section, char *key, char *value);
    int ini_saveDouble(char *ini_file, char *section, char *key, double value);
    int ini_saveInteger(char *ini_file, char *section, char *key, int value);
    double ini_getDouble(char *ini_file, char *section, char *key, double def_value);
    ClientType getClientType(void);
    int getArraySize(char *array);
    long long unsigned getRpiSerial(void);
    int is_airnav_product(void);
    void run_cmd2(char *cmd);
    float getCPUTemp(void);
    int file_exist(char *filename);
    pid_t run_cmd3(char *cmd);
    char *airnav_concat(char* ori, const char* format, ...);
    json_t *load_json(const char *text);
    int ini_hasSection(char *ini_file, char *section);
    long fseek_filesize(const char *filename);
    void removePidFile(void);
    void createPidFile(void);
    int check_bit(int number, int position);
    void set_bit(uint32_t *number, int position);
    void clear_bit(uint32_t *number, int position);


#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_UTILS_H */

