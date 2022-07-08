/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef AIRNAV_TYPES_H
#define AIRNAV_TYPES_H

#include <pthread.h>
#include "dump1090.h"

#ifdef __cplusplus
extern "C" {
#endif


    // This struct is only to organize local data before send.
    // This struct will not be send this way!

    typedef struct p_data {
        char cmd;
        uint64_t c_version;
        char c_key[33];
        short c_key_set;
        short c_version_set;
        char c_sn[30];
        int32_t modes_addr;
        short modes_addr_set;
        char callsign[9];
        short callsign_set;
        int32_t altitude;
        short altitude_set;
        int32_t altitude_geo;
        short altitude_geo_set;
        double lat;
        double lon;
        short position_set;
        short heading;
        short heading_set;
        double heading_full;
        short heading_full_set;
        short gnd_speed;
        double gnd_speed_full;
        short gnd_speed_full_set;
        char c_type;
        short c_type_set;
        short gnd_speed_set;
        short ias;
        short ias_set;
        uint32_t tas;
        short tas_set;
        double track;
        short track_set;
        double true_heading;
        short true_heading_set;
        uint32_t ias_full;
        short ias_full_set;
        short vert_rate;
        short vert_rate_set;
        int32_t vert_rate_full;
        short vert_rate_full_set;
        short squawk;
        short squawk_set;
        char airborne;
        short airborne_set;
        char payload[501]; // Payload for error and messages
        short payload_set;
        uint64_t timestp;
        char p_timestamp[25];
        char c_ip[20];
        int32_t extra_flags;
        short extra_flags_set;
        int is_mlat;
        short is_978;
        double roll;
        short roll_set;
        
        // NAV
        short nav_altitude_fms_set;
        unsigned nav_altitude_fms;
        short nav_altitude_mcp_set;
        unsigned nav_altitude_mcp;
        short nav_qnh_set;
        int32_t nav_qnh;
        short nav_heading_set;
        int32_t nav_heading;
        char nav_altitude_src;

        // Nav Modes
        short nav_modes_autopilot_set;
        short nav_modes_vnav_set;
        short nav_modes_alt_hold_set;
        short nav_modes_aproach_set;
        short nav_modes_lnav_set;
        short nav_modes_tcas_set;


        // Weather        
        char weather_source;
        short weather_source_set;
        short wind_dir;
        short wind_dir_set;
        short wind_speed;
        short wind_speed_set;
        int32_t temperature;
        short temperature_set;
        int32_t pressure;
        short pressure_set;
        char humidity;
        short humidity_set;

        // MRAR
        double mrar_wind_speed;
        short mrar_wind_speed_set;
        double mrar_wind_dir;
        short mrar_wind_dir_set;
        double mrar_pressure;
        short mrar_pressure_set;
        hazard_t mrar_turbulence;
        short mrar_turbulence_set;
        double mrar_humidity;
        short mrar_humidity_set;
        double mrar_temperature;
        short mrar_temperature_set;
        

        unsigned pos_nic;
        char pos_nic_set;
        char nic_baro;
        char nic_baro_set;
        unsigned nac_p;
        char nac_p_set;
        unsigned nac_v;
        char nac_v_set;
        unsigned sil;
        char sil_set;
        unsigned sil_type;
        char sil_type_set;

        int32_t adsb_version;
        short adsb_version_set;
        int32_t adsr_version;
        short adsr_version_set;
        int32_t tisb_version;
        short tisb_version_set;

        
    } p_data;

    typedef struct packet_list {
        struct p_data *packet;
        struct packet_list *next;
    } packet_list;

    typedef struct s_anrb {
        int8_t active;
        int32_t port;
        pthread_t s_thread;
        int *socket;
    } s_anrb;

    typedef struct sdongle_version {
        uint major;
        uint minor;
    } sdongle_version;




#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_TYPES_H */

