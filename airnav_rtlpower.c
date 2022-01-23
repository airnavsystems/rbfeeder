/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifdef WIN32
#include <io.h>
#define READ_3RD_ARG unsigned int
#else
#include <unistd.h>
#define READ_3RD_ARG size_t
#endif


#include "airnav_rtlpower.h"
#include "airnav_utils.h"


int rfsurvey_execute = 0;
unsigned int rfsurvey_dongle;
unsigned int rfsurvey_min;
unsigned int rfsurvey_max;
pid_t p_rtlpower;
char *rtlpower_cmd;
char *rfsurvey_url;
char *rfsurvey_file;
double rfsurvey_gain;
char rfsurvey_key[501];
char *rfsurvey_samplerate;


/*
 * Check if dump1090-rb is running
 */
int rtlpower_checkRtlpowerRunning(void) {
    if (p_rtlpower <= 0) {
        return 0;
    }

    if (kill(p_rtlpower, 0) == 0) {
        return 1;
    } else {
        p_rtlpower = 0;
        return 0;
    }
}

/*
 * Start RTL_power, if not running
 */
void rtlpower_startRtlpower(void) {
/*
    if (rtlpower_checkRtlpowerRunning() != 0) {
        airnav_log_level(3, "Looks like rtl_power is already running.\n");
        return;
    }


    if (rtlpower_cmd == NULL) {
        airnav_log_level(3, "rtl_power command line not defined.\n");
        return;
    }


    char *tmp_cmd = malloc(300);
    memset(tmp_cmd, 0, 300);

    sprintf(tmp_cmd, "%s -d %u -f %uM:%uM:%s -g %.0f -c 20%% -i 2m -1 %s", rtlpower_cmd, rfsurvey_dongle, rfsurvey_min, rfsurvey_max,
            rfsurvey_samplerate, rfsurvey_gain, rfsurvey_file);

    airnav_log_level(3, "Starting rtl_power with this command: '%s'\n", tmp_cmd);

    p_rtlpower = run_cmd3(tmp_cmd);
    free(tmp_cmd);

    sleep(3);

    if (rtlpower_checkRtlpowerRunning() != 0) {
        airnav_log_level(3, "Ok, rtl_power started! Pid is: %i\n", p_rtlpower);
    } else {
        airnav_log_level(3, "Error starting rtl_power\n");
    }
*/
    return;
}

/*
 * Stop rtl_power
 */
void rtlpower_stopRtlpower(void) {

    if (rtlpower_checkRtlpowerRunning() == 0) {
        airnav_log_level(3, "rtl_power is not running.\n");
        return;
    }
    if (kill(p_rtlpower, SIGTERM) == 0) {
        airnav_log_level(3, "Succesfully stopped rtl_power!\n");
        sleep(1);
        return;
    } else {
        airnav_log_level(3, "Error stopping rtl_power.\n");
        return;
    }

    return;
}

curlioerr rtlpower_my_ioctl(CURL *handle, curliocmd cmd, void *userp) {
    int *fdp = (int *) userp;
    int fd = *fdp;

    (void) handle; /* not used in here */

    switch (cmd) {
        case CURLIOCMD_RESTARTREAD:
            /* mr libcurl kindly asks as to rewind the read data stream to start */
            if (-1 == lseek(fd, 0, SEEK_SET))
                /* couldn't rewind */
                return CURLIOE_FAILRESTART;

            break;

        default: /* ignore unknown commands */
            return CURLIOE_UNKNOWNCMD;
    }
    return CURLIOE_OK; /* success! */
}

/* read callback function, fread() look alike */
size_t rtlpower_read_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    ssize_t retcode;
    curl_off_t nread;

    AN_NOTUSED(nread);
    int *fdp = (int *) stream;
    int fd = *fdp;

    retcode = read(fd, ptr, (READ_3RD_ARG) (size * nmemb));

    nread = (curl_off_t) retcode;

    //fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T " bytes from file\n", nread);

    return retcode;
}

int rtlpower_curl_send_rtlpower(char *url, char *username, char *userpwd, char *rtl_file) {

    CURL *curl;
    CURLcode res;
    int hd;
    struct stat file_info;

#if LIBCURL_VERSION_NUM >= 0x073700
    curl_off_t speed_upload, total_time;
#endif

    char *file = rtl_file;

    /* get the file size of the local file */
    hd = open(file, O_RDONLY);
    fstat(hd, &file_info);

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, rtlpower_read_callback);

        /* which file to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &hd);

        /* set the ioctl function */
        curl_easy_setopt(curl, CURLOPT_IOCTLFUNCTION, rtlpower_my_ioctl);

        /* pass the file descriptor to the ioctl callback as well */
        curl_easy_setopt(curl, CURLOPT_IOCTLDATA, (void *) &hd);

        /* enable "uploading" (which means PUT when doing HTTP) */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* specify target URL, and note that this URL should also include a file
           name, not only a directory (as you can do with GTP uploads) */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* and give the size of the upload, this supports large file sizes
           on systems that have general support for it */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                (curl_off_t) file_info.st_size);

        /* tell libcurl we can use "any" auth, which lets the lib pick one, but it
           also costs one extra round-trip and possibly sending of all the PUT
           data twice!!! */
        //curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);
        char *tmp_auth = calloc(60, sizeof (char));
        sprintf(tmp_auth, "%s:%s", username, userpwd);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long) CURLAUTH_DIGEST);

        /* set user name and password for the authentication */
        curl_easy_setopt(curl, CURLOPT_USERPWD, tmp_auth);

        /* Now run off and do what you've been told! */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));


#if LIBCURL_VERSION_NUM >= 0x073700
        /* now extract transfer info */
        curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &speed_upload);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &total_time);


        airnav_log_level(1, "Speed: %" CURL_FORMAT_CURL_OFF_T " bytes/sec during %"
                CURL_FORMAT_CURL_OFF_T ".%06ld seconds\n",
                speed_upload,
                (total_time / 1000000), (long) (total_time % 1000000));

#endif

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    close(hd); /* close the local file */

    curl_global_cleanup();
    return 0;
}