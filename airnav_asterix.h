/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#ifndef ASTERIX_H
#define ASTERIX_H

#include "rbfeeder.h"
#include <string.h>
#include <jansson.h>
#include <netinet/in.h>
#include "airnav_net.h"
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CAT21_ITEMS 100
#define MAX_CAT21_FSPEC_BYTES 10

    struct asterixItemDef {
        char id[10];
        unsigned int size;
        unsigned short in_use;
        unsigned char *data;
    };

    struct fspecByteDef {
        unsigned char fspec_byte;
        unsigned char fspec_byte_inuse;
    };

    struct asterixPacketDef_cat21 {
        unsigned int p_size;
        struct fspecByteDef fspec_byte[MAX_CAT21_FSPEC_BYTES];

        //char dataSourceSAC;
        //char dataSourceSIC;

        // List of eidls and content
        struct asterixItemDef fields[MAX_CAT21_ITEMS];

    };


    extern char *asterix_spec_path;

    // asterix
    extern char *asterix_host;
    extern int asterix_port;
    extern int asterix_enabled;
    extern char *cat21_spec;
    extern char *cat21_version;
    extern int cat21_loaded;
    extern int asterix_sic;
    extern int asterix_sac;
    extern int asterix_sid;
    extern int asterix_sid_enabled;
    extern struct asterixItemDef cat21items[MAX_CAT21_ITEMS];





    int asterix_CreateUdpSocket();
    int asterix_SendCat21Packet(struct asterixPacketDef_cat21 *packet);


    int asterix_get_bit(char bit, char byte);
    void asterix_bit_set(char bit, unsigned char *byte);

    int getItemIdx(struct asterixItemDef *toc, char *item_id, int max_items);
    struct asterixPacketDef_cat21 *prepareAsterixPacket_cat21(int max_fspec_bytes, int max_items);
    void asterix_set_byte_inuse(struct asterixPacketDef_cat21 *packet, unsigned int byte);
    int set_frn_inuse(struct asterixPacketDef_cat21 *packet, unsigned int frn);
    unsigned int map2bit(unsigned int item);
    int loadAsterixConfiguration();

    // Cat 021
    int loadCat21Specs(void);
    int cat021_setDataSourceSACSIC(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int SAC, unsigned int SIC);
    int cat021_setServiceId(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int SID);
    int cat021_setAirSpeed(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int type, short speed);
    int cat021_setEmitterCat(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char cat);
    int cat021_setServiveManagement(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char sid);
    int cat021_setReceiverID(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char rec_id);
    int cat021_setGeometricHeight(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, short height);
    int cat021_setMode3A(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned char mode3a[2]);
    int cat021_setMessageAmplitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char msg_amp);
    int cat021_setTrueAirSpeed(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short speed);
    int cat021_setMagneticHeading(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float heading);
    int cat021_setBaroVertRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short rate);
    int cat021_setGeoVertRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short rate);
    int cat021_setFlightLevel(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, int flevel);
    int cat021_setTargetId(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char tid[8]);
    int cat021_setTargetAddress(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned char address[3]);
    int cat021_setMOPSVersion(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char vns, char vn, char ltt);
    int cat021_setTrackNumber(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, short track);
    int cat021_setTrackAngleRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float rate);
    int cat021_setTargetStatus(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char icf, char lnav, char me, char ps, char ss);
    int cat021_setSelectedAltitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char sas, char source, short level);
    int cat021_setFinalStateSelectedAltitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char mv, char ah, char am, short altitude);
    int cat021_setAircraftOperStatus(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char ra, char tc, char ts, char arv, char cdtia, char ntcas, char sa);
    int cat021_setRollAngle(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float angle);
    int cat021_setTimeAplicabilityPosition(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time);
    int cat021_setTimeAplicabilityVelocity(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time);
    int cat021_setTimeMsgRcptPos(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time);
    int cat021_setTimeMsgRcptPosPrecision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char fsi, uint32_t time);
    int cat021_setTimeMsgRcptVel(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time);
    int cat021_setTimeMsgRcptVelPrecision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char fsi, uint32_t time);
    int cat021_setTimeASTERIXReport(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time);
    int cat021_setPosWGS84(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, double lat, double lon);
    int cat021_setPosWGS84Precision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, double lat, double lon);
    int cat021_setAirborneVector(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char re, short gspeed, short track_angle);


    int asterix_CreateUdpSocket();


#ifdef __cplusplus
}
#endif

#endif /* ASTERIX_H */

