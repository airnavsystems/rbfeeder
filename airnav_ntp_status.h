/*
 * Copyright (c) 2022 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#ifndef AIRNAV_NTP_STATUS_H
#define AIRNAV_NTP_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ntp_status {
        int8_t stratum;
        int8_t precision;
        float root_distance_ms;
        float offset_ms;
        float delay_ms;
        float jitter_ms;
        float frequency_ppm;
    } ntp_status;
    
    void ntpStatus_init(void);
    void ntpStatus_loadNtpStatusConfig(void);
    void ntpStatus_startNtpStatusGetter(void);
    void ntpStatus_stopNtpStatusGetter(void);
    int ntpStatus_saveNtpStatusConfig(void);
    ntp_status *ntpStatus_getNtpStatus(void);
    int ntpStatus_isNtpStatusGetterRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* AIRNAV_NTP_STATUS_H */

