/* 
 * File:   rbfeeder.h 
 *
 * Created on 13 de Fevereiro de 2020, 11:49
 */

#ifndef RBFEEDER_H
#define RBFEEDER_H
#define AN_NOTUSED(V) ((void) V)
#define FREE(ptr) do{ free((ptr)); (ptr) = NULL;  }while(0);
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <jansson.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <net/if.h> 
#include <inttypes.h>
#include <libgen.h>
#include "airnav_types.h"
#include "airnav_main.h"
#include "airnav_utils.h"
#include "airnav_rtlpower.h"
#include "airnav_asterix.h"
#include "airnav_vhf.h"
#include "airnav_mlat.h"
#include "airnav_dumprb.h"
#include "airnav_acars.h"
#include "airnav_uat.h"
#include "net_io.h"
#include "airnav_anrb.h"
#include "airnav_geomag.h"
#include "airnav_scom.h"


#ifdef __cplusplus
extern "C" {
#endif


#define BUFFLEN 4096
#define AIRNAV_INIFILE "/etc/rbfeeder.ini"
#ifndef DEF_XOR_KEY
#define DEFAULT_XOR_KEY "abcd"
#else
#define DEFAULT_XOR_KEY DEF_XOR_KEY    
#endif    
#define RPI_LED_STATUS 26
#define RPI_LED_ADSB 5
#define MAX_ANRB 10
#define AIRNV_STATISTICS_INTERVAL 60
#define AIRNAV_STATS_SEND_TIME 300 // In seconds
#define AIRNAV_MAX_ITEM_AGE 3000ULL // 3 Seconds - send interval
#define AIRNAV_SEND_INTERVAL 3 // 3 second
    // Minimum time for sending each field (if data is the same), in seconds
#define MAX_TIME_FIELD_ALTITUDE         60
#define MAX_TIME_FIELD_MAG_HEADING      60
#define MAX_TIME_FIELD_GS               60
#define MAX_TIME_FIELD_GEOM_RATE        60
#define MAX_TIME_FIELD_BARO_RATE        60
#define MAX_TIME_FIELD_SQUAWKE          120 // 2 minutes
#define MAX_TIME_FIELD_IAS              60
#define MAX_TIME_FIELD_TAS              60
#define MAX_TIME_FIELD_TRACK            60
#define MAX_TIME_FIELD_TRUE_HEADING     60    
#define MAX_TIME_FIELD_ROLL             60    
#define MAX_TIME_FIELD_MRAR_WIND        180    
#define MAX_TIME_FIELD_MRAR_PRESSURE    180    
#define MAX_TIME_FIELD_MRAR_TEMPERATURE 180    
#define MAX_TIME_FIELD_MRAR_HUMIDITY    180    
#define MAX_TIME_FIELD_MRAR_TURBULENCE  180    
#define MAX_TIME_FIELD_ADSB_VERSION     180
#define MAX_TIME_FIELD_ADSR_VERSION     180
#define MAX_TIME_FIELD_TISB_VERSION     180
#define MAX_TIME_FIELD_CALLSIGN         60
#define MAX_TIME_FIELD_NAV_MODES        180 // 3 minutes
#define MAX_TIME_FIELD_AIRBORNE         180 // 3 minutes
#define MAX_TIME_FIELD_WIND             180 // 3 minutes
#define MAX_TIME_FIELD_TEMPERATURE      180 // 3 minutes
#define MAX_TIME_FIELD_NAV_QNH          180 // 3 minutes
#define MAX_TIME_FIELD_NAV_ALT_FMS      180 // 3 minutes
#define MAX_TIME_FIELD_NAV_ALT_MCP      180 // 3 minutes
#define MAX_TIME_FIELD_POS_NIC          180 // 3 minutes
#define MAX_TIME_FIELD_NAC_P            180 // 3 minutes
#define MAX_TIME_FIELD_NAC_V            180 // 3 minutes
#define MAX_TIME_FIELD_NIC_BARO         180 // 3 minutes
#define MAX_TIME_FIELD_SIL              180 // 3 minutes


    // Constants for calculations
#define FEET_TO_M(FT) FT*0.3048
#define KNOT_TO_MS(KTS) KTS*0.514444
#define MS_TO_KNOT(MS) MS*1.9438444924406
#define STRATOSPHERE_BASE_HEIGHT 11000
#define KELVIN_TO_C(K) K-273.15

#define TO_DEGREES(R) (R*180.0/M_PI)
#define TO_RADIANS(D) (M_PI*D)/180.0


    extern char *pidfile;
    extern int disable_log;
    extern int daemon_mode;
    extern char *log_file;
    extern int debug_level;
    extern uint64_t c_version_int; // Store version in integer format
    extern int device_n;
    extern int net_mode;
    extern char *configuration_file;
    extern char *sharing_key;
    extern char *sn;
    extern char *xorkey;
    extern double g_lat;
    extern double g_lon;
    extern int g_alt;
    extern int use_gnss;
    extern struct packet_list *flist;
    extern struct packet_list *flist2;
    extern int rf_filter_status;
    extern int led_pin_adsb;
    extern int led_pin_status;
    extern int use_leds;
    extern int send_beast_config;
    extern int send_weather_data;
    extern struct client *c;
    extern struct net_service *beast_input;
    extern struct net_service *raw_input;
    extern struct s_anrb anrbList[MAX_ANRB];
    extern char start_datetime[100];
    extern int packet_cache_count; // How many packets we have in cache
    extern int packet_list_count;
    extern int currently_tracked_flights;
    extern pthread_mutex_t m_copy; // Mutex copy
    extern pthread_mutex_t m_serial; // Serial mutex
    extern pthread_t t_monitor;
    extern pthread_t t_statistics;
    extern pthread_t t_stats;
    extern pthread_t t_send_data;
    extern pthread_t t_serial_data;
    extern pthread_t t_prepareData;
    extern double max_cpu_temp;
    extern ClientType c_type;
    extern int fd_serial;
    extern char *serial_device;
    extern int32_t serial_speed;
    extern struct termios tty;
    extern int serial_use_att;
    extern int serial_bias_t;
    extern int serial_old_firmware;
    extern int debug_level_cmd;
    extern char *debug_filter;
    


    void receiverPositionChanged(float lat, float lon, float alt);
    void rbfeederSigintHandler(int dummy);
    void rbfeederSigtermHandler(int dummy);
    int doRtlPower(void);


#ifdef __cplusplus
}
#endif

#endif /* RBFEEDER_H */

