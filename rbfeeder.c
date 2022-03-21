// RBFeeder 
//
// Copyright (c) 2020 - AirNav Systems
//
// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// Copyright (c) 2014,2015 Oliver Jowett <oliver@mutability.co.uk>
//
// This file is free software: you may copy, redistribute and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 2 of the License, or (at your
// option) any later version.
//
// This file is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// This file incorporates work covered by the following copyright and
// permission notice:
//
//   Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
//
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are
//   met:
//
//    *  Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//    *  Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "rbfeeder.h"
#include "dump1090.h"

struct _Modes Modes;
char *pidfile;
int disable_log;
int daemon_mode;
char *log_file;
int debug_level;
uint64_t c_version_int = 0;
int device_n = -1;
int net_mode = 0;
char *configuration_file = NULL;
char *sharing_key = NULL;
char *sn = NULL;
char *xorkey;
double g_lat;
double g_lon;
int g_alt;
int use_gnss;
struct packet_list *flist;
struct packet_list *flist2;
int rf_filter_status;
int led_pin_adsb;
int led_pin_status;
int use_leds;
int send_beast_config;
int send_weather_data;
struct client *c;
struct net_service *beast_input;
struct net_service *raw_input;
struct s_anrb anrbList[MAX_ANRB];
char start_datetime[100] = {0};
int packet_cache_count = 0; // How many packets we have in cache
int packet_list_count = 0;
int currently_tracked_flights = 0;
double max_cpu_temp = 0;
ClientType c_type = CLIENT_TYPE__OTHER;
char *serial_device = NULL;
int32_t serial_speed = 921600;


pthread_mutex_t m_copy; // Mutex copy
pthread_t t_monitor;
pthread_t t_statistics;
pthread_t t_stats;
pthread_t t_send_data;
pthread_t t_serial_data;
pthread_t t_prepareData;

void rbfeederSigintHandler(int dummy) {
    MODES_NOTUSED(dummy);
    signal(SIGINT, SIG_DFL); // reset signal handler - bit extra safety
    Modes.exit = 1; // Signal to threads that we are done

#ifdef RBCSRBLC
    led_off(LED_ADSB);
    led_off(LED_STATUS);
    led_off(LED_GPS);
    led_off(LED_PC);
    led_off(LED_ERROR);
    led_off(LED_VHF);
#endif

    // LED's nor available (this way) for RBCS/RBLC
#ifdef ENABLE_LED
    if (use_leds) {
        digitalWrite(RPI_LED_ADSB, LOW);
        digitalWrite(RPI_LED_STATUS, LOW);
    }
#endif
    printf("Exiting.... (sigint)\n");
}

void rbfeederSigtermHandler(int dummy) {
    MODES_NOTUSED(dummy);
    signal(SIGTERM, SIG_DFL); // reset signal handler - bit extra safety
    Modes.exit = 1; // Signal to threads that we are done
#ifdef RBCSRBLC
    led_off(LED_ADSB);
    led_off(LED_STATUS);
    led_off(LED_GPS);
    led_off(LED_PC);
    led_off(LED_ERROR);
    led_off(LED_VHF);
#endif

    // LED's nor available (this way) for RBCS/RBLC
#ifdef ENABLE_LED
    if (use_leds) {
        digitalWrite(RPI_LED_ADSB, LOW);
        digitalWrite(RPI_LED_STATUS, LOW);
    }
#endif

    printf("Exiting.... (sigterm)\n");
}

void receiverPositionChanged(float lat, float lon, float alt) {
    /* nothing */
    (void) lat;
    (void) lon;
    (void) alt;
}

static void rbfeeder_init(void) {
    // Validate the users Lat/Lon home location inputs
    if ((Modes.fUserLat > 90.0) // Latitude must be -90 to +90
            || (Modes.fUserLat < -90.0) // and
            || (Modes.fUserLon > 360.0) // Longitude must be -180 to +360
            || (Modes.fUserLon < -180.0)) {
        Modes.fUserLat = Modes.fUserLon = 0.0;
    } else if (Modes.fUserLon > 180.0) { // If Longitude is +180 to +360, make it -180 to 0
        Modes.fUserLon -= 360.0;
    }

    Modes.bUserFlags &= ~MODES_USER_LATLON_VALID;
    if ((Modes.fUserLat != 0.0) || (Modes.fUserLon != 0.0)) {
        Modes.bUserFlags |= MODES_USER_LATLON_VALID;
    }

    modesChecksumInit(1);
    icaoFilterInit();
    modeACInit();


    rfsurvey_execute = 0;
    rfsurvey_dongle = 0;
    rfsurvey_min = 24;
    rfsurvey_max = 1700;
    asterix_spec_path = NULL;
    global_data_sent = 0;
    // Get MAC ADDress
    mac_a = net_get_mac_address(1);
    

}


static void backgroundTasks(void) {
   icaoFilterExpire();
   trackPeriodicUpdate();
   modesNetPeriodicWork();

   static uint64_t next_stats_update;
   static uint64_t next_json, next_history;
   uint64_t now = mstime();

   // always update end time so it is current when requests arrive
   Modes.stats_current.end = mstime();

   if (now >= next_stats_update) {
       int i;

       if (next_stats_update == 0) {
           next_stats_update = now + 60000;
       } else {
           Modes.stats_newest_1min = (Modes.stats_newest_1min + 1) % 15;
           Modes.stats_1min[Modes.stats_newest_1min] = Modes.stats_current;

           add_stats(&Modes.stats_current, &Modes.stats_alltime, &Modes.stats_alltime);
           add_stats(&Modes.stats_current, &Modes.stats_periodic, &Modes.stats_periodic);

           reset_stats(&Modes.stats_5min);
           for (i = 0; i < 5; ++i)
               add_stats(&Modes.stats_1min[(Modes.stats_newest_1min - i + 15) % 15], &Modes.stats_5min, &Modes.stats_5min);

           reset_stats(&Modes.stats_15min);
           for (i = 0; i < 15; ++i)
               add_stats(&Modes.stats_1min[i], &Modes.stats_15min, &Modes.stats_15min);

           reset_stats(&Modes.stats_current);
           Modes.stats_current.start = Modes.stats_current.end = now;

           if (Modes.json_dir)
               writeJsonToFile("rbfeeder_stats.json", generateStatsJson);

           next_stats_update += 60000;
       }
   }


   if (Modes.json_dir && now >= next_json) {
       writeJsonToFile("rbfeeder_aircraft.json", generateAircraftJson);
       writeJsonToFile("rbfeeder_status.json", airnav_generateStatusJson);
       next_json = now + Modes.json_interval;
   }

   if (now >= next_history) {
       int rewrite_receiver_json = (Modes.json_dir && Modes.json_aircraft_history[HISTORY_SIZE - 1].content == NULL);

       free(Modes.json_aircraft_history[Modes.json_aircraft_history_next].content); // might be NULL, that's OK.
       Modes.json_aircraft_history[Modes.json_aircraft_history_next].content =
               generateAircraftJson("/data/rbfeeder_aircraft.json", &Modes.json_aircraft_history[Modes.json_aircraft_history_next].clen);

       if (Modes.json_dir) {
           char filebuf[PATH_MAX];
           snprintf(filebuf, PATH_MAX, "rbfeeder_history_%d.json", Modes.json_aircraft_history_next);
           writeJsonToFile(filebuf, generateHistoryJson);
       }

       Modes.json_aircraft_history_next = (Modes.json_aircraft_history_next + 1) % HISTORY_SIZE;

       if (rewrite_receiver_json)
           writeJsonToFile("rbfeeder_receiver.json", generateReceiverJson); // number of history entries changed

       next_history = now + HISTORY_INTERVAL;
   }

}

 
static int connectLocalDump(void) {
   char *bo_connect_ipaddr = "127.0.0.1";
   int bo_connect_port = (external_port + 100);

   if (dumprb_cmd == NULL) {
       airnav_log("\n");
       airnav_log("\n");
       airnav_log("Configuration is set to use local dongle, but rbfeeder\n");
       airnav_log("can't locate dump1090 executable.\n");
       airnav_log("Please check your configuration or define dump1090-rb executable using: \n");
       airnav_log("\n");
       airnav_log("[client]\n");
       airnav_log("dumprb_cmd=/usr/bin/dump1090-rb\n");
       airnav_log("\n");
       exit(EXIT_FAILURE);
   }

   if (!dumprb_checkDumprbRunning()) {
       dumprb_startDumprb();
       if (!dumprb_checkDumprbRunning()) {
           airnav_log_level(2, "dump1090-rb is not running!\n");
           //return 0;
       } else {
           airnav_log_level(2, "Dump1090-rb started!!!!!\n");
           //return 1;
       }
   }

   // Set up input connection
   beast_input = makeBeastInputService();
   c = serviceConnect(beast_input, bo_connect_ipaddr, bo_connect_port);

   if (c && beast_input->connections > 0) {
        
       if (send_beast_config == 1) {
           sendBeastSettings(c, "CdfJV");
       }
        
       airnav_log_level(2, "Local dump connected!\n");
       return 1;
   } else {
       return 0;
   }

}

static int connectExtBeast(void) {
   // Set up input connection
   beast_input = makeBeastInputService();
   c = serviceConnect(beast_input, external_host, external_port);

   if (c) {
       if (send_beast_config == 1) {
           sendBeastSettings(c, "CdfJV");
       }
       airnav_log_level(2, "External host connected!\n");
       return 1;
   } else {
       airnav_log_level(2, "Can't connect to external host (%s:%d)\n", external_host, external_port);
       return 0;
   }

}

static int connectExtRaw(void) {
   // Set up input connection
    
   raw_input = makeRawInputService();
   c = serviceConnect(raw_input, external_host, external_port);

   if (c) {
       airnav_log_level(2, "External host connected!\n");
       return 1;
   } else {
       airnav_log_level(2, "Can't connect to external host (%s:%d)\n", external_host, external_port);
       return 0;
   }

}

static int connectData(void) {

   if (!net_mode) {
       return connectLocalDump();
   } else {
       if (net_mode == 1) {
           return connectExtRaw();
       } else if (net_mode == 2) {
           return connectExtBeast();
       }
   }

   return -1;
}
  

int doRtlPower(void) {
   struct timespec r = {1, 100 * 1000 * 1000};
   int int_count = 0;
   int rtl_timeout = 300;

   if (rtlpower_checkRtlpowerRunning() == 0) {
       rtlpower_startRtlpower();
       sleep(2);
   }

   while (rtlpower_checkRtlpowerRunning() == 1 && int_count < rtl_timeout) {
       backgroundTasks();
       nanosleep(&r, NULL);
       airnav_log_level(2, "rtl_power is running! Waiting finish (%d)....\n", int_count);
       int_count++;
   }

   long fsize = fseek_filesize(rfsurvey_file);
   airnav_log_level(2, "Filesize after rtl_power: %ld\n", fsize);

   if (int_count >= rtl_timeout) {
       airnav_log("RFSurvey timeout (%d). Forcing exit...\n", rtl_timeout);
       rtlpower_stopRtlpower();
       rfsurvey_execute = 0;
       return 1;
   }

   if (fsize >= 40000 && sn != NULL) { // 40k
       rtlpower_curl_send_rtlpower(rfsurvey_url, sn, rfsurvey_key, rfsurvey_file);
   }

   rfsurvey_execute = 0;
   return 1;
}

//
//=========================================================================
//

int main(int argc, char **argv) {

    MODES_NOTUSED(argc);
    MODES_NOTUSED(argv);

    start_datetime[0] = '\0';
    // application start date/time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(start_datetime, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    // signal handlers:
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    //disable_log = 0;

    memset(&Modes, 0, sizeof (Modes));
    Modes.nfix_crc = 1;
    Modes.check_crc = 1;
    Modes.net = 1;
    Modes.fix_df = 1;
    Modes.net_heartbeat_interval = MODES_NET_HEARTBEAT_INTERVAL;
    Modes.maxRange = 1852 * 360;
    Modes.quiet = 1;
    Modes.net_output_flush_size = MODES_OUT_FLUSH_SIZE;
    Modes.net_output_flush_interval = 200; // milliseconds    
    Modes.json_interval = 1000;
    Modes.json_location_accuracy = 1;
    Modes.json_dir = strdup("/dev/shm/");
    Modes.net_input_raw_ports = strdup("32008");
    Modes.net_output_raw_ports = strdup("32458"); // 
    Modes.net_output_sbs_ports = strdup("32459"); //
    Modes.net_input_beast_ports = strdup("32104"); //
    Modes.net_output_beast_ports = strdup("32457"); //
    Modes.mlat = 1;
    Modes.forward_mlat = 1;

    // Initialization
    airnav_loadConfig(argc, argv);

    rbfeeder_init();
    modesInitNet();
    airnav_main();    
    
    if (serial_device == NULL) {
        connectData();
    }
    
    // Run
    while (!Modes.exit) {
        
        struct timespec r = {0, 100 * 1000 * 1000};

        backgroundTasks();
        nanosleep(&r, NULL);

        if (rfsurvey_execute == 1) {

            if (!net_mode && dumprb_checkDumprbRunning() == 1) {
                dumprb_stopDumprb();
                sleep(1);
            }
            if (dumprb_checkDumprbRunning() == 0) {
                doRtlPower();
            }

        } else {

            if (serial_device == NULL) {                
            
                while (!c && Modes.exit != 1) {
                    struct timespec r2 = {3, 0};
                    nanosleep(&r2, NULL);
                    connectData();
                }

                // In case of connection lost, try to reconnect
                if (net_mode == 2) {
                    if (beast_input->connections == 0) {
                        if (c) {
                            sleep(3);
                            airnav_log_level(2, "Reconnecting...\n");
                            connectData();
                            continue;
                        }
                    }
                } else if (net_mode == 1) {
                    if (raw_input->connections == 0) {
                        if (c) {
                            sleep(3);
                            airnav_log_level(2, "Reconnecting...\n");
                            connectData();
                            continue;
                        }
                    }
                }

            
            } else { // Serial device - Just spend some time
                while (Modes.exit != 1) {
                    sleep(1);
                    // Taking some time...
                }
            }


        }
        
    }

    
    pthread_join(t_waitcmd, NULL);
    pthread_join(t_monitor, NULL);
    pthread_join(t_statistics, NULL);
    pthread_join(t_stats, NULL);
    pthread_join(t_send_data, NULL);
    pthread_join(t_prepareData, NULL);
    pthread_join(t_anrb, NULL);
    pthread_join(t_anrb_send, NULL); 
    
    if (serial_device != NULL) {
        pthread_join(t_serial_data, NULL);
    }
    
    
    if (dump978_enabled) {
        pthread_join(t_dump978, NULL);
    }

    if (mlat_checkMLATRunning()) {
        mlat_stopMLAT();
    }
    if (checkVhfRunning()) {
        stopVhf();
    }
    if (acars_checkACARSRunning()) {
        acars_stopACARS();
    }
    if (uat_check978Running()) {
        uat_stop978();
    }
    if (dumprb_checkDumprbRunning()) {
        dumprb_stopDumprb();
    }

    // Clear flist
    struct packet_list *tmp;
    while (flist != NULL) {
        if (flist->packet != NULL) {
            free(flist->packet);
        }
        tmp = flist;                
        flist = flist->next;
        free(tmp);
    }

    removePidFile();

    airnav_log("Exit success!\n");
    return EXIT_SUCCESS;
}
//
//=========================================================================
//
