/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#include "rbfeeder.h"
#include "airnav_main.h"
#include "airnav_scom.h"
#include "airnav_mlat.h"
#include "airnav_uat.h"
#include "airnav_ntp_status.h"

/*
 * Load configuration from ini file
 */
void airnav_loadConfig(int argc, char **argv) {

    char tmp_str[7] = {0};
    char* endptr = "0123456789";
    int j = 0;
    c_version_int = strtoimax(BDTIME, &endptr, 10); // Store version in integer format

    configuration_file = malloc(strlen(AIRNAV_INIFILE) + 1);
    strcpy(configuration_file, AIRNAV_INIFILE);



    short device_n_changed = 0;
    AN_NOTUSED(device_n_changed);
    short show_key = 0;
    AN_NOTUSED(show_key);
    short define_key = 0;
    AN_NOTUSED(define_key);
    short network_mode = 0;
    AN_NOTUSED(network_mode);
    short network_mode_changed = 0;
    AN_NOTUSED(network_mode_changed);
    char *network_host;
    AN_NOTUSED(network_host);
    short network_host_changed = 0;
    AN_NOTUSED(network_host_changed);
    int network_port = 0;
    AN_NOTUSED(network_port);
    short network_port_changed = 0;
    AN_NOTUSED(network_port_changed);
    short no_start = 0;
    AN_NOTUSED(no_start);
    short protocol = 0;
    AN_NOTUSED(protocol);
    short protocol_changed = 0;
    AN_NOTUSED(protocol_changed);    


    // Print version information
    if (argc > 1) {
        for (j = 1; j < argc; j++) {

            if (!strcmp(argv[j], "--version") || !strcmp(argv[j], "-v")) {
                printf("RBFeeder v%s (build %" PRIu64 ")\n", AIRNAV_VER_PRINT, c_version_int);
                exit(EXIT_SUCCESS);
            } else if (!strcmp(argv[j], "--version2") || !strcmp(argv[j], "-v2")) {
                printf("RBFeeder v%s (build %" PRIu64 " Arch: %s)\n", AIRNAV_VER_PRINT, c_version_int, F_ARCH);
                exit(EXIT_SUCCESS);
            } else if (!strcmp(argv[j], "--version3") || !strcmp(argv[j], "-v3")) {
                printf("%" PRIu64, c_version_int);
                exit(EXIT_SUCCESS);
            } else if (!strcmp(argv[j], "--config") || !strcmp(argv[j], "-c")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {
                    configuration_file = strdup(argv[++j]);
                } else {
                    airnav_log("Invalid argument for configuration file (--config).\n");
                    exit(EXIT_FAILURE);
                }
            } else if (!strcmp(argv[j], "--device") || !strcmp(argv[j], "-d")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {
                    Modes.dev_name = strdup(argv[++j]);
                    device_n = atoi(Modes.dev_name);
                    device_n_changed = 1;
                } else {
                    airnav_log("Invalid argument for device (--device).\n");
                    exit(EXIT_FAILURE);
                }
                
            } else if (!strcmp(argv[j], "--debug-filter") || !strcmp(argv[j], "-df")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {
                    debug_filter = strdup(argv[++j]);
                    airnav_log("Debug filter set to '%s'\n",debug_filter);                
                } else {
                    airnav_log("Invalid argument for debug filter (--debug-filter).\n");
                    exit(EXIT_FAILURE);
                }    
                
            } else if (!strcmp(argv[j], "--debug-level") || !strcmp(argv[j], "-dl")) {
                                
                debug_level_cmd = atoi(argv[++j]);
                airnav_log("Debug level set to %d\n",debug_level_cmd);                
                
            } else if (!strcmp(argv[j], "--help") || !strcmp(argv[j], "-h")) {
                airnav_showHelp();
                exit(EXIT_SUCCESS);
            } else if (!strcmp(argv[j], "--showkey") || !strcmp(argv[j], "-sw")) {
                show_key = 1;
            } else if (!strcmp(argv[j], "--setkey") || !strcmp(argv[j], "-sk")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {
                    char *tmp_key;
                    tmp_key = strdup(argv[++j]);
                    if (strlen(tmp_key) != 32) {
                        airnav_log("Invalid sharing key. Sharing key must have exactly 32 chars.\n");
                        exit(EXIT_FAILURE);
                    } else {
                        sharing_key = strdup(tmp_key);
                        define_key = 1;
                    }
                } else {
                    airnav_log("Invalid argument for sharing key.\n");
                    exit(EXIT_FAILURE);
                }

            } else if (!strcmp(argv[j], "--set-network-mode") || !strcmp(argv[j], "-snm")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {

                    if (!strcmp(argv[j + 1], "1") || !strcmp(argv[j + 1], "on")) {
                        network_mode = 1;
                        network_mode_changed = 1;
                        ++j;
                    } else if (!strcmp(argv[j + 1], "0") || !strcmp(argv[j + 1], "off")) {
                        network_mode = 0;
                        network_mode_changed = 1;
                        ++j;
                    } else {
                        airnav_log("Invalid argument for network mode.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    airnav_log("Invalid argument for network mode.\n");
                    exit(EXIT_FAILURE);
                }
            } else if (!strcmp(argv[j], "--set-network-protocol") || !strcmp(argv[j], "-snp")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {

                    if (!strcmp(argv[j + 1], "beast")) {
                        protocol = 1;
                        protocol_changed = 1;
                        ++j;
                    } else if (!strcmp(argv[j + 1], "raw")) {
                        protocol = 2;
                        protocol_changed = 1;
                        ++j;
                    } else {
                        airnav_log("Invalid argument for network protocol.\n");
                        exit(EXIT_FAILURE);
                    }

                } else {
                    airnav_log("Invalid argument for network protocol.\n");
                    exit(EXIT_FAILURE);
                }
            } else if (!strcmp(argv[j], "--set-network-host") || !strcmp(argv[j], "-snh")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {

                    network_host = strdup(argv[++j]);
                    network_host_changed = 1;

                } else {
                    airnav_log("Invalid argument for network host.\n");
                    exit(EXIT_FAILURE);
                }
            } else if (!strcmp(argv[j], "--set-network-port")) {
                if (argc - 1 > j && argv[j + 1][0] != '-') {

                    network_port = atoi(argv[++j]);
                    network_port_changed = 1;

                } else {
                    airnav_log("Invalid argument for network port.\n");
                    exit(EXIT_FAILURE);
                }
            } else if (!strcmp(argv[j], "--interactive")) {
                //Modes.interactive = Modes.throttle = 1;
            } else if (!strcmp(argv[j], "--no-start")) {
                no_start = 1;
            }

        }

    }    

    if (access(configuration_file, F_OK) != -1) {
        airnav_log_level(5, "Configuration file exist and is valid\n");
    } else {
        airnav_log("Configuration file (%s) doesn't exist.\n", configuration_file);
        exit(EXIT_FAILURE);
    }

    if (device_n_changed == 1) {
        ini_saveGeneric(configuration_file, "client", "dump_device", Modes.dev_name);
    } else {
        device_n = ini_getInteger(configuration_file, "client", "dump_device", -1);
    }
    // If device number is set (>-1), set in dump too
    if (device_n > -1) {
        Modes.dev_name = malloc(5);
        //        sprintf(Modes.dev_name, "%d", device_n);
        airnav_log_level(5, "Device number: %s\n", Modes.dev_name);
    }


    if (debug_level_cmd == -1) {
        debug_level = ini_getInteger(configuration_file, "client", "debug_level", 0);
    } else {
        debug_level = debug_level_cmd;
    }        
    
    log_file = NULL;
    ini_getString(&log_file, configuration_file, "client", "log_file", NULL);

    if (define_key == 1) {
        ini_saveGeneric(configuration_file, "client", "key", sharing_key);
    } else {
        ini_getString(&sharing_key, configuration_file, "client", "key", "");
    }
    airnav_log_level(5, "Key carregada: %s\n", sharing_key);

    // Is network mode has changed using parameters, save to ini
    if (network_mode_changed == 1) {
        if (network_mode == 1) {
            ini_saveGeneric(configuration_file, "client", "network_mode", "true");
        } else {
            ini_saveGeneric(configuration_file, "client", "network_mode", "false");
        }
    }

    // Is network protocol has changed using parameters, save to ini
    if (protocol_changed == 1) {
        if (protocol == 1) {
            ini_saveGeneric(configuration_file, "network", "mode", "beast");
        } else {
            ini_saveGeneric(configuration_file, "network", "mode", "raw");
        }
    }

    // If network host has changed
    if (network_host_changed == 1) {
        ini_saveGeneric(configuration_file, "network", "external_host", network_host);
    }

    // If network port has changed
    if (network_port_changed == 1) {
        char *t_port = calloc(1, 20);
        sprintf(t_port, "%d", network_port);
        ini_saveGeneric(configuration_file, "network", "external_port", t_port);
    }

    // This must be the last item
    if (show_key == 1) {

        if (strlen(sharing_key) == 32) {
            printf("\nSharing key: %s\n", sharing_key);
            printf("You can link this sharing key to your account at http://www.radarbox.com\n");
            printf("Configuration file: %s\n\n", configuration_file);
            exit(EXIT_SUCCESS);
        } else {
            printf("\nYour sharing key is not set or is not valid. If is not set, a new sharing key will be create on first connection.\n");
            printf("Configuration file: %s\n\n", configuration_file);
            exit(EXIT_SUCCESS);
        }


    }


    if (no_start == 1) {
        exit(EXIT_SUCCESS);
    }

    // Disable or not log
    disable_log = ini_getBoolean(configuration_file, "client", "disable_log", 0);

    if (disable_log == 1) {
        printf("Log disabled in rbfeeder.ini configuration.\n");
    }

    ini_getString(&airnav_host, configuration_file, "server", "a_host", DEFAULT_AIRNAV_HOST);
    airnav_port = ini_getInteger(configuration_file, "server", "a_port", 33755);
    airnav_port_v2 = ini_getInteger(configuration_file, "server", "a_port_v2", 33756);
    airnav_log_level(2, "Server address: %s\n", airnav_host);
    airnav_log_level(2, "Server port: %d\n", airnav_port);

    // Asterix configuration
    loadAsterixConfiguration();

    // Port for data input in beast format (MLAT Return)
    ini_getString(&beast_in_port, configuration_file, "network", "beast_input_port", "32004");
    ini_getString(&local_input_port, configuration_file, "network", "intern_port", "32008");


    /*
     * Test if client is setup for network data
     * or for local RTL data.
     */

    if (ini_getBoolean(configuration_file, "client", "network_mode", 0)) {
        char *external_host_name = NULL;
        Modes.net = 1;
        Modes.net_only = 1;
        Modes.sdr_type = SDR_NONE;
        airnav_log_level(3, "Network mode selected from ini\n");
        net_mode = 1;

        // Load remote host name from ini file
        ini_getString(&external_host_name, configuration_file, "network", "external_host", NULL);
        if (external_host_name == NULL) {

            airnav_log("When 'network_mode' is enabled, you must specify one remote host for connection using\n");
            airnav_log("'external_host' parameter in [network] section of configuration file.\n");
            airnav_log("If it's your first time running this program, please check configuration file and setup\n");
            airnav_log("basic configuration.\n");
            exit(EXIT_FAILURE);
        }

        // Resolve host name
        external_host = (char *) malloc(100 * sizeof (char));
        if (net_hostname_to_ip(external_host_name, external_host) != 0) {
            airnav_log("Could not resolve host name specified in 'external_host'.\n");
            exit(EXIT_FAILURE);
        }

        // Now we get external port for connection
        external_port = ini_getInteger(configuration_file, "network", "external_port", 0);
        if (external_port == 0) {
            airnav_log("When 'network_mode' is enabled, you must specify one remote port for connection using\n");
            airnav_log("'external_port' parameter in [network] section of configuration file.\n");
            airnav_log("If it's your first time running this program, please check configuration file and setup\n");
            airnav_log("basic configuration.\n");
            exit(EXIT_FAILURE);
        }


        char *network_mode = NULL;
        ini_getString(&network_mode, configuration_file, "network", "mode", NULL);
        // Let's define the network mode (RAW or BEAST)
        if (network_mode == NULL) {
            airnav_log("Unknow network mode (%s). Only RAW and BEAST are supported at this time.\n", tmp_str);
            airnav_log("If it's your first time running this program, please check configuration file and setup\n");
            airnav_log("basic configuration.\n");
            exit(EXIT_FAILURE);
        }


        if (strstr(network_mode, "raw") != NULL) { // RAW mode
            net_mode = 1;
            free(Modes.net_input_raw_ports);
            Modes.net_input_raw_ports = strdup(local_input_port);
        } else if (strstr(network_mode, "beast") != NULL) { // beast mode
            net_mode = 2;
            free(Modes.net_input_beast_ports);
            Modes.net_input_beast_ports = strdup(beast_in_port);
        } else {
            airnav_log("Unknow network mode (%s). Only RAW and BEAST are supported at this time.\n", network_mode);
            airnav_log("If it's your first time running this program, please check configuration file and setup\n");
            airnav_log("basic configuration.\n");
            exit(EXIT_FAILURE);
        }


    } else {
        Modes.net = 1;
        Modes.net_only = 0;
        net_mode = 0;
    }

    external_port = ini_getInteger(configuration_file, "network", "external_port", 30005);

    // Dump978 JSON port to connect
    dump978_port = ini_getInteger(configuration_file, "dump978", "dump978_port", 28380);


    airnav_log("Starting RBFeeder Version %s (build %" PRIu64 ")\n", AIRNAV_VER_PRINT, c_version_int);
    airnav_log("Using configuration file: %s\n", configuration_file);
    if (net_mode > 0) {
        airnav_log("Network-mode enabled.\n");
        airnav_log("\t\tRemote host to fetch data: %s\n", external_host);
        airnav_log("\t\tRemote port: %d\n", external_port);
        airnav_log("\t\tRemote protocol: %s\n", (net_mode == 1 ? "RAW" : "BEAST"));
    } else {
        airnav_log("Network-mode disabled. Using local dongle.\n");
        if (device_n > -1) {
            airnav_log("\tDevice selected in configuration file: %d\n", device_n);
        }
    }


    // configs

    g_lat = ini_getDouble(configuration_file, "client", "lat", 0);
    g_lon = ini_getDouble(configuration_file, "client", "lon", 0);
    g_alt = ini_getInteger(configuration_file, "client", "alt", -999);
    ini_getString(&sn, configuration_file, "client", "sn", NULL);

    dump_gain = ini_getDouble(configuration_file, "client", "dump_gain", -10);
    if (dump_gain != -10) {
        Modes.gain = (int) ((double) dump_gain * (double) 10);
    }

    dump_agc = ini_getBoolean(configuration_file, "client", "dump_agc", 0);
    Modes.nfix_crc = ini_getBoolean(configuration_file, "client", "dump_fix", 1);
    Modes.mode_ac = ini_getBoolean(configuration_file, "client", "dump_mode_ac", 1);
#ifdef RBCSRBLC
    Modes.dc_filter = 0;
#else
    Modes.dc_filter = ini_getBoolean(configuration_file, "client", "dump_dc_filter", 1);
#endif
    Modes.fUserLat = ini_getDouble(configuration_file, "client", "lat", 0.0);
    Modes.fUserLon = ini_getDouble(configuration_file, "client", "lon", 0.0);
    Modes.check_crc = ini_getBoolean(configuration_file, "client", "dump_check_crc", 1);

    ini_getString(&pidfile, configuration_file, "client", "pid", "/var/run/rbfeeder/rbfeeder.pid");

    if (ini_hasSection(configuration_file, "vhf") == 1) {
        // VHF
        loadVhfConfig();

        // Force create of VHF config
        if (generateVHFConfig() == 0) {
            airnav_log("Error creating VHF configuration. Check permissions.\n");
        }
    }

    // MLAT
    mlat_loadMlatConfig();
    
    

    // dump1090-rb
    ini_getString(&dumprb_cmd, configuration_file, "client", "dumprb_cmd", NULL);
    if (dumprb_cmd == NULL) {

        if (file_exist("/usr/local/bin/dump1090-rb")) {
            ini_getString(&dumprb_cmd, configuration_file, "client", "dumprb_cmd", "/usr/local/bin/dump1090-rb");
        }

        if (file_exist("/usr/bin/dump1090-rb")) {
            ini_getString(&dumprb_cmd, configuration_file, "client", "dumprb_cmd", "/usr/bin/dump1090-rb");
        }

    }


    // rtl_power / RFSurvey
    ini_getString(&rtlpower_cmd, configuration_file, "rtl_power", "rtlpower_cmd", NULL);
    if (rtlpower_cmd == NULL) {

        if (file_exist("/usr/bin/rtl_power")) {
            ini_getString(&rtlpower_cmd, configuration_file, "rtl_power", "rtlpower_cmd", "/usr/bin/rtl_power");
        }

        // If still not found, try to locate in /usr/local/bin
        if (rtlpower_cmd == NULL) {
            if (file_exist("/usr/local/bin/rtl_power")) {
                ini_getString(&rtlpower_cmd, configuration_file, "rtl_power", "rtlpower_cmd", "/usr/local/bin/rtl_power");
            }
        }


    }
    ini_getString(&rfsurvey_url, configuration_file, "rtl_power", "rfsurvey_url", DEFAULT_RTLPOWER_URL);
    ini_getString(&rfsurvey_file, configuration_file, "rtl_power", "rfsurvey_file", DEFAULT_RTLPOWER_FILE);
    ini_getString(&rfsurvey_samplerate, configuration_file, "rtl_power", "rfsurvey_samplerate", DEFAULT_RTLPOWER_SAMPLERATE);
    rfsurvey_gain = ini_getDouble(configuration_file, "rtl_power", "rfsurvey_gain", 0);
    rfsurvey_dongle = ini_getInteger(configuration_file, "rtl_power", "rfsurvey_dongle", 0);


    

    // dump978
    uat_loadUatConfig();
    


    // ACARS
    ini_getString(&acars_pidfile, configuration_file, "acars", "pid", NULL);
    ini_getString(&acars_cmd, configuration_file, "acars", "acars_cmd", NULL);
    ini_getString(&acars_server, configuration_file, "acars", "server", "airnavsystems.com:9743");
    acars_device = ini_getInteger(configuration_file, "acars", "device", 1);
    ini_getString(&acars_freqs, configuration_file, "acars", "freqs", "131.550");
    autostart_acars = ini_getBoolean(configuration_file, "acars", "autostart_acars", 0);
    anrb_port = ini_getInteger(configuration_file, "client", "anrb_port", 32088);
    ini_getString(&xorkey, configuration_file, "client", "xorkey", DEFAULT_XOR_KEY);

    // Always enable net mode.
    Modes.quiet = ini_getBoolean(configuration_file, "client", "dump_quiet", 1);
    int closed = 1;


    // Port for data output in beast format
    ini_getString(&beast_out_port, configuration_file, "network", "beast_output_port", "32457");
    ini_getString(&raw_out_port, configuration_file, "network", "raw_output_port", "32458");
    ini_getString(&sbs_out_port, configuration_file, "network", "sbs_output_port", "32459");

    // JSON output to shared-mem dir
    if ((strcmp(F_ARCH, "rbcs") == 0) || (strcmp(F_ARCH, "rblc") == 0) || (strcmp(F_ARCH, "rblc2") == 0)) {
        ini_getString(&Modes.json_dir, configuration_file, "client", "json_dir", "/run/shm/");
        closed = ini_getBoolean(configuration_file, "client", "dump_closed", 1);
    } else if ((strcmp(F_ARCH, "raspberry") == 0)) {
        ini_getString(&Modes.json_dir, configuration_file, "client", "json_dir", "/dev/shm/");
        closed = ini_getBoolean(configuration_file, "client", "dump_closed", 0);
    } else {
        ini_getString(&Modes.json_dir, configuration_file, "client", "json_dir", "/dev/shm/");
        closed = ini_getBoolean(configuration_file, "client", "dump_closed", 1);
    }

    Modes.net = 1;
    Modes.mlat = 1;

    // If we should open or not network mode
    if (closed) {
        free(Modes.net_bind_address);
        Modes.net_bind_address = strdup("127.0.0.1");
    }

    // Enable input for MLAT returning from server
    // Always enable input-port
    free(Modes.net_input_beast_ports);
    Modes.net_input_beast_ports = strdup(beast_in_port);

    // n RBCS, always listen on this port for MLAT client
    free(Modes.net_output_beast_ports);
    Modes.net_output_beast_ports = strdup(beast_out_port);
    free(Modes.net_output_raw_ports);
    Modes.net_output_raw_ports = strdup(raw_out_port);

    free(Modes.net_output_sbs_ports);
    Modes.net_output_sbs_ports = strdup(sbs_out_port);

    free(Modes.net_input_raw_ports);
    Modes.net_input_raw_ports = strdup(local_input_port);



    // Dump978
    ini_getString(&dump978_soapy_params, configuration_file, "dump978", "dump978_soapy_params", "--sdr driver=rtlsdr --sdr-auto-gain");


    rf_filter_status = ini_getBoolean(configuration_file, "client", "rf_filter", 0);

    led_pin_adsb = ini_getInteger(configuration_file, "led", "pin_adsb", RPI_LED_ADSB);
    led_pin_status = ini_getInteger(configuration_file, "led", "pin_status", RPI_LED_STATUS);
    use_leds = ini_getInteger(configuration_file, "led", "active", 0);
    airnav_log_level(2, "Use leds: %d\n", use_leds);

    // GNSS
    use_gnss = ini_getBoolean(configuration_file, "client", "use_gnss", 1);
    if (use_gnss == 1) {
        Modes.use_gnss = 1;
        airnav_log("Using GNSS (when available)\n");
    }

    // Send weather data
    send_weather_data = ini_getBoolean(configuration_file, "client", "send_weather", 0);

    send_beast_config = ini_getBoolean(configuration_file, "client", "send_beast_config", 1);

    // Load serial device configuration
    serial_loadSerialConfig();
    

}

/*
 * Display help message in console
 */
void airnav_showHelp(void) {
    fprintf(stderr, "Starting RBFeeder Version %s\n", BDTIME);
    fprintf(stderr, "\n");
    fprintf(stderr, "\t--config [-c]\t <filename>\t\tSpecify configuration file to use. (default: /etc/rbfeeder.ini)\n");
    fprintf(stderr, "\t--device [-d]\t <number>\t\tSpecify device number to use.\n");
    fprintf(stderr, "\t--showkey [-sw]\t\t\t\tShow current sharing key set in  configuration file.\n");
    fprintf(stderr, "\t--setkey [-sk] <sharing key>\t\tSet sharing key and store in configuration file.\n");
    fprintf(stderr, "\t--set-network-mode <on / off>\t\tEnable or disable network mode (get data from external host).\n");
    fprintf(stderr, "\t--set-network-protocol <beast/raw>\tSet network protocol for external host.\n");
    fprintf(stderr, "\t--set-network-port <port>\t\tSet network port for external host.\n");
    fprintf(stderr, "\t--no-start\t\t\t\tDon't start application, just set options and save to configuration file.\n");
    fprintf(stderr, "\t--help [-h]\t\t\t\tDisplay this message.\n");
    fprintf(stderr, "\n");
}

/*
 * Main function for AirNav Feeder
 */
void airnav_main() {

    createPidFile();

    // LED's nor available (this way) for RBCS/RBLC
#ifdef ENABLE_LED
    if (use_leds) {
        wiringPiSetupGpio();
        pinMode(RPI_LED_STATUS, OUTPUT);
        pinMode(RPI_LED_ADSB, OUTPUT);
        // Turn off ADSB LED and on Status
        digitalWrite(RPI_LED_STATUS, HIGH);
        digitalWrite(RPI_LED_ADSB, LOW);
    }
#endif


    for (int ab = 0; ab < MAX_ANRB; ab++) {
        anrbList[ab].active = 0;
        anrbList[ab].socket = malloc(sizeof (int));
        *anrbList[ab].socket = -1;
    }

    flist = NULL;

    if (cat21_loaded == 1) {
        airnav_log("ASTERIX CAT 021 Version Loaded: %s\n", cat21_version);
    }

#ifdef DEBUG_RELEASE
    airnav_log("****** Debug RELEASE ******\n");
#endif

    airnav_log("Start date/time: %s\n", start_datetime);
    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };

    sigaction(SIGCHLD, &sigchld_action, NULL);
    signal(SIGPIPE, net_sigpipe_handler);

    // Init all mutex
    airnav_init_mutex();


}

void airnav_init_mutex(void) {
    // Create mutex for counters
    if (pthread_mutex_init(&m_packets_counter, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Socket Mutex
     */
    if (pthread_mutex_init(&m_socket, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Copy Mutex
     */
    if (pthread_mutex_init(&m_copy, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }


    /*
     * Cmd Mutex
     */
    if (pthread_mutex_init(&m_cmd, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    
    
    /*
     * Led ADSB Mutex
     */
    //if (pthread_mutex_init(&m_led_adsb, NULL) != 0) {
    //    printf("\n mutex init failed\n");
    //    exit(EXIT_FAILURE);
    //}




    airnav_create_thread();

}

void airnav_create_thread(void) {

    ntpStatus_init();
    ntpStatus_startNtpStatusGetter();

    // Thread to wait commands
    pthread_create(&t_waitcmd, NULL, net_thread_WaitCmds, NULL);

    // is dump978 configured?
    if (dump978_enabled > 0) {
        airnav_log_level(1, "Creating dump978 thread...\n");
        pthread_create(&t_dump978, NULL, uat_airnav_ext978, NULL);
    }

    // Thread to monitor connection with AirNav server
    pthread_create(&t_monitor, NULL, airnav_monitorConnection, NULL);

    // Start thread that prepare data and send
    pthread_create(&t_prepareData, NULL, airnav_prepareData, NULL);

    // Thread to show statistics on screen
    pthread_create(&t_statistics, NULL, airnav_statistics, NULL);

    // Thread to send stats
    pthread_create(&t_stats, NULL, airnav_send_stats_thread, NULL);

    // Thread to send data
    pthread_create(&t_send_data, NULL, airnav_threadSendData, NULL);

    if (serial_device != NULL) {
        // Thread to get serial data
        pthread_create(&t_serial_data, NULL, serial_threadGetSerialData, NULL);
    }
    
    // Thread for ANRB
    pthread_create(&t_anrb, NULL, anrb_threadWaitNewANRB, NULL);
    pthread_create(&t_anrb_send, NULL, anrb_threadSendDataANRB, NULL);


    // Thread to updated ADS-B Led
    //pthread_create(&t_led_adsb, NULL, thread_LED_ADSB, NULL);


    if (autostart_acars) {
        if (!acars_checkACARSRunning()) {
            acars_startACARS();
        }
    }


    if (autostart_vhf) {
        if (!checkVhfRunning()) {
            startVhf();
        }
    }


    // Create asterix socket, if enabled
    if (asterix_enabled == 1) {
        airnav_log_level(1, "Asterix enabled. Creating socket\n");
        asterix_CreateUdpSocket();

    }


    p_mlat = 0;
    if (autostart_mlat) {
        if (!mlat_checkMLATRunning()) {
            mlat_startMLAT();
        }
    }


    p_978 = 0;
    if (autostart_978) {
        airnav_log_level(1, "Lets check 978 to autostart....\n");
        if (!uat_check978Running()) {
            airnav_log_level(1, "978 not running! Let's start it\n");
            uat_start978();
        }
    }



}

/*
 * Function to monitor connection with AirNav
 * Server.
 */
void *airnav_monitorConnection(void *arg) {
    MODES_NOTUSED(arg);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    int local_counter = 0;

    sleep(1);
    net_initial_com();


    while (!Modes.exit) {


        if (local_counter >= AIRNAV_MONITOR_SECONDS) {
            local_counter = 0;

            // Check if we need to create VHF configuration
            if (ini_getBoolean(configuration_file, "vhf", "force_create", 0) == 1) {
                airnav_log_level(1, "VHF Configuration file creating requested. doing.....");
                if (generateVHFConfig() == 1) {
                    airnav_log_level(1, "done!\n");

                    // Restart VHF daemon, if is running
                    if (checkVhfRunning() == 1) {
                        restartVhf();
                    }

                } else {
                    airnav_log_level(1, "error creating vhf configuration.\n");
                }
                ini_saveGeneric(configuration_file, "vhf", "force_create", "false");
            }

            // Check if VHF auto start is enabled and if VHF is running
            if (autostart_vhf) {
                if (!checkVhfRunning()) {
                    startVhf();
                }
            }

            // Check if we need to reload ASTERIX configuration
            if (ini_getBoolean(configuration_file, "asterix", "force_reload", 0) == 1) {
                airnav_log_level(1, "ASTERIX Reload requested. doing.....");
                loadAsterixConfiguration();
                ini_saveGeneric(configuration_file, "asterix", "force_reload", "false");
                airnav_log_level(1, "Reload done!\n");
            }


            if (airnav_com_inited == 0) {
                airnav_log_level(5, "[MONITOR1] Connection not initialized. Trying init protocol.\n");
                close(airnav_socket);
                airnav_socket = -1;
                net_initial_com();
            } else {
                airnav_log_level(5, "[MONITOR4] Connection OK. Sending Ping...\n");
                sendPing();
                //                sendMultipleFlights();
            }

        } else {
            local_counter++;
        }

        sleep(1);
    }

    airnav_log_level(1, "Exited thread_monitorConnection Successfull!\n");
    pthread_exit(EXIT_SUCCESS);

}

/*
 * Thread to show statistics on screen
 */
void *airnav_statistics(void *arg) {

    MODES_NOTUSED(arg);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    int local_counter = 0;

    while (!Modes.exit) {

        if (local_counter == AIRNV_STATISTICS_INTERVAL) {
            
            if (debug_level_cmd == -1) {
                debug_level = ini_getInteger(configuration_file, "client", "debug_level", 0);
            } else {
                debug_level = debug_level_cmd;
            }            
            
            local_counter = 0;
            airnav_log("******** Statistics updated every %d seconds ********\n", AIRNV_STATISTICS_INTERVAL);
            pthread_mutex_lock(&m_packets_counter);
            airnav_log("Packets sent in the last %d seconds: %ld, Total packets sent since startup: %d\n", AIRNV_STATISTICS_INTERVAL, packets_last, packets_total);
            packets_last = 0;
            double count = 0;

            count = global_data_sent;
            pthread_mutex_unlock(&m_packets_counter);

            const char* suffixes[7];
            suffixes[0] = "B";
            suffixes[1] = "KB";
            suffixes[2] = "MB";
            suffixes[3] = "GB";
            suffixes[4] = "TB";
            suffixes[5] = "PB";
            suffixes[6] = "EB";
            uint s = 0; // which suffix to use

            while (count >= 1024 && s < 7) {
                s++;
                count /= 1024;
            }

            if (count - floor(count) == 0.0) {
                airnav_log("Data sent: %d %s\n", (int) count, suffixes[s]);
            } else {
                airnav_log("Data sent: %.1f %s\n", count, suffixes[s]);
                airnav_log_level(1, "Data sent (bytes) %lu\n", (int) global_data_sent);
            }

            s = 0;
            count = data_received;
            while (count >= 1024 && s < 7) {
                s++;
                count /= 1024;
            }

            if (count - floor(count) == 0.0) {
                airnav_log("Data received: %d %s\n", (int) count, suffixes[s]);
            } else {
                airnav_log("Data received: %.1f %s\n", count, suffixes[s]);
            }

            //airnav_log("Currently tracked flights: %d\n", currently_tracked_flights);

        } else {
            local_counter++;
        }
        sleep(1);

    }

    airnav_log_level(1, "Exited airnav_statistics Successfull!\n");
    pthread_exit(EXIT_SUCCESS);

}

/*
 * Send statistics to FW
 */
void *airnav_send_stats_thread(void *argv) {

    MODES_NOTUSED(argv);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    static uint64_t next_update;
    static uint64_t next_mlat_check;
    uint64_t now = mstime();

    next_update = now + (AIRNAV_STATS_SEND_TIME * 1000);
    next_mlat_check = now + 30 * 1000;


    airnav_log_level(3, "Starting stats thread...\n");
    while (Modes.exit == 0) {

        now = mstime();
        if (now >= next_mlat_check) {
            next_mlat_check = now + 30 * 1000;
            if (autostart_mlat) {
                mlat_startMLAT();
            }
        }
        if (now >= next_update) {
            next_update = now + (AIRNAV_STATS_SEND_TIME * 1000);
            net_sendStats();
        }

        usleep(1000000);
    }


    airnav_log_level(1, "Exited send_stats Successfull!\n");
    pthread_exit(EXIT_SUCCESS);
}

/*
 * Thread to send data
 */
void *airnav_threadSendData(void *argv) {
    MODES_NOTUSED(argv);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);

    struct packet_list *local_list = NULL, *tmp_counter = NULL;
    unsigned qtd = 0;



    while (!Modes.exit) {

        pthread_mutex_lock(&m_copy);
        local_list = flist;
        tmp_counter = flist;
        flist = NULL;
        packet_cache_count = 0;
        pthread_mutex_unlock(&m_copy);


        // Count how many itens we have
        qtd = 0;
        while (tmp_counter != NULL) {
            qtd++;
            tmp_counter = tmp_counter->next;
        }

        // Create main packet structure
        if (local_list != NULL) {
            // Send packets
            sendMultipleFlights(local_list, qtd);
        }

        sleep(1);
    }

    airnav_log_level(1, "Exited sendData Successfull!\n");
    pthread_exit(EXIT_SUCCESS);
}

/*
 * Tis function get data from ModeS Decoder and prepare
 * to send to AirNAv
 */
void *airnav_prepareData(void *arg) {
    AN_NOTUSED(arg);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, net_sigpipe_handler);
    signal(SIGINT, rbfeederSigintHandler);
    signal(SIGTERM, rbfeederSigtermHandler);


    struct aircraft *b = Modes.aircrafts;
    uint64_t now = mstime();
    int send = 0;
    int force_send = 0;
    uint32_t extra = 0;
    struct packet_list *tmp, *tmp2;

    MODES_NOTUSED(arg);
    struct p_data *acf, *acf2;
    struct asterixPacketDef_cat21 *packet = NULL;
    struct timeval tv;
    //printf("Seconds since Jan. 1, 1970: %ld\n", tv.tv_sec);


    while (!Modes.exit) {



        //pthread_mutex_lock(&Modes.data_mutex);
        pthread_mutex_lock(&m_copy);
        pthread_mutex_lock(&m_copy2);
        packet_list_count = 0;
        currently_tracked_flights = 0;

        b = Modes.aircrafts;
        if (b) {

            b = Modes.aircrafts;

            while (b) {
                now = mstime();
                acf = net_preparePacket_v2();
                acf2 = net_preparePacket_v2(); // ANRB
                send = 0;
                gettimeofday(&tv, NULL);
                force_send = 0;


                // Asterix
                if (asterix_enabled == 1 && cat21_loaded == 1) {
                    packet = prepareAsterixPacket_cat21(MAX_CAT21_FSPEC_BYTES, MAX_CAT21_ITEMS);
                    // Not optional item
                    cat021_setDataSourceSACSIC(packet, cat21items, asterix_sac, asterix_sic);

                    if (asterix_sid_enabled == 1) {
                        cat021_setServiceId(packet, cat21items, asterix_sid);
                    }

                }



                if (b->addr & MODES_NON_ICAO_ADDRESS) {
                    airnav_log_level(5, "Invalid ICAO code.\n");
                } else {
                    acf->modes_addr = b->addr;
                    acf->modes_addr_set = 1;
                    // ANRB
                    acf2->modes_addr = b->addr;
                    acf2->modes_addr_set = 1;

                    // Asterix
                    unsigned char hexval[3];
                    hexval[0] = (b->addr >> 16);
                    hexval[1] = (b->addr >> 8);
                    hexval[2] = (b->addr & 0x0000ff);
                    cat021_setTargetAddress(packet, cat21items, hexval);
                    currently_tracked_flights++;

                    // Check conditions that force data to be sent

                    // Speed less than 50
                    if (trackDataValid(&b->gs_valid) && b->gs <= 50) {
                        force_send = 1;
                        airnav_log_level(3, "[%06X, Callsign '%s'] Speed <= 50, force send.\n", (b->addr & 0xffffff), b->callsign);
                    }

                    // Altitude < 3000
                    if (trackDataValid(&b->altitude_geom_valid) && b->altitude_geom <= 3000) {
                        force_send = 1;
                        airnav_log_level(3, "[%06X, Callsign '%s'] Altitude (geometric) <= 3000, force send.\n", (b->addr & 0xffffff), b->callsign);
                    } else if (trackDataValid(&b->altitude_baro_valid) && b->altitude_baro <= 3000) {
                        force_send = 1;
                        airnav_log_level(3, "[%06X, Callsign '%s'] Altitude (barometric) < 3000, force send.\n", (b->addr & 0xffffff), b->callsign);
                    }

                    // Airborne = Ground
                    if (trackDataValid(&b->airground_valid) && b->airground == AG_GROUND && b->airground_valid.source >= SOURCE_MODE_S_CHECKED) {
                        force_send = 1;
                        airnav_log_level(3, "[%06X, Callsign '%s'] Airborne = GROUND, force send. Altitude (baro): %d, Altitude (geom): %d\n", (b->addr & 0xffffff), b->callsign, b->altitude_baro, b->altitude_geom);
                    }


                    // (vertical_rate > 1000 && altitude < 10000)
                    if (trackDataValid(&b->geom_rate_valid) && b->geom_rate >= 1000) {
                        // Now, check altitude
                        if (trackDataValid(&b->altitude_geom_valid) && b->altitude_geom <= 10000) {
                            force_send = 1;
                            airnav_log_level(3, "[%06X, Callsign '%s'] Geometric rate > 1000 and altitude (geom) < 7000, force send.\n", (b->addr & 0xffffff), b->callsign);
                        } else if (trackDataValid(&b->altitude_baro_valid) && b->altitude_baro <= 10000) {
                            force_send = 1;
                            airnav_log_level(3, "[%06X, Callsign '%s'] Geometric rate > 1000 and altitude (baro) < 7000, force send.\n", (b->addr & 0xffffff), b->callsign);
                        }
                    } else if (trackDataValid(&b->baro_rate_valid) && b->baro_rate >= 1000) {
                        // Now, check altitude
                        if (trackDataValid(&b->altitude_geom_valid) && b->altitude_geom <= 10000) {
                            force_send = 1;
                            airnav_log_level(3, "[%06X, Callsign '%s'] Baro rate > 1000 and altitude (geom) < 7000, force send.\n", (b->addr & 0xffffff), b->callsign);
                        } else if (trackDataValid(&b->altitude_baro_valid) && b->altitude_baro <= 10000) {
                            force_send = 1;
                            airnav_log_level(3, "[%06X, Callsign '%s'] Baro rate > 1000 and altitude (baro) < 7000, force send.\n", (b->addr & 0xffffff), b->callsign);
                        }
                    }




                }

                acf->timestp = now;
                // ANRB
                acf2->timestp = now;

                // Check if Callsign updated
                if (trackDataAge(&b->callsign_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_callsign_time) >= MAX_TIME_FIELD_CALLSIGN) || (strcmp(b->callsign, b->an.rpisrv_emitted_callsign) != 0) || force_send == 1) { // Send only once every 60 seconds (or when data changed)
                        strcpy(b->an.rpisrv_emitted_callsign, b->callsign);
                        b->an.rpisrv_emitted_callsign_time = tv.tv_sec;


                        strcpy(acf->callsign, b->callsign);
                        acf->callsign_set = 1;
                        // ANRB
                        strcpy(acf2->callsign, b->callsign);
                        acf2->callsign_set = 1;
                        //send = 0;
                        if (asterix_enabled == 1 && cat21_loaded == 1) {
                            cat021_setTargetId(packet, cat21items, b->callsign);
                        }

                    } else {
                        airnav_log_level(3, "[%06X] Callsign is the same for less than %d seconds, will NOT send anything (%d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_CALLSIGN, b->callsign);
                    }


                }

                airnav_log_level(3, "[%06X] Callsign: '%s'.\n", (b->addr & 0xffffff), b->callsign);


                // Check if Alt updated
                if (trackDataValid(&b->airground_valid) && b->airground == AG_GROUND && b->airground_valid.source >= SOURCE_MODE_S_CHECKED) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_airborne_time) >= MAX_TIME_FIELD_AIRBORNE) || (b->an.rpisrv_emitted_airborne != 0) || force_send == 1) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_airborne_time = tv.tv_sec;
                        b->an.rpisrv_emitted_airborne = 0;
                        acf->airborne = 0;
                        acf->airborne_set = 1;
                        // ANRB
                        acf2->airborne = 0;
                        acf2->airborne_set = 1;
                        send = 1;
                        airnav_log_level(3, "[%06X] ******* Sending Airborne .\n", (b->addr & 0xffffff));
                    } else {
                        airnav_log_level(3, "[%06X] Airborne is the same for less than %d seconds, will NOT send anything (%d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_AIRBORNE, b->an.rpisrv_emitted_airborne);
                    }


                } else {


                    if (Modes.use_gnss && trackDataValid(&b->altitude_geom_valid)) {
                        if (trackDataAge(&b->altitude_geom_valid) <= AIRNAV_MAX_ITEM_AGE) {

                            if (((tv.tv_sec - b->an.rpisrv_emitted_altitude_geom_time) >= MAX_TIME_FIELD_ALTITUDE) || (b->an.rpisrv_emitted_altitude_geom != b->altitude_geom) || force_send == 1) { // Send only once every 60 seconds (or when data changed)
                                b->an.rpisrv_emitted_altitude_geom_time = tv.tv_sec;
                                b->an.rpisrv_emitted_altitude_geom = b->altitude_geom;

                                if (b->altitude_geom > 0) {
                                    if (((tv.tv_sec - b->an.rpisrv_emitted_airborne_time) >= MAX_TIME_FIELD_AIRBORNE) || (b->an.rpisrv_emitted_airborne != 1) || force_send == 1) { // Send only once every X seconds (or when data changed)
                                        b->an.rpisrv_emitted_airborne = 1;
                                        b->an.rpisrv_emitted_airborne_time = tv.tv_sec;

                                        acf->airborne = 1;
                                        acf->airborne_set = 1;
                                        // ANRB
                                        acf2->airborne = 1;
                                        acf2->airborne_set = 1;
                                    }
                                }
                                acf->altitude_geo = b->altitude_geom;
                                acf->altitude_geo_set = 1;
                                acf->geom_altitude_timestamp_us = b->altitude_geom_valid.updatedUs;
                                // ANRB
                                acf2->altitude_geo = b->altitude_geom;
                                acf2->altitude_geo_set = 1;
                                send = 1;
                                airnav_log_level(3, "[%06X] Sending altitude_geom...%d\n", (b->addr & 0xffffff), b->altitude_geom);
                            } else {
                                airnav_log_level(3, "[%06X] Altitude (geom) is the same for less than %d seconds, will NOT send anything (%d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_AIRBORNE, b->an.rpisrv_emitted_altitude_geom);
                            }


                            // Asterix
                            if (asterix_enabled == 1 && cat21_loaded == 1) {
                                cat021_setGeometricHeight(packet, cat21items, (b->altitude_geom / 100));
                            }




                        }
                    }


                    // Altitude barometric
                    if (trackDataValid(&b->altitude_baro_valid)) {

                        if (trackDataAge(&b->altitude_baro_valid) <= AIRNAV_MAX_ITEM_AGE) {


                            if (((tv.tv_sec - b->an.rpisrv_emitted_altitude_baro_time) >= MAX_TIME_FIELD_ALTITUDE) || (b->an.rpisrv_emitted_altitude_baro != b->altitude_baro) || force_send == 1) { // Send only once every 60 seconds (or when data changed)

                                // Update send time and value
                                b->an.rpisrv_emitted_altitude_baro_time = tv.tv_sec;
                                b->an.rpisrv_emitted_altitude_baro = b->altitude_baro;

                                if (b->altitude_baro > 0) {
                                    if (((tv.tv_sec - b->an.rpisrv_emitted_airborne_time) >= MAX_TIME_FIELD_AIRBORNE) || (b->an.rpisrv_emitted_airborne != 1) || force_send == 1) { // Send only once every X seconds (or when data changed)
                                        b->an.rpisrv_emitted_airborne = 1;
                                        b->an.rpisrv_emitted_airborne_time = tv.tv_sec;
                                        acf->airborne = 1;
                                        acf->airborne_set = 1;
                                        // ANRB
                                        acf2->airborne = 1;
                                        acf2->airborne_set = 1;
                                    }
                                }
                                acf->altitude = b->altitude_baro;
                                acf->altitude_set = 1;
                                acf->baro_altitude_timestamp_us = b->altitude_baro_valid.updatedUs;
                                // ANRB
                                acf2->altitude = b->altitude_baro;
                                acf2->altitude_set = 1;
                                send = 1;


                                airnav_log_level(3, "[%06X] Sending altitude_baro...%d\n", (b->addr & 0xffffff), b->altitude_baro);

                            } else {
                                airnav_log_level(3, "[%06X] Altitude (baro) is the same for less than %d seconds, will NOT send anything (%d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_ALTITUDE, b->altitude_baro);
                            }

                            // Asterix
                            if (asterix_enabled == 1 && cat21_loaded == 1) {
                                cat021_setFlightLevel(packet, cat21items, (b->altitude_baro / 100));
                            }

                        }
                    }


                }
                                
                // Position
                if (trackDataAge(&b->position_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (trackDataValid(&b->position_valid)) {
                        acf->lat = b->lat;
                        acf->lon = b->lon;
                        acf->position_set = 1;
                        acf->lat_lon_timestamp_us = b->position_valid.updatedUs;

                        if (((tv.tv_sec - b->an.rpisrv_emitted_pos_nic_time) >= MAX_TIME_FIELD_POS_NIC) || (b->an.rpisrv_emitted_pos_nic != b->pos_nic) || force_send == 1) { // Send only once every X seconds (or when data changed)
                            b->an.rpisrv_emitted_pos_nic_time = tv.tv_sec;
                            b->an.rpisrv_emitted_pos_nic = b->pos_nic;
                            acf->pos_nic = b->pos_nic;
                            acf->pos_nic_set = 1;
                        }

                        // ANRB
                        acf2->lat = b->lat;
                        acf2->lon = b->lon;
                        acf2->position_set = 1;
                        send = 1;
                        // Asterix
                        if (asterix_enabled == 1 && cat21_loaded == 1) {
                            cat021_setPosWGS84Precision(packet, cat21items, b->lat, b->lon);
                        }

                    }
                }

                // Heading
                if (trackDataAge(&b->mag_heading_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mag_heading_time) >= MAX_TIME_FIELD_MAG_HEADING) || (b->an.rpisrv_emitted_mag_heading != (b->mag_heading / 10))) { // Send only once every 60 seconds (or when data changed)

                        // Update values
                        b->an.rpisrv_emitted_mag_heading_time = tv.tv_sec;
                        b->an.rpisrv_emitted_mag_heading = (b->mag_heading / 10);

                        if (trackDataValid(&b->mag_heading_valid)) {                                                       
                            //acf->heading = (b->mag_heading / 10);                                                        
                            //acf->heading_set = 1;
                                                        
                            acf->heading_full = b->mag_heading;
                            acf->heading_full_set = 1;                                                        
                            
                            // ANRB
                            acf2->heading = (b->mag_heading / 10);
                            acf2->heading_set = 1;
                            send = 1;

                            airnav_log_level(3, "[%06X] Sending heading (mag)...%d\n", (b->addr & 0xffffff), (b->mag_heading / 10));

                        }

                    } else {
                        airnav_log_level(3, "[%06X] Heading (mag) is the same for less than %d seconds, will NOT send anything.\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MAG_HEADING);
                    }

                    // Asterix
                    if (asterix_enabled == 1 && cat21_loaded == 1) {
                        cat021_setMagneticHeading(packet, cat21items, b->mag_heading);
                    }

                }


                // True air speed
                if (trackDataAge(&b->tas_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (trackDataValid(&b->tas_valid)) {
                        // Asterix
                        if (asterix_enabled == 1 && cat21_loaded == 1) {
                            cat021_setTrueAirSpeed(packet, cat21items, 0, (short) b->tas);
                        }
                    }
                }


                // Mach speed
                if (trackDataAge(&b->mach_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (trackDataValid(&b->mach_valid)) {
                        //airnav_log("[%06X] MACH speed: %.2f\n", (b->addr & 0xffffff), b->mach);
                    }
                }

                // Weather information
                if (trackDataAge(&b->temperature_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (trackDataValid(&b->temperature_valid)) {
                        airnav_log_level(1,"[%06X] Temperature (MRAR): %.2f\n", (b->addr & 0xffffff), b->temperature);
                    }
                }
                if (trackDataAge(&b->wind_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (trackDataValid(&b->wind_valid)) {
                        airnav_log_level(1,"[%06X] Wind Dir and Speed (MRAR): '%.2f' - '%.2f'\n", (b->addr & 0xffffff), b->wind_dir, b->wind_speed);
                    }
                }
                
                
                
                // Calculate air temperature and wind speed/direction
                if (trackDataValid(&b->mach_valid) && trackDataValid(&b->ias_valid) && trackDataValid(&b->altitude_baro_valid) && trackDataValid(&b->tas_valid)) {

                    float alt = b->altitude_baro; // altitude
                    float vtas = b->tas; // TAS
                    float vias = b->ias; // IAS
                    float mach = b->mach; // MACH
                    float temp = 0;
                    float p = 0;
                    float rho0 = 1.225; // kg/m3, air density, sea level ISA
                    float R = 287.05287; // m2/(s2 x K), gas constant, sea level ISA
                    //float rho = 0;
                    float T0 = 288.15; // K, temperature, sea level ISA
                    float a0 = 340.293988; // m/s, sea level speed of sound ISA, sqrt(gamma*R*T0)
                    //float vtas2 = 0;

                    // Convert to IS units
                    float altMeters = FEET_TO_M(alt);
                    float vtasMs = KNOT_TO_MS(vtas);
                    float viasMs = KNOT_TO_MS(vias);

                    if (altMeters < STRATOSPHERE_BASE_HEIGHT) {
                        p = pow((101325 * (1 + (-0.0065 * altMeters) / 288.15)), (-9.81 / (-0.0065 * 287.05)));
                    } else {
                        p = 22632 * exp(-(9.81 * (altMeters - STRATOSPHERE_BASE_HEIGHT) / (287.05 * 216.65)));
                    }

                    if (mach < 0.3) {
                        temp = pow(vtasMs, 2) * p / (pow(viasMs, 2) * rho0 * R);
                        //rho = p / (R * temp);
                        //vtas2 = viasMs * sqrt(rho0 / rho);
                    } else {
                        temp = pow(vtasMs, 2) * T0 / (pow(mach, 2) * pow(a0, 2));
                        //vtas2 = mach * a0 * sqrt(temp / T0);
                    }

                    float tempC = KELVIN_TO_C(temp);


                    // Calculate wind speed/direction
                    if ((acf->altitude_set == 1) && (acf->position_set == 1) && (acf->heading_set == 1)) {

                        if ((tempC > -90) && (tempC < 100)) { // xTreme range
                            
                            // If we have altitude and location set, we can check if we need to send temperature or not
                            if (((tv.tv_sec - b->an.rpisrv_emitted_temperature_time) >= MAX_TIME_FIELD_TEMPERATURE) || (b->an.rpisrv_emitted_temperature != (short) tempC)) { //                             
                                b->an.rpisrv_emitted_temperature = (short) tempC;
                                b->an.rpisrv_emitted_temperature_time = tv.tv_sec;

                                acf->temperature = (short) tempC;
                                acf->temperature_set = 1;
                                send = 1;
                                airnav_log_level(3, "[%06X] Sending Weather: Air temp: %.3f K (%.3f C); \n", (b->addr & 0xffffff), temp, tempC);
                            } else {
                                airnav_log_level(3, "[%06X] Weather temperature is the same for less than %d seconds, will NOT send anything.\n", (b->addr & 0xffffff), MAX_TIME_FIELD_TEMPERATURE);
                            }
                            
                        }


                        double magAlt = altMeters / 1000.0;
                        double magLat = acf->lat;
                        double magLon = acf->lon;

                        double declination; // Magnetic declination
                        double dip; //
                        double ti; // Total intensity
                        double gv; // Grid variation

                        time_t t = time(NULL);
                        struct tm tm = *localtime(&t);
                        sprintf(start_datetime, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

                        double decimalYear = ((double) (tm.tm_year + 1900.0)) + (((double) tm.tm_yday) / 365.0) + (((double) tm.tm_hour) / (24.0 * 365.0));
                        geomag_geomg1(magAlt, magLat, magLon, decimalYear, &declination, &dip, &ti, &gv);

                        double realHeading = b->mag_heading + declination;
                        double trackHeading = b->track;

                        if (realHeading < 0.0) {
                            realHeading = 360.0 + realHeading;
                        } else if (realHeading > 360.0) {
                            realHeading = realHeading - 360.0;
                        }

                        double realHeadingRadians = TO_RADIANS(realHeading);
                        double trackHeadingRadians = TO_RADIANS(trackHeading);

                        double groundSpeedMs = KNOT_TO_MS(b->gs);

                        double gsX = sin(trackHeadingRadians) * (groundSpeedMs);
                        double gsY = cos(trackHeadingRadians) * (groundSpeedMs);

                        double vtasX = sin(realHeadingRadians) * vtasMs;
                        double vtasY = cos(realHeadingRadians) * vtasMs;

                        double windHeadingRadians = atan((gsX - vtasX) / (gsY - vtasY));
                        double windSpeed = (gsX - vtasX) / sin(windHeadingRadians);

                        double windX = sin(windHeadingRadians) * windSpeed;
                        double windY = cos(windHeadingRadians) * windSpeed;

                        double windHeading = TO_DEGREES(windHeadingRadians);
                        if (windSpeed < 0.0) {
                            windSpeed = -windSpeed;
                            windHeading = windHeading + 180.0;
                            if (windHeading > 360.0) {
                                windHeading = 360.0 - windHeading;
                            }
                        }

                        short tmp_wind_dir = (short) (windHeading / 10.0);
                        short tmp_wind_speed = (short) MS_TO_KNOT(windSpeed);

                        if (((tv.tv_sec - b->an.rpisrv_emitted_wind_time) >= MAX_TIME_FIELD_WIND) || ((b->an.rpisrv_emitted_wind_dir != tmp_wind_dir) || (b->an.rpisrv_emitted_wind_speed != tmp_wind_speed))) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_wind_dir = tmp_wind_dir;
                            b->an.rpisrv_emitted_wind_speed = tmp_wind_speed;
                            b->an.rpisrv_emitted_wind_time = tv.tv_sec;


                            acf->wind_dir = tmp_wind_dir;
                            acf->wind_dir_set = 1;
                            acf->wind_speed = tmp_wind_speed;
                            acf->wind_speed_set = 1;
                            send = 1;
                            airnav_log_level(3, "[%06X] Sending Weather: wind speed: %.3f m/s; wind angle: %.3f degrees. Components: %.3f,%.3f\n", (b->addr & 0xffffff), windSpeed, windHeading, windX, windY);
                        } else {
                            airnav_log_level(3, "[%06X] Weather is the same for less than %d seconds, will NOT send anything.\n", (b->addr & 0xffffff), MAX_TIME_FIELD_WIND);
                        }
                        

                    } else {
                        airnav_log_level(3, "[%06X] missing parameters for wind calculation: altitude_set: %d; altitude_set: %d; position_set: %d; heading_set: %d;\n", acf->altitude_set, acf->position_set, acf->heading_set);
                    }

                } else {
                    airnav_log_level(3, "[%06X] missing parameters for temperature calculation: mach_valid: %d; ias_valid: %d; altitude_baro_valid: %d; tas_valid: %d\n", trackDataValid(&b->mach_valid), trackDataValid(&b->ias_valid), trackDataValid(&b->altitude_baro_valid), trackDataValid(&b->tas_valid));
                }

                // Check if Speed updated

                if (trackDataAge(&b->gs_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_gs_time) >= MAX_TIME_FIELD_GS) || (b->an.rpisrv_emitted_gs != (b->gs / 10)) || force_send == 1) { // Send only once every 60 seconds (or when data changed)

                        // Update values
                        b->an.rpisrv_emitted_gs_time = tv.tv_sec;
                        b->an.rpisrv_emitted_gs = (b->gs / 10);
                        

                        if (trackDataValid(&b->gs_valid)) {
                            //acf->gnd_speed = (b->gs / 10);
                            acf->gnd_speed_full = b->gs;
                            //acf->gnd_speed_set = 1;
                            acf->gnd_speed_full_set = 1;                                                        
                            
                            // ANRB
                            acf2->gnd_speed = (b->gs / 10);
                            acf2->gnd_speed_set = 1;
                            send = 1;
                            airnav_log_level(3, "[%06X] Sending ground speed...%.0f\n", (b->addr & 0xffffff), (b->gs / 10));
                        }

                    } else {
                        airnav_log_level(3, "[%06X] Gnd_speed is the same for less than %d seconds, will NOT send anything (b->gs: %.0f, emitted_gs: %.0f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_GS, (b->gs / 10), b->an.rpisrv_emitted_gs);
                    }


                }

                // Check vertical rate
                if (Modes.use_gnss && trackDataValid(&b->geom_rate_valid)) {
                    if (trackDataAge(&b->geom_rate_valid) <= AIRNAV_MAX_ITEM_AGE) {

                        if (((tv.tv_sec - b->an.rpisrv_emitted_geom_rate_time) >= MAX_TIME_FIELD_GEOM_RATE) || (b->an.rpisrv_emitted_geom_rate != (b->geom_rate / 10)) || force_send == 1) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_geom_rate = (b->geom_rate / 10);
                            b->an.rpisrv_emitted_geom_rate_time = tv.tv_sec;

                            //acf->vert_rate = (b->geom_rate / 10);
                            //acf->vert_rate_set = 1;
                            
                            acf->vert_rate_full = b->geom_rate;
                            acf->vert_rate_full_set = 1;
                            
                            // ANRB
                            acf2->vert_rate = (b->geom_rate / 10);
                            acf2->vert_rate_set = 1;
                            send = 1;
                            airnav_log_level(3, "[%06X] Sending vertical rate geom...%d\n", (b->addr & 0xffffff), (b->geom_rate / 10));
                        } else {
                            airnav_log_level(3, "[%06X] geom_rate is the same for less than %d seconds, will NOT send anything (b->geom_rate: %d, emitted_geom_rate: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_GEOM_RATE, (b->geom_rate / 10), b->an.rpisrv_emitted_geom_rate);
                        }

                    }
                } else if (trackDataValid(&b->baro_rate_valid)) {
                    if (trackDataAge(&b->baro_rate_valid) <= AIRNAV_MAX_ITEM_AGE) {

                        if (((tv.tv_sec - b->an.rpisrv_emitted_baro_rate_time) >= MAX_TIME_FIELD_BARO_RATE) || (b->an.rpisrv_emitted_baro_rate != (b->baro_rate / 10)) || force_send == 1) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_baro_rate = (b->baro_rate / 10);
                            b->an.rpisrv_emitted_baro_rate_time = tv.tv_sec;

                            //acf->vert_rate = (b->baro_rate / 10);
                            //acf->vert_rate_set = 1;
                            
                            acf->vert_rate_full = b->baro_rate;
                            acf->vert_rate_full_set = 1;
                            
                            // ANRB
                            acf2->vert_rate = (b->baro_rate / 10);
                            acf2->vert_rate_set = 1;
                            send = 1;
                            airnav_log_level(3, "[%06X] Sending vertical rate baro...%d\n", (b->addr & 0xffffff), (b->baro_rate / 10));
                        } else {
                            airnav_log_level(3, "[%06X] baro_rate is the same for less than %d seconds, will NOT send anything (b->baro_rate: %d, emitted_baro_rate: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_BARO_RATE, (b->baro_rate / 10), b->an.rpisrv_emitted_baro_rate);
                        }

                    }
                }

                // Check if Squawk updated
                if (trackDataAge(&b->squawk_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (trackDataValid(&b->squawk_valid)) {

                        if (((tv.tv_sec - b->an.rpisrv_emitted_squawk) >= MAX_TIME_FIELD_SQUAWKE) || (b->an.rpisrv_emitted_squawk != b->squawk)) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_squawk = b->squawk;
                            b->an.rpisrv_emitted_squawk_time = tv.tv_sec;

                            acf->squawk = b->squawk;
                            acf->squawk_set = 1;
                            // ANRB
                            acf2->squawk = b->squawk;
                            acf2->squawk_set = 1;
                            send = 1;
                            airnav_log_level(3, "[%06X] Sending sqawk...%u\n", (b->addr & 0xffffff), b->squawk);
                        } else {
                            airnav_log_level(3, "[%06X] squawk is the same for less than %d seconds, will NOT send anything (b->squawk: %u, emitted_squawk: %u).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_SQUAWKE, b->squawk, b->an.rpisrv_emitted_squawk);
                        }


                    }
                }



                // Check if IAS updated
                if (trackDataAge(&b->ias_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_ias_time) >= MAX_TIME_FIELD_IAS) || (b->an.rpisrv_emitted_ias != (b->ias / 10))) { // Send only once every 60 seconds (or when data changed)
                        b->an.rpisrv_emitted_ias = (b->ias / 10);
                        b->an.rpisrv_emitted_ias_time = tv.tv_sec;

                        //acf->ias = (b->ias / 10);
                        //acf->ias_set = 1;
                        
                        acf->ias_full = b->ias;
                        acf->ias_full_set = 1;
                        
                        // ANRB
                        acf2->ias = (b->ias / 10);
                        acf2->ias_set = 1;
                        airnav_log_level(3, "[%06X] Sending IAS...%u\n", (b->addr & 0xffffff), b->ias);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] ias is the same for less than %d seconds, will NOT send anything (b->ias: %u, emitted_ias: %u).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_IAS, (b->ias / 10), b->an.rpisrv_emitted_ias);
                    }

                }

                // MLAT Flag check
                if (mlat_check_is_mlat(b) == 1) {
                    acf->is_mlat = 1;
                }


                // Navigation options

                extra = 0;
                if (trackDataAge(&b->nav_modes_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_nav_modes_time) >= MAX_TIME_FIELD_NAV_MODES) || (b->an.rpisrv_emitted_nav_modes != b->nav_modes)) { // Send only once every 60 seconds (or when data changed)

                        b->an.rpisrv_emitted_nav_modes = b->nav_modes;
                        b->an.rpisrv_emitted_nav_modes_time = tv.tv_sec;

                        if (b->nav_modes & NAV_MODE_AUTOPILOT) {
                            airnav_log_level(3, "HEX: %06X, AutoPilot: ON\n", b->addr);
                            acf->nav_modes_autopilot_set = 1;
                        }
                        if (b->nav_modes & NAV_MODE_VNAV) {
                            airnav_log_level(3, "HEX: %06X, VNAV: ON\n", b->addr);
                            acf->nav_modes_vnav_set = 1;
                        }
                        if (b->nav_modes & NAV_MODE_ALT_HOLD) {
                            airnav_log_level(3, "HEX: %06X, AltitudeHold: ON\n", b->addr);
                            acf->nav_modes_alt_hold_set = 1;
                        }
                        if (b->nav_modes & NAV_MODE_APPROACH) {
                            airnav_log_level(3, "HEX: %06X, Aproach: ON\n", b->addr);
                            acf->nav_modes_aproach_set = 1;
                        }
                        if (b->nav_modes & NAV_MODE_LNAV) {
                            airnav_log_level(3, "HEX: %06X, LNAV: ON\n", b->addr);
                            acf->nav_modes_lnav_set = 1;
                        }
                        if (b->nav_modes & NAV_MODE_TCAS) {
                            airnav_log_level(3, "HEX: %06X, TCAS: ON\n", b->addr);
                            acf->nav_modes_tcas_set = 1;
                        }

                        airnav_log_level(3, "[%06X] Sending navigation modes\n", (b->addr & 0xffffff));

                    } else {
                        airnav_log_level(3, "[%06X] nav_modes is the same for less than %d seconds, will NOT send anything (nav_modes).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_IAS, b->nav_modes);
                    }




                }

                if (trackDataAge(&b->nav_altitude_fms_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (b->nav_altitude_fms >= 1000) {

                        if (((tv.tv_sec - b->an.rpisrv_emitted_nav_altitude_fms_time) >= MAX_TIME_FIELD_NAV_ALT_FMS) || (b->an.rpisrv_emitted_nav_altitude_fms != b->nav_altitude_fms)) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_nav_altitude_fms = b->nav_altitude_fms;
                            b->an.rpisrv_emitted_nav_altitude_fms_time = tv.tv_sec;
                            acf->nav_altitude_fms_set = 1;
                            acf->nav_altitude_fms = (int) b->nav_altitude_fms;
                            airnav_log_level(3, "[%06X] Sending nav_altitude_fms: %u!\n", b->addr, b->nav_altitude_fms);
                        } else {
                            airnav_log_level(3, "[%06X] nav_altitude_fms is the same for less than %d seconds, will NOT send anything.\n", (b->addr & 0xffffff), MAX_TIME_FIELD_NAV_ALT_FMS);
                        }


                    }

                }
                if (trackDataAge(&b->nav_altitude_mcp_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if (b->nav_altitude_mcp >= 1000) {

                        if (((tv.tv_sec - b->an.rpisrv_emitted_nav_altitude_mcp_time) >= MAX_TIME_FIELD_NAV_ALT_MCP) || (b->an.rpisrv_emitted_nav_altitude_mcp != b->nav_altitude_mcp)) { // Send only once every 60 seconds (or when data changed)
                            b->an.rpisrv_emitted_nav_altitude_mcp = b->nav_altitude_mcp;
                            b->an.rpisrv_emitted_nav_altitude_mcp_time = tv.tv_sec;

                            acf->nav_altitude_mcp_set = 1;
                            acf->nav_altitude_mcp = b->nav_altitude_mcp;
                            airnav_log_level(3, "[%06X] Sending nav_altitude_mcp: %u!\n", b->addr, b->nav_altitude_mcp);
                        } else {
                            airnav_log_level(3, "[%06X] nav_altitude_mcp is the same for less than %d seconds, will NOT send anything.\n", (b->addr & 0xffffff), MAX_TIME_FIELD_NAV_ALT_MCP);
                        }

                    }
                }

                if (trackDataAge(&b->nav_altitude_src_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    // Ignore unknow and invalid sources
                    if (b->nav_altitude_src > 1 && b->nav_altitude_src < 5) {
                        acf->nav_altitude_src = b->nav_altitude_src;
                        if (b->nav_altitude_src == 2) {
                            set_bit(&extra, 7);
                            acf->extra_flags_set = 1;
                            acf->extra_flags = extra;
                        } else if (b->nav_altitude_src == 3) {
                            set_bit(&extra, 8);
                            acf->extra_flags_set = 1;
                            acf->extra_flags = extra;
                        } else if (b->nav_altitude_src == 4) {
                            set_bit(&extra, 9);
                            acf->extra_flags_set = 1;
                            acf->extra_flags = extra;
                        }
                        //                        airnav_log_level(4, "HEX: %06X, NAV_ALTITUDE_SOURCE Valid received! => %s\n", b->addr, nav_altitude_source_enum_string2(b->nav_altitude_src));
                    }
                }

                // NAV Altitude is set?
                if (trackDataAge(&b->nav_qnh_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_nav_qnh_time) >= MAX_TIME_FIELD_NAV_QNH) || (b->an.rpisrv_emitted_nav_qnh != b->nav_qnh)) { // Send only once every 60 seconds (or when data changed)

                        b->an.rpisrv_emitted_nav_qnh = b->nav_qnh;
                        b->an.rpisrv_emitted_nav_qnh_time = tv.tv_sec;

                        acf->nav_qnh_set = 1;
                        acf->nav_qnh = (int) b->nav_qnh;
                        airnav_log_level(3, "[%06X] Sending NAV QNH Set: %.2f (%d)\n", (b->addr & 0xffffff), b->nav_qnh, (int) b->nav_qnh);
                    } else {
                        airnav_log_level(3, "[%06X] nav_qnh is the same for less than %d seconds, will NOT send anything (%d).\n", (b->addr & 0xffffff), b->nav_qnh);
                    }

                }

                // NAV Heading is set?
                if (trackDataAge(&b->nav_heading_valid) <= AIRNAV_MAX_ITEM_AGE) {
                    if ((int) b->nav_heading != 0) {
                        acf->nav_heading_set = 1;
                        acf->nav_heading = (int) b->nav_heading;
                        airnav_log_level(3, "HEX: %06X, NAV Heading Set: %.2f (%d)\n", b->addr, b->nav_heading, (int) b->nav_heading);
                    }
                }


                // NIC Baro
                if (trackDataValid(&b->nic_baro_valid)) {
                    if (((tv.tv_sec - b->an.rpisrv_emitted_nic_baro_time) >= MAX_TIME_FIELD_NIC_BARO) || (b->an.rpisrv_emitted_nic_baro != b->nic_baro) || force_send == 1) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_nic_baro_time = tv.tv_sec;
                        b->an.rpisrv_emitted_nic_baro = b->nic_baro;
                        acf->nic_baro = b->nic_baro;
                        acf->nic_baro_set = 1;
                        send = 1;
                    }
                    //airnav_log("[%06X] nic_baro: %u\n", (b->addr & 0xffffff), b->nic_baro);
                }

                // NACp
                if (trackDataValid(&b->nac_p_valid)) {
                    if (((tv.tv_sec - b->an.rpisrv_emitted_nac_p_time) >= MAX_TIME_FIELD_NAC_P) || (b->an.rpisrv_emitted_nac_p != b->nac_p) || force_send == 1) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_nac_p_time = tv.tv_sec;
                        b->an.rpisrv_emitted_nac_p = b->nac_p;
                        acf->nac_p = b->nac_p;
                        acf->nac_p_set = 1;
                        send = 1;
                    }
                    //airnav_log("[%06X] nic_a: %u\n", (b->addr & 0xffffff), b->nic_a);
                }

                // NACv
                if (trackDataValid(&b->nac_v_valid)) {
                    if (((tv.tv_sec - b->an.rpisrv_emitted_nac_v_time) >= MAX_TIME_FIELD_NAC_V) || (b->an.rpisrv_emitted_nac_v != b->nac_v) || force_send == 1) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_nac_v_time = tv.tv_sec;
                        b->an.rpisrv_emitted_nac_v = b->nac_v;
                        acf->nac_v = b->nac_v;
                        acf->nac_v_set = 1;
                        send = 1;
                    }
                    //airnav_log("[%06X] nac_v: %u\n", (b->addr & 0xffffff), b->nac_v);
                }



                // SIL
                if (trackDataValid(&b->sil_valid)) {
                    if (((tv.tv_sec - b->an.rpisrv_emitted_sil_time) >= MAX_TIME_FIELD_SIL) || (b->an.rpisrv_emitted_sil != b->sil) || force_send == 1) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_sil_time = tv.tv_sec;
                        b->an.rpisrv_emitted_sil = b->sil;
                        acf->sil = b->sil;
                        acf->sil_set = 1;
                        acf->sil_type = b->sil_type;
                        acf->sil_type_set = 1;
                        send = 1;
                    }
                    //airnav_log("[%06X] sil: %u\n", (b->addr & 0xffffff), b->sil);
                }
                
                // TAS
                if (trackDataAge(&b->tas_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_tas_time) >= MAX_TIME_FIELD_TAS) || (b->an.rpisrv_emitted_tas != b->tas)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_tas = b->tas;
                        b->an.rpisrv_emitted_tas_time = tv.tv_sec;

                        acf->tas = b->tas;
                        acf->tas_set = 1;
                                                
                        airnav_log_level(3, "[%06X] Sending TAS...%u\n", (b->addr & 0xffffff), b->tas);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] TAS is the same for less than %d seconds, will NOT send anything (b->tas: %u, emitted_tas: %u).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_TAS, b->tas, b->an.rpisrv_emitted_tas);
                    }

                }
                
                // Track
                if (trackDataAge(&b->track_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_track_time) >= MAX_TIME_FIELD_TRACK) || (b->an.rpisrv_emitted_track != b->track)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_track = b->track;
                        b->an.rpisrv_emitted_tas_time = tv.tv_sec;

                        acf->track = b->track;
                        acf->track_set = 1;
                                                
                        airnav_log_level(3, "[%06X] Sending TRACK...%.2f\n", (b->addr & 0xffffff), b->track);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] TRACK is the same for less than %d seconds, will NOT send anything (b->track: %.2f, emitted_track: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_TRACK, b->track, b->an.rpisrv_emitted_track);
                    }

                }
                
                // True heading
                if (trackDataAge(&b->true_heading_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_true_heading_time) >= MAX_TIME_FIELD_TRUE_HEADING) || (b->an.rpisrv_emitted_true_heading != b->true_heading)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_true_heading = b->true_heading;
                        b->an.rpisrv_emitted_true_heading_time = tv.tv_sec;

                        acf->true_heading = b->true_heading;
                        acf->true_heading_set = 1;
                                                
                        airnav_log_level(3, "[%06X] Sending True Heading...%.2f\n", (b->addr & 0xffffff), b->true_heading);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] True Heading is the same for less than %d seconds, will NOT send anything (b->true_heading: %.2f, emitted_true_heading: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_TRUE_HEADING, b->true_heading, b->an.rpisrv_emitted_true_heading);
                    }

                }
                
                
                
                // MRAR Wind Speed and Dir
                if (trackDataAge(&b->wind_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mrar_wind_time) >= MAX_TIME_FIELD_MRAR_WIND) || (b->an.rpisrv_emitted_mrar_wind != b->wind_speed)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_mrar_wind = b->wind_speed;
                        b->an.rpisrv_emitted_mrar_wind_time = tv.tv_sec;

                        acf->mrar_wind_speed = b->wind_speed;
                        acf->mrar_wind_speed_set = 1;
                        
                        acf->mrar_wind_dir = b->wind_dir;
                        acf->mrar_wind_dir_set = 1;                        
                        
                        airnav_log_level(3, "[%06X] Sending MRAR Wind...Speed: %.2f, dir: %.2f\n", (b->addr & 0xffffff), b->wind_speed, b->wind_dir);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] MRAR Wind is the same for less than %d seconds, will NOT send anything (b->wind_speed: %.2f, emitted_wind_speed: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MRAR_WIND, b->wind_speed, b->an.rpisrv_emitted_mrar_wind);
                    }

                }
                
                // MRAR Pressure
                if (trackDataAge(&b->pressure_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mrar_pressure_time) >= MAX_TIME_FIELD_MRAR_PRESSURE) || (b->an.rpisrv_emitted_mrar_pressure != b->pressure)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_mrar_pressure = b->pressure;
                        b->an.rpisrv_emitted_mrar_pressure_time = tv.tv_sec;

                        acf->mrar_pressure = b->pressure;
                        acf->mrar_pressure_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending MRAR Pressure...%.2f\n", (b->addr & 0xffffff), b->pressure);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] MRAR Pressure is the same for less than %d seconds, will NOT send anything (b->pressure: %.2f, emitted_pressure: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MRAR_PRESSURE, b->pressure, b->an.rpisrv_emitted_mrar_pressure);
                    }

                }
                
                
                // MRAR Temperature
                if (trackDataAge(&b->temperature_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mrar_temperature_time) >= MAX_TIME_FIELD_MRAR_TEMPERATURE) || (b->an.rpisrv_emitted_mrar_temperature != b->temperature)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_mrar_temperature = b->temperature;
                        b->an.rpisrv_emitted_mrar_temperature_time = tv.tv_sec;

                        acf->mrar_temperature = b->temperature;
                        acf->mrar_temperature_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending MRAR Temperature...%.2f\n", (b->addr & 0xffffff), b->temperature);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] MRAR Temperature is the same for less than %d seconds, will NOT send anything (b->temperature: %.2f, emitted_temperature: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MRAR_TEMPERATURE, b->temperature, b->an.rpisrv_emitted_mrar_temperature);
                    }

                }
                
                // MRAR Humidity
                if (trackDataAge(&b->humidity_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mrar_humidity_time) >= MAX_TIME_FIELD_MRAR_HUMIDITY) || (b->an.rpisrv_emitted_mrar_humidity != b->humidity)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_mrar_humidity = b->humidity;
                        b->an.rpisrv_emitted_mrar_humidity_time = tv.tv_sec;

                        acf->mrar_humidity = b->humidity;
                        acf->mrar_humidity_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending MRAR Humidity...%.2f\n", (b->addr & 0xffffff), b->humidity);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] MRAR Humidity is the same for less than %d seconds, will NOT send anything (b->humidity: %.2f, emitted_humidity: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MRAR_HUMIDITY, b->humidity, b->an.rpisrv_emitted_mrar_humidity);
                    }

                }
                
                // MRAR Turbulence
                if (trackDataAge(&b->turbulence_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_mrar_turbulence_time) >= MAX_TIME_FIELD_MRAR_TURBULENCE) || (b->an.rpisrv_emitted_mrar_turbulence != b->turbulence)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_mrar_turbulence = b->turbulence;
                        b->an.rpisrv_emitted_mrar_turbulence_time = tv.tv_sec;

                        acf->mrar_turbulence = b->turbulence;
                        acf->mrar_turbulence_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending MRAR Turbulence...%.2f\n", (b->addr & 0xffffff), b->turbulence);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] MRAR Turbulence is the same for less than %d seconds, will NOT send anything (b->turbulence: %d, emitted_turbulence: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_MRAR_TURBULENCE, b->turbulence, b->an.rpisrv_emitted_mrar_turbulence);
                    }

                }
                
                // ADS-B Version
                if (b->adsb_version != -1) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_adsb_version_time) >= MAX_TIME_FIELD_ADSB_VERSION) || (b->an.rpisrv_emitted_adsb_version != b->adsb_version)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_adsb_version = b->adsb_version;
                        b->an.rpisrv_emitted_adsb_version_time = tv.tv_sec;

                        acf->adsb_version = b->adsb_version;
                        acf->adsb_version_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending ADS-B Version...%d\n", (b->addr & 0xffffff), b->adsb_version);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] ADS-B Version is the same for less than %d seconds, will NOT send anything (b->adsb_version: %d, emitted_adsb_version: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_ADSB_VERSION, b->adsb_version, b->an.rpisrv_emitted_adsb_version);
                    }

                }
                
                // ADS-R Version
                if (b->adsr_version != -1) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_adsr_version_time) >= MAX_TIME_FIELD_ADSR_VERSION) || (b->an.rpisrv_emitted_adsr_version != b->adsr_version)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_adsr_version = b->adsr_version;
                        b->an.rpisrv_emitted_adsr_version_time = tv.tv_sec;

                        acf->adsr_version = b->adsr_version;
                        acf->adsr_version_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending ADS-R Version...%d\n", (b->addr & 0xffffff), b->adsr_version);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] ADS-R Version is the same for less than %d seconds, will NOT send anything (b->adsr_version: %d, emitted_adsr_version: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_ADSR_VERSION, b->adsr_version, b->an.rpisrv_emitted_adsr_version);
                    }

                }
                
                // TIS-B Version
                if (b->tisb_version != -1) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_tisb_version_time) >= MAX_TIME_FIELD_TISB_VERSION) || (b->an.rpisrv_emitted_tisb_version != b->tisb_version)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_tisb_version = b->tisb_version;
                        b->an.rpisrv_emitted_tisb_version_time = tv.tv_sec;

                        acf->tisb_version = b->tisb_version;
                        acf->tisb_version_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending TIS-B Version...%d\n", (b->addr & 0xffffff), b->tisb_version);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] TIS-B Version is the same for less than %d seconds, will NOT send anything (b->tisb_version: %d, emitted_tisb_version: %d).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_TISB_VERSION, b->tisb_version, b->an.rpisrv_emitted_tisb_version);
                    }

                }
                
                // Roll
                if (trackDataAge(&b->roll_valid) <= AIRNAV_MAX_ITEM_AGE) {

                    if (((tv.tv_sec - b->an.rpisrv_emitted_roll_time) >= MAX_TIME_FIELD_ROLL) || (b->an.rpisrv_emitted_roll != b->roll)) { // Send only once every X seconds (or when data changed)
                        b->an.rpisrv_emitted_roll = b->roll;
                        b->an.rpisrv_emitted_roll_time = tv.tv_sec;

                        acf->roll = b->roll;
                        acf->roll_set = 1;                                                
                        
                        airnav_log_level(3, "[%06X] Sending Roll...%.2f\n", (b->addr & 0xffffff), b->roll);
                        send = 1;
                    } else {
                        airnav_log_level(3, "[%06X] Roll is the same for less than %d seconds, will NOT send anything (b->roll: %.2f, emitted_roll: %.2f).\n", (b->addr & 0xffffff), MAX_TIME_FIELD_ROLL, b->roll, b->an.rpisrv_emitted_roll);
                    }

                }

                acf->last_timestamp_us = b->an.rpisrv_emitted_last_timestamp_us;
                acf->timestamp_source = (char)b->an.rpisrv_emitted_timestamp_source;

                if (b->an.rpisrv_emitted_ntp_sync_ok) {
                    acf->ntp_sync_ok = 1;
                    acf->ntp_stratum = b->an.rpisrv_emitted_ntp_stratum;
                    acf->ntp_precision = b->an.rpisrv_emitted_ntp_precision;
                    acf->ntp_root_distance_ms = b->an.rpisrv_emitted_ntp_root_distance_ms;
                    acf->ntp_offset_ms = b->an.rpisrv_emitted_ntp_offset_ms;
                    acf->ntp_delay_ms = b->an.rpisrv_emitted_ntp_delay_ms;
                    acf->ntp_jitter_ms = b->an.rpisrv_emitted_ntp_jitter_ms;
                    acf->ntp_frequency_ppm = b->an.rpisrv_emitted_ntp_frequency_ppm;
                    airnav_log_level(5, "Copied NTP info from aircraft to p_data: last_wall %lld, ts_source %hhd, stratum %hhd, precision %hhd, distance %f, offset %f, delay %f, jitter %f, frequency %f\n",
                            acf->last_timestamp_us,
                            acf->timestamp_source,
                            acf->ntp_stratum,
                            acf->ntp_precision,
                            acf->ntp_root_distance_ms,
                            acf->ntp_offset_ms,
                            acf->ntp_delay_ms,
                            acf->ntp_jitter_ms,
                            acf->ntp_frequency_ppm);
                } else {
                    acf->ntp_sync_ok = 0;
                }

                if (send == 1) {
                    airnav_log_level(12, "[Lat:%8.05f,Lon:%8.05f]Hex:%06x CLS:%s HDG:%d ALT:%d GSD:%d VR:%d SQW:%04x IAS:%d AIRBRN: %d\n", acf->lat, acf->lon, acf->modes_addr, acf->callsign, (acf->heading * 10), acf->altitude, (acf->gnd_speed * 10), (acf->vert_rate * 10), acf->squawk, acf->ias, acf->airborne);
                    packet_cache_count++;
                    acf->cmd = 5;
                    // ANRB
                    acf2->cmd = 5;

                    tmp = malloc(sizeof (struct packet_list));
                    tmp->next = flist;
                    tmp->packet = acf;
                    flist = tmp;


                    // ANRB
                    tmp2 = malloc(sizeof (struct packet_list));
                    tmp2->next = flist2;
                    tmp2->packet = acf2;
                    flist2 = tmp2;

                    send = 0;

                    if (asterix_enabled == 1 && cat21_loaded == 1) {
                        asterix_SendCat21Packet(packet);
                    }


                } else {
                    free(acf);
                    // ANRB
                    free(acf2);
                    if (asterix_enabled == 1 && cat21_loaded == 1) {
                        free(packet);
                    }
                }


                b = b->next;
            }


        }
        pthread_mutex_unlock(&m_copy2);
        pthread_mutex_unlock(&m_copy);
        sleep(AIRNAV_SEND_INTERVAL); // Send every X second
    }


    airnav_log_level(1, "Exited prepareData Successfull!\n");
    pthread_exit(EXIT_SUCCESS);

}




//
// Return a description of the receiver in json.
//

char *airnav_generateStatusJson(const char *url_path, int *len) {

    char *buf = (char *) malloc(1024), *p = buf;
    //int history_size;

    MODES_NOTUSED(url_path);
    char *myip = net_getLocalIp();

    double pmu_temp = 0;

#ifdef RBCSRBLC
    pmu_temp = getPMUTemp();
#endif

    p += sprintf(p, "{" \
                 "\"rbfeeder\" : 1,"
            "\"vhf\": %d,"
            "\"mlat\": %d,"
            "\"acars\": %d,"
            "\"vhf_mode\": \"%s\","
            "\"vhf_freqs\": \"%s\","
            "\"vhf_gain\": %.1f,"
            "\"vhf_squelch\": %d,"
            "\"vhf_correction\": %d,"
            "\"vhf_afc\": %d,"
            "\"vhf_autostart\": %d,"
            "\"mlat_autostart\": %d,"
            "\"ip\": \"%s\","
            "\"mac\": \"%s\","
            "\"lat\": %.4f,"
            "\"lon\": %.4f,"
            "\"alt\": %d,"
            "\"start_datetime\": \"%s\","
            "\"sn\": \"%s\","
            "\"pmu_temp\" : %.2f, "
            "\"cpu_temp\" : %.2f, "
            "\"pmu_temp_max\" : %.2f, "
            "\"cpu_temp_max\" : %.2f, "
            "\"gps_fixed\" : %d ,"
            "\"gps_fix_mode\" : %d, "
            "\"sats_visible\" : %d, "
            "\"sats_used\" : %d, "
            "\"connected\": %d,"
            "\"version\": \"%s\", "
            "\"build_date\": \"%s\"",
            checkVhfRunning(), mlat_checkMLATRunning(), acars_checkACARSRunning(), vhf_mode, vhf_freqs, vhf_gain, vhf_squelch, vhf_correction, vhf_afc, autostart_vhf, autostart_mlat, myip, mac_a, g_lat, g_lon, g_alt, start_datetime, sn,
            pmu_temp, getCPUTemp(), 0.0, max_cpu_temp, 0, 0, 0, 0, airnav_com_inited, MODES_DUMP1090_VERSION, BDTIME);


    p += sprintf(p, "}\n");

    *len = (p - buf);
    free(myip);
    return buf;

}
