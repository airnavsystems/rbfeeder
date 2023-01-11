/*
 * Copyright (c) 2022 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "airnav_utils.h"
#include "airnav_ntp_status.h"

#define NTP_STATUS_CHECK_INTERVAL_SECS 30

ntp_status *status;
int getterRunning;
pthread_t getterThread;
time_t lastCheckedNtpStatus;
time_t lastCheckedNtpStatusSuccessfully;

void initStatus(ntp_status *status);
void *getNtpStatusInALoop(void *arg);
int isNtpSyncOkOnChrony();
int isNtpSyncOkOnSystemdTimesyncd();
int getNtpStatusFromChrony(ntp_status *newStatus);
int getNtpStatusFromSystemdTimesyncd(ntp_status *newStatus);

void initStatus(ntp_status *status) {
    status->stratum = -99;
    status->precision = -99;
    status->root_distance_ms = -99;
    status->offset_ms = -99;
    status->delay_ms = -99;
    status->jitter_ms = -99;
    status->frequency_ppm = -99;
}

int getNtpStatusFromSystemdTimesyncd(ntp_status *newStatus) {
    FILE *fp;
    char line[100];
    int errorFound = 0;
    
    fp = popen("timedatectl timesync-status |sed \"s/^ *//g\" |grep \"Stratum:\\|Precision:\\|Root distance:\\|Offset:\\|Delay:\\|Jitter:\\|Frequency:\"", "r");

    initStatus(newStatus);

    if (fp == NULL) {
        airnav_log_level(1, "Error: Failed to run command for getting NTP status details from systemd-timesyncd\n");
        return 1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *attributeName;
        char *attributeValue;
        int res;
        if (strlen(line) <= 1) {
            continue;
        }
        attributeName = strtok(line, ":");
        if (attributeName == NULL) {
            airnav_log_level(1, "Error: Could not get attribute name when reading system clock NTP status\n");
            errorFound = 1;
            break;
        }
        attributeValue = strtok(NULL, ":");
        if (attributeValue == NULL) {
            airnav_log_level(1, "Error: Could not get attribute value when reading system clock NTP status - %s\n", line);
            errorFound = 1;
            break;
        }

        if (strcmp(attributeName, "Stratum") == 0) {
            res = sscanf(attributeValue, "%hhd", &newStatus->stratum);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not parse value when reading stratum from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Precision") == 0) {
            attributeValue = strtok(attributeValue, "(");
            if (attributeValue == NULL) {
                airnav_log_level(1, "Error: Could not get attribute value when reading precision from NTP - %s\n", line);
                errorFound = 1;
                break;
            }
            attributeValue = strtok(NULL, ")");
            if (attributeValue == NULL) {
                airnav_log_level(1, "Error: Could not get raw attribute value when reading precision from NTP - %s\n", line);
                errorFound = 1;
                break;
            }
            res = sscanf(attributeValue, "%hhd", &newStatus->precision);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not parse value when reading precision from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Root distance") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->root_distance_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->root_distance_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading root distance from NTP - %s / %s / %f / %f / %d\n", attributeValue, rest, strtofResult, newStatus->root_distance_ms, strtofErrno);
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading root distance from NTP - %s / %s\n", attributeValue, rest);
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "s") == 0) {
                newStatus->root_distance_ms *= 1000.0f;
            } else if (strcmp(unit, "us") == 0) {
                newStatus->root_distance_ms /= 1000.0f;
            } else if (strcmp(unit, "ns") == 0) {
                newStatus->root_distance_ms /= 1000000.0f;
            } else if (strcmp(unit, "ms") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading root distance from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Offset") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->offset_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->offset_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading offset from NTP - %s / %s / %f / %f / %d\n", attributeValue, rest, strtofResult, newStatus->offset_ms, strtofErrno);
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading offset from NTP - %s / %s\n", attributeValue, rest);
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "s") == 0) {
                newStatus->offset_ms *= 1000.0f;
            } else if (strcmp(unit, "us") == 0) {
                newStatus->offset_ms /= 1000.0f;
            } else if (strcmp(unit, "ns") == 0) {
                newStatus->offset_ms /= 1000000.0f;
            } else if (strcmp(unit, "ms") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading offset from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Delay") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->delay_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->delay_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading delay from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading delay from NTP\n");
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "s") == 0) {
                newStatus->delay_ms *= 1000.0f;
            } else if (strcmp(unit, "us") == 0) {
                newStatus->delay_ms /= 1000.0f;
            } else if (strcmp(unit, "ns") == 0) {
                newStatus->delay_ms /= 1000000.0f;
            } else if (strcmp(unit, "ms") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading delay from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Jitter") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->jitter_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->jitter_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading jitter from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading jitter from NTP\n");
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "s") == 0) {
                newStatus->jitter_ms *= 1000.0f;
            } else if (strcmp(unit, "us") == 0) {
                newStatus->jitter_ms /= 1000.0f;
            } else if (strcmp(unit, "ns") == 0) {
                newStatus->jitter_ms /= 1000000.0f;
            } else if (strcmp(unit, "ms") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading jitter from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Frequency") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->frequency_ppm);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->frequency_ppm || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "ppm") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
        }
    }

    pclose(fp);

    return errorFound;
}

int getNtpStatusFromChrony(ntp_status *newStatus) {
    FILE *fp;
    char line[100];
    int errorFound = 0;
    
    fp = popen("chronyc tracking", "r");

    initStatus(newStatus);

    if (fp == NULL) {
        airnav_log_level(1, "Error: Failed to run command for getting NTP status details from chrony\n");
        return 1;
    }

    float rootDispersion = -99.0f;

    while (fgets(line, sizeof(line), fp)) {
        char *attributeName;
        char *attributeValue;
        int res;
        if (strlen(line) <= 1) {
            continue;
        }
        attributeName = strtok(line, ":");
        if (attributeName == NULL) {
            airnav_log_level(1, "Error: Could not get attribute name when reading system clock NTP status\n");
            errorFound = 1;
            break;
        }

        // Trim trailing spaces
        char* back = attributeName + strlen(attributeName);
        while (isspace(*--back));
        *(back + 1) = '\0';

        attributeValue = strtok(NULL, ":");
        if (attributeValue == NULL) {
            airnav_log_level(1, "Error: Could not get attribute value when reading system clock NTP status - %s\n", line);
            errorFound = 1;
            break;
        }

        if (strcmp(attributeName, "Stratum") == 0) {
            airnav_log_level(2, "Stratum value: %s\n", attributeValue);
            res = sscanf(attributeValue, "%hhd", &newStatus->stratum);
            airnav_log_level(2, "Stratum value saved: %hhd\n", newStatus->stratum);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not parse value when reading stratum from NTP\n");
                errorFound = 1;
                break;
            }
        }
        if (strcmp(attributeName, "Last offset") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->offset_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->offset_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading offset from NTP - %s / %s / %f / %f / %d\n", attributeValue, rest, strtofResult, newStatus->offset_ms, strtofErrno);
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading offset from NTP - %s / %s\n", attributeValue, rest);
                errorFound = 1;
                break;
            }
            newStatus->offset_ms *= 1000.0f;
        }
        if (strcmp(attributeName, "RMS offset") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->jitter_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->jitter_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading RMS offset from NTP - %s / %s / %f / %f / %d\n", attributeValue, rest, strtofResult, newStatus->jitter_ms, strtofErrno);
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading RMS offset from NTP - %s / %s\n", attributeValue, rest);
                errorFound = 1;
                break;
            }
            newStatus->jitter_ms *= 1000.0f;
        }
        if (strcmp(attributeName, "Root delay") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->delay_ms);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->delay_ms || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading delay from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading delay from NTP\n");
                errorFound = 1;
                break;
            }
            newStatus->delay_ms *= 1000.0f;
        }
        if (strcmp(attributeName, "Root dispersion") == 0) {
            res = sscanf(attributeValue, "%f", &rootDispersion);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != rootDispersion || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading dispersion from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading dispersion from NTP\n");
                errorFound = 1;
                break;
            }
            rootDispersion *= 1000.0f;
        }
        if (strcmp(attributeName, "Frequency") == 0) {
            res = sscanf(attributeValue, "%f", &newStatus->frequency_ppm);
            char *rest;
            char unit[10];
            float strtofResult = strtof(attributeValue, &rest);
            int strtofErrno = errno;
            if (strtofResult != newStatus->frequency_ppm || (strtofResult == 0 && strtofErrno != 0)) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s", unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "ppm") != 0) {
                airnav_log_level(1, "Error: Unknown unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            res = sscanf(rest, "%s %s", unit, unit);
            if (res < 0) {
                airnav_log_level(1, "Error: Could not get attribute unit when reading frequency from NTP\n");
                errorFound = 1;
                break;
            }
            if (strcmp(unit, "fast") == 0) {
                newStatus->frequency_ppm = -newStatus->frequency_ppm;
            } else if (strcmp(unit, "slow") != 0) {
                airnav_log_level(1, "Error: Unknown direction when reading frequency from NTP: %s\n", unit);
                errorFound = 1;
                break;
            }
        }
    }

    pclose(fp);

    if (!errorFound && newStatus->delay_ms != -99.0f && rootDispersion != -99.0f) {
        newStatus->root_distance_ms = newStatus->delay_ms / 2.0f + rootDispersion;
    }

    return errorFound;
}

int isNtpSyncOkOnSystemdTimesyncd() {
    FILE *fp;
    char line[100];

    fp = popen("service systemd-timesyncd status |grep Status |grep \"ynchronized\\|Initial synchronization\"", "r");

    if (fp == NULL) {
        airnav_log_level(1, "Error: Failed to run command for checking if NTP sync is ok on systemd-timesyncd\n");
        return 0;
    }

    int ok = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Status") != NULL) {
            ok = 1;
            break;
        }
    }

    pclose(fp);

    if (ok) {
        airnav_log_level(2, "System clock is synchronized using systemd-timesyncd\n");
    }

    return ok;
}

int isNtpSyncOkOnChrony() {
    FILE *fp;
    char line[100];

    fp = popen("chronyc tracking |grep Stratum |grep -v \": 0\"", "r");

    if (fp == NULL) {
        airnav_log_level(1, "Error: Failed to run command for checking if NTP sync is ok on chrony\n");
        return 0;
    }

    int ok = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Stratum") != NULL) {
            ok = 1;
            break;
        }
    }

    pclose(fp);

    if (ok) {
        airnav_log_level(2, "System clock is synchronized using chrony\n");
    }

    return ok;
}

void *getNtpStatusInALoop(__attribute__((unused)) void *arg) {

    while (getterRunning) {
        int ntpSyncIsActive = 1;
        time_t now;
        ntp_status newStatus;
        //int errorFound = 0;

        now = time(NULL);

        if (lastCheckedNtpStatus != 0 && now - lastCheckedNtpStatus < 30) {
            sleep(1);
            continue;
        }

        lastCheckedNtpStatus = now;
        
        if (isNtpSyncOkOnSystemdTimesyncd()) {
            ntpSyncIsActive = 1;
            /*errorFound =*/ getNtpStatusFromSystemdTimesyncd(&newStatus);
        } else if (isNtpSyncOkOnChrony()) {
            ntpSyncIsActive = 1;
            /*errorFound =*/ getNtpStatusFromChrony(&newStatus);
        }

        if (!ntpSyncIsActive /*|| errorFound*/) {
            ntp_status *oldStatus = status;
            status = NULL;
            if (oldStatus != NULL) {
                free(oldStatus);
            }
            continue;
        }

        ntp_status *newStatusInHeap = (ntp_status *)malloc(sizeof(ntp_status));

        *newStatusInHeap = newStatus;

        lastCheckedNtpStatusSuccessfully = now;

        airnav_log_level(2, "Got NTP status: stratum %hhd, precision 2^ %hhd, root distance %f ms, offset %f ms, delay %f ms, jitter %f ms, frequency %f ppm\n",
                newStatusInHeap->stratum,
                newStatusInHeap->precision,
                newStatusInHeap->root_distance_ms,
                newStatusInHeap->offset_ms,
                newStatusInHeap->delay_ms,
                newStatusInHeap->jitter_ms,
                newStatusInHeap->frequency_ppm);

        ntp_status *oldStatus = status;
        status = newStatusInHeap;
        if (oldStatus != NULL) {
            free(oldStatus);
        }
    }

    pthread_exit(EXIT_SUCCESS);
}

void ntpStatus_init() {
    status = NULL;
    getterRunning = 0;
    lastCheckedNtpStatus = 0;
    lastCheckedNtpStatusSuccessfully = 0;
}

void ntpStatus_loadNtpStatusConfig() {
    // TODO
}

void ntpStatus_startNtpStatusGetter() {
    if (getterRunning) {
        airnav_log_level(1, "NTP status getter was already running\n");
        return;
    }
    airnav_log_level(1, "Starting NTP status getter\n");
    getterRunning = 1;
    pthread_create(&getterThread, NULL, getNtpStatusInALoop, NULL);
}

void ntpStatus_stopNtpStatusGetter() {
    if (!getterRunning) {
        airnav_log_level(1, "NTP status getter was not running\n");
        return;
    }
    getterRunning = 0;
}

int ntpStatus_saveNtpStatusConfig() {
    // TODO
    return 0;
}

ntp_status *ntpStatus_getNtpStatus() {
    if (!lastCheckedNtpStatusSuccessfully) {
        return NULL;
    }
    time_t now = time(NULL);
    if (now - lastCheckedNtpStatusSuccessfully < 60) {
        return status;
    }
    return NULL;
}

int ntpStatus_isNtpStatusGetterRunning() {
    return getterRunning;
}
