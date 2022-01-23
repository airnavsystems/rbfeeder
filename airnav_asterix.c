/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/AirNav-Systems/rbfeeder
 * 
 */
#include "airnav_asterix.h"

// Variaveis
struct in_addr asterix_localInterface;
struct sockaddr_in asterix_groupSock;
int asterix_socket;
struct asterixItemDef cat21items[MAX_CAT21_ITEMS];

char *asterix_spec_path;
char *asterix_host;
int asterix_port;
int asterix_enabled;
char *cat21_spec;
char *cat21_version;
int cat21_loaded;
int asterix_sic;
int asterix_sac;
int asterix_sid;
int asterix_sid_enabled;

static inline void asterix_set_bit(unsigned char *number, unsigned int position) {
    *number |= (char) 1 << position;
}

static inline void asterix_clear_bit(unsigned char *number, unsigned int position) {
    *number &= ~((char) 1 << position);
}

int asterix_get_bit(char bit, char byte) {
    //bit = 1 << bit;
    //return(bit & byte);
    return ((byte >> bit) & 0x01);
}

void asterix_bit_set(char bit, unsigned char *byte) {
    bit = 1 << bit;
    *byte = *byte | bit;
}

/*
 * Search item index by ID
 */
int getItemIdx(struct asterixItemDef *toc, char *item_id, int max_items) {



    for (int i = 0; i < max_items; i++) {
        //printf("FRN: %d, ID: %s\n",i,cat21items[i].id);
        if (strcmp(item_id, toc[i].id) == 0) {
            //printf("ACHEI!!!\n");
            return i;
        }
    }

    return -1;
}

/*
 * Prepare/create Asterix Packet pointer
 */
struct asterixPacketDef_cat21 *prepareAsterixPacket_cat21(int max_fspec_bytes, int max_items) {

    struct asterixPacketDef_cat21 *res = malloc(sizeof (struct asterixPacketDef_cat21));

    //res->dataSourceSAC = 0;
    //res->dataSourceSIC = 0;
    res->p_size = 0;
    for (int i = 0; i < max_fspec_bytes; i++) {
        res->fspec_byte[i].fspec_byte = 0;
        res->fspec_byte[i].fspec_byte_inuse = 0;
    }


    for (int i = 0; i < max_items; i++) {
        res->fields[i].in_use = 0;
        res->fields[i].size = 0;
        res->fields[i].data = NULL;
    }

    return res;
}

/*
 * Set internal FSPEC byte usage
 */
void asterix_set_byte_inuse(struct asterixPacketDef_cat21 *packet, unsigned int byte) {

    packet->fspec_byte[byte].fspec_byte_inuse = 1;

    if (byte > 0) {
        for (unsigned int i = 0; i < byte; i++) {
            asterix_set_bit(&packet->fspec_byte[i].fspec_byte, 0);
            packet->fspec_byte[i].fspec_byte_inuse = 1;
        }
    }

}

/*
 * Set that this FRN is in use
 */
int set_frn_inuse(struct asterixPacketDef_cat21 *packet, unsigned int frn) {

    if (packet == NULL) {
        return -1;
    }

    int b_number = frn / 7;
    if ((frn % 7) > 0) {
        b_number++;
    }

    //printf("Bit-position for FRN %d: %d\n",frn,b_number);
    asterix_set_bit(&packet->fspec_byte[b_number - 1].fspec_byte, map2bit(frn - ((b_number - 1) * 7)));
    asterix_set_byte_inuse(packet, b_number - 1);

    return 1;
}

/*
 * Map FRN number to bit (inverse)
 */
unsigned int map2bit(unsigned int item) {

    if (item == 1) {
        return 7;
    } else if (item == 2) {
        return 6;
    } else if (item == 3) {
        return 5;
    } else if (item == 4) {
        return 4;
    } else if (item == 5) {
        return 3;
    } else if (item == 6) {
        return 2;
    } else if (item == 7) {
        return 1;
    } else if (item == 8) {
        return 0;
    }

    return 0;
}

/*
 * Set Aircraft Operational Status
 * ID: I021/008
 * Length: 1
 */
int cat021_setAircraftOperStatus(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char ra, char tc, char ts, char arv, char cdtia, char ntcas, char sa) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/008", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[0] = 0;

        // ra
        if (ra == 1) asterix_set_bit(&packet->fields[idx].data[0], 7);

        // tc
        if (asterix_get_bit(0, tc) == 1) asterix_set_bit(&packet->fields[idx].data[0], 5);
        if (asterix_get_bit(1, tc) == 1) asterix_set_bit(&packet->fields[idx].data[0], 6);

        // ts
        if (ts == 1) asterix_set_bit(&packet->fields[idx].data[0], 4);

        // arv
        if (arv == 1) asterix_set_bit(&packet->fields[idx].data[0], 3);

        // cdtia
        if (cdtia == 1) asterix_set_bit(&packet->fields[idx].data[0], 2);

        // ntcas
        if (ntcas == 1) asterix_set_bit(&packet->fields[idx].data[0], 1);

        // sa
        if (sa == 1) asterix_set_bit(&packet->fields[idx].data[0], 0);



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set data SAC and SIC
 * ID: I021/010
 * Length: 2
 */
int cat021_setDataSourceSACSIC(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int SAC, unsigned int SIC) {

    if (packet != NULL && toc != NULL && SAC > 0 && SIC > 0) {
        int idx = getItemIdx(toc, "I021/010", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);
        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(2);

        packet->fields[idx].data[0] = SAC;
        packet->fields[idx].data[1] = SIC;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Service Identification
 * ID: I021/015
 * Length: 1
 */
int cat021_setServiceId(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int SID) {

    if (packet != NULL && toc != NULL && SID > 0) {
        int idx = getItemIdx(toc, "I021/015", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);
        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(1);
        packet->fields[idx].data[0] = SID;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Service Management
 * ID: I021/016
 * Length: 1
 */
int cat021_setServiveManagement(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char sid) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/016", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = sid;

    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Emitter Category
 * ID: I021/020
 * Length: 1
 */
int cat021_setEmitterCat(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char cat) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/020", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = cat;

    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Mode3A
 * ID: I021/070
 * Length: 2  
 */
int cat021_setMode3A(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned char mode3a[2]) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/070", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = mode3a[0];
        packet->fields[idx].data[1] = mode3a[1];



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Aplicability for Position
 * ID: I021/071
 * Length: 3  
 */
int cat021_setTimeAplicabilityPosition(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/071", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        time = time * 128;
        packet->fields[idx].data[0] = (unsigned char) (time >> 16);
        packet->fields[idx].data[1] = (unsigned char) (time >> 8);
        packet->fields[idx].data[2] = (unsigned char) time;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Aplicability for Velocity
 * ID: I021/072
 * Length: 3  
 */
int cat021_setTimeAplicabilityVelocity(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/072", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        time = time * 128;
        packet->fields[idx].data[0] = (unsigned char) (time >> 16);
        packet->fields[idx].data[1] = (unsigned char) (time >> 8);
        packet->fields[idx].data[2] = (unsigned char) time;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Message Reception for Position
 * ID: I021/073
 * Length: 3  
 */
int cat021_setTimeMsgRcptPos(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/073", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        time = time * 128;
        packet->fields[idx].data[0] = (unsigned char) (time >> 16);
        packet->fields[idx].data[1] = (unsigned char) (time >> 8);
        packet->fields[idx].data[2] = (unsigned char) time;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Message Reception for Position High-precision
 * ID: I021/074
 * Length: 4  
 */
int cat021_setTimeMsgRcptPosPrecision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char fsi, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/074", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        //time = time * 128;
        float r = (float) time / 0.9313;
        //printf("Resultado float: %.2f\n",r);
        time = (uint32_t) r;

        packet->fields[idx].data[0] = (unsigned char) (time >> 24);
        packet->fields[idx].data[1] = (unsigned char) (time >> 16);
        packet->fields[idx].data[2] = (unsigned char) (time >> 8);
        packet->fields[idx].data[3] = (unsigned char) time;
        if (asterix_get_bit(0, fsi) == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 6);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 6);
        }
        if (asterix_get_bit(1, fsi) == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Message Reception for Velocity
 * ID: I021/075
 * Length: 3  
 */
int cat021_setTimeMsgRcptVel(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/075", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        time = time * 128;
        packet->fields[idx].data[0] = (unsigned char) (time >> 16);
        packet->fields[idx].data[1] = (unsigned char) (time >> 8);
        packet->fields[idx].data[2] = (unsigned char) time;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of Message Reception for Velocity High-precision
 * ID: I021/076
 * Length: 4  
 */
int cat021_setTimeMsgRcptVelPrecision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char fsi, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/076", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        //time = time * 128;
        float r = (float) time / 0.9313;
        //printf("Resultado float: %.2f\n",r);
        time = (uint32_t) r;

        packet->fields[idx].data[0] = (unsigned char) (time >> 24);
        packet->fields[idx].data[1] = (unsigned char) (time >> 16);
        packet->fields[idx].data[2] = (unsigned char) (time >> 8);
        packet->fields[idx].data[3] = (unsigned char) time;
        if (asterix_get_bit(0, fsi) == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 6);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 6);
        }
        if (asterix_get_bit(1, fsi) == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Time of ASTERIX Report Tx
 * ID: I021/077
 * Length: 3  
 */
int cat021_setTimeASTERIXReport(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, uint32_t time) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/077", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        time = time * 128;
        packet->fields[idx].data[0] = (unsigned char) (time >> 16);
        packet->fields[idx].data[1] = (unsigned char) (time >> 8);
        packet->fields[idx].data[2] = (unsigned char) time;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Target Address
 * ID: I021/080
 * Length: 3 
 */
int cat021_setTargetAddress(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned char address[3]) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/080", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = address[0];
        packet->fields[idx].data[1] = address[1];
        packet->fields[idx].data[2] = address[2];



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Position in WGS-84 co-ordinates
 * ID: I021/130
 * Length: 6  
 */
int cat021_setPosWGS84(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, double lat, double lon) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/130", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        const float weight = 180. / (1 << 23);
        int fp_lat = (int) (0.5f + lat / weight);

        int fp_lon = (int) (0.5f + lon / weight);


        packet->fields[idx].data[3] = 0;
        packet->fields[idx].data[4] = 0;
        packet->fields[idx].data[5] = 0;



        /*latitude */
        packet->fields[idx].data[0] = (fp_lat >> 16) & 0xff;
        ; /* most significant byte of 24 bits*/
        packet->fields[idx].data[1] = (fp_lat >> 8) & 0xff;
        packet->fields[idx].data[2] = (fp_lat) & 0xff;


        /*longitude */
        packet->fields[idx].data[3] = (fp_lon >> 16) & 0xff;
        ; /* most significant byte of 24 bits*/
        packet->fields[idx].data[4] = (fp_lon >> 8) & 0xff;
        packet->fields[idx].data[5] = (fp_lon) & 0xff;





    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Position in WGS-84 co-ordinates High-Precision
 * ID: I021/131
 * Length: 8  
 */
int cat021_setPosWGS84Precision(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, double lat, double lon) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/131", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        const float weight = 180. / (1 << 30);
        long fp_lat = (int) (0.5f + lat / weight);

        long fp_lon = (int) (0.5f + lon / weight);

        packet->fields[idx].data[3] = 0;
        packet->fields[idx].data[4] = 0;
        packet->fields[idx].data[5] = 0;


        /*latitude */
        packet->fields[idx].data[0] = (fp_lat >> 24) & 0xff;
        ;
        packet->fields[idx].data[1] = (fp_lat >> 16) & 0xff;
        ; /* most significant byte of 24 bits*/
        packet->fields[idx].data[2] = (fp_lat >> 8) & 0xff;
        packet->fields[idx].data[3] = (fp_lat) & 0xff;


        /*longitude */
        packet->fields[idx].data[4] = (fp_lon >> 24) & 0xff;
        ; /* most significant byte of 24 bits*/
        packet->fields[idx].data[5] = (fp_lon >> 16) & 0xff;
        ; /* most significant byte of 24 bits*/
        packet->fields[idx].data[6] = (fp_lon >> 8) & 0xff;
        packet->fields[idx].data[7] = (fp_lon) & 0xff;



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Message Amplitude
 * ID: I021/132
 * Length: 1
 */
int cat021_setMessageAmplitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char msg_amp) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/132", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = msg_amp;

    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Geometric Height
 * ID: I021/140
 * Length: 2  
 */
int cat021_setGeometricHeight(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, short height) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/140", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        short temp; // = (float)heading * (float)0.0055;
        temp = (height / 6.25);


        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[1] = temp & 0xff;
        packet->fields[idx].data[0] = (temp >> 8) & 0xff;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Flight Level
 * ID: I021/145
 * Length: 2
 */
int cat021_setFlightLevel(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, int flevel) {


    if (packet != NULL && toc != NULL && flevel > 0) {
        int idx = getItemIdx(toc, "I021/145", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        short temp2 = (short) ((float) flevel / 0.25);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[1] = temp2 & 0xff;
        packet->fields[idx].data[0] = (temp2 >> 8) & 0xff;

    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Selected Altitude
 * ID: I021/146
 * Length: 2
 */
int cat021_setSelectedAltitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char sas, char source, short level) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/146", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);


        float tmp = (float) level / (float) 25;
        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[1] = (short) tmp & 0xff;
        packet->fields[idx].data[0] = ((short) tmp >> 8) & 0xff;

        // sas
        if (sas == 1) asterix_set_bit(&packet->fields[idx].data[0], 7);

        // source
        if (asterix_get_bit(0, source) == 1) asterix_set_bit(&packet->fields[idx].data[0], 5);
        if (asterix_get_bit(1, source) == 1) asterix_set_bit(&packet->fields[idx].data[0], 6);



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Final State Selected Altitude
 * ID: I021/148
 * Length: 2
 */
int cat021_setFinalStateSelectedAltitude(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char mv, char ah, char am, short altitude) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/148", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);


        float tmp = (float) altitude / (float) 25;
        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[1] = (short) tmp & 0xff;
        packet->fields[idx].data[0] = ((short) tmp >> 8) & 0xff;

        // mv
        if (mv == 1) asterix_set_bit(&packet->fields[idx].data[0], 7);

        // ah
        if (ah == 1) asterix_set_bit(&packet->fields[idx].data[0], 6);

        // am
        if (am == 1) asterix_set_bit(&packet->fields[idx].data[0], 5);



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Air Speed
 * ID: I021/150
 * Length: 2
 */
int cat021_setAirSpeed(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int type, short speed) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/150", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[1] = speed & 0xff;
        packet->fields[idx].data[0] = (speed >> 8) & 0xff;
        if (type == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set True Air Speed
 * ID: I021/151
 * Length: 2
 * RE = Range Exceeded Indicator (1 for exceed)
 */
int cat021_setTrueAirSpeed(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short speed) {


    airnav_log_level(1, "True air speed (TAS) under ASTERIX: %d\n", speed);
    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/151", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[1] = speed & 0xff;
        packet->fields[idx].data[0] = (speed >> 8) & 0xff;


        if (re == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Magnetic Heading
 * ID: I021/152
 * Length: 2  
 */
int cat021_setMagneticHeading(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float heading) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/152", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        short temp; // = (float)heading * (float)0.0055;
        temp = (heading * (float) 65536.0) / (float) 360.0;

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[1] = temp & 0xff;
        packet->fields[idx].data[0] = (temp >> 8) & 0xff;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Barometric Vertical Rate
 * ID: I021/155
 * Length: 2
 * RE = Range Exceeded Indicator (1 for exceed)
 * INCOMPLETE
 */
int cat021_setBaroVertRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short rate) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/155", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);


        packet->fields[idx].data[1] = rate & 0xff;
        packet->fields[idx].data[0] = (rate >> 8) & 0xff;


        if (re == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Geometric Vertical Rate
 * ID: I021/157
 * Length: 2
 * RE = Range Exceeded Indicator (1 for exceed)
 * INCOMPLETE
 */
int cat021_setGeoVertRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, unsigned int re, short rate) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/157", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);


        packet->fields[idx].data[1] = rate & 0xff;
        packet->fields[idx].data[0] = (rate >> 8) & 0xff;


        if (re == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
        }


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Airborne Ground Vector
 * ID: I021/160
 * Length: 4  
 */
int cat021_setAirborneVector(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char re, short gspeed, short track_angle) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/160", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        //        printf("IDX: %d\n",idx);
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);


        short temp; // = (float)heading * (float)0.0055;
        temp = (track_angle * 65536) / 360;

        short gspeed_t = gspeed / .22;

        packet->fields[idx].data[2] = (temp >> 8) & 0xff;
        packet->fields[idx].data[3] = temp & 0xff;


        packet->fields[idx].data[0] = (gspeed_t >> 8) & 0xff;
        packet->fields[idx].data[1] = gspeed_t & 0xff;


        //printf("Data[0]: 0x%X\n",packet->fields[idx].data[0]);

        if (re == 1) {
            asterix_set_bit(&packet->fields[idx].data[0], 7);
            //  printf("RE = 1\n");
        } else {
            asterix_clear_bit(&packet->fields[idx].data[0], 7);
            // printf("RE = 0\n");
        }



    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Track Number
 * ID: I021/161
 * Length: 2  
 */
int cat021_setTrackNumber(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, short track) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/161", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[1] = 0;
        packet->fields[idx].data[1] = 0;

        packet->fields[idx].data[1] = track & 0xff;
        packet->fields[idx].data[0] = (track >> 8) & 0xff;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Track Angle Rate
 * ID: I021/165
 * Length: 2  
 * NEED TO VALIDATE!!!!!
 */
int cat021_setTrackAngleRate(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float rate) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/165", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        //packet->fields[idx].data[0] = 0;
        //packet->fields[idx].data[1] = 1;

        short r1 = (short) (rate * 32);

        packet->fields[idx].data[1] = r1 & 0xff;
        packet->fields[idx].data[0] = (r1 >> 8) & 0xff;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Target Identification
 * ID: I021/170
 * Length: 6 
 */
int cat021_setTargetId(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char tid[8]) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/170", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[0] = 0;
        packet->fields[idx].data[1] = 0;
        packet->fields[idx].data[2] = 0;
        packet->fields[idx].data[3] = 0;
        packet->fields[idx].data[4] = 0;
        packet->fields[idx].data[5] = 0;


        int idx_byte = 0;
        int idx_bit = 7;

        // Each input char
        for (int i = 0; i < 8; i++) {

            // First 5 bit's of char
            for (int b = 5; b >= 0; b--) {
                //
                if (asterix_get_bit(b, tid[i]) == 1) asterix_set_bit(&packet->fields[idx].data[idx_byte], idx_bit);
                //        printf("TID %d, idx_bit %d, idx_byte %d\n",i, idx_bit, idx_byte);
                if (idx_bit == 0) {
                    idx_bit = 7;
                    idx_byte++;
                } else {
                    idx_bit--;
                }



            }

        }




    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Target Status
 * ID: I021/200
 * Length: 1
 */
int cat021_setTargetStatus(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char icf, char lnav, char me, char ps, char ss) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/200", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[0] = 0;

        // ICF
        if (icf == 1) asterix_set_bit(&packet->fields[idx].data[0], 7);

        // LNAV
        if (lnav == 1) asterix_set_bit(&packet->fields[idx].data[0], 6);

        // ME
        if (me == 1) asterix_set_bit(&packet->fields[idx].data[0], 5);

        // ps
        if (asterix_get_bit(0, ps) == 1) asterix_set_bit(&packet->fields[idx].data[0], 2);
        if (asterix_get_bit(1, ps) == 1) asterix_set_bit(&packet->fields[idx].data[0], 3);
        if (asterix_get_bit(2, ps) == 1) asterix_set_bit(&packet->fields[idx].data[0], 4);

        // ss
        if (asterix_get_bit(0, ss) == 1) asterix_set_bit(&packet->fields[idx].data[0], 0);
        if (asterix_get_bit(1, ss) == 1) asterix_set_bit(&packet->fields[idx].data[0], 1);


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set MOPS Version
 * ID: I021/210
 * Length: 1
 */
int cat021_setMOPSVersion(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char vns, char vn, char ltt) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/210", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[0] = 0;

        // vns
        if (vns == 1) asterix_set_bit(&packet->fields[idx].data[0], 6);

        // vn
        if (asterix_get_bit(0, vn) == 1) asterix_set_bit(&packet->fields[idx].data[0], 3);
        if (asterix_get_bit(1, vn) == 1) asterix_set_bit(&packet->fields[idx].data[0], 4);
        if (asterix_get_bit(2, vn) == 1) asterix_set_bit(&packet->fields[idx].data[0], 5);

        // ltt
        if (asterix_get_bit(0, ltt) == 1) asterix_set_bit(&packet->fields[idx].data[0], 0);
        if (asterix_get_bit(1, ltt) == 1) asterix_set_bit(&packet->fields[idx].data[0], 1);
        if (asterix_get_bit(2, ltt) == 1) asterix_set_bit(&packet->fields[idx].data[0], 2);

    } else {
        return -1;
    }


    return 1;
}

/*
 * Set Roll Angle
 * ID: I021/230
 * Length: 2   
 */
int cat021_setRollAngle(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, float angle) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/230", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);

        packet->fields[idx].data[0] = 0;
        packet->fields[idx].data[1] = 1;

        short r1 = (short) (angle * 100);

        //r1 = 100;
        packet->fields[idx].data[1] = r1 & 0xff;
        packet->fields[idx].data[0] = (r1 >> 8) & 0xff;


    } else {
        return -1;
    }


    return 1;
}

/*
 * Set ReceiverID
 * ID: I021/400
 * Length: 1
 */
int cat021_setReceiverID(struct asterixPacketDef_cat21 *packet, struct asterixItemDef *toc, char rec_id) {

    if (packet != NULL && toc != NULL) {
        int idx = getItemIdx(toc, "I021/400", MAX_CAT21_ITEMS);
        if (idx < 0) {
            return -1;
        }
        set_frn_inuse(packet, idx);

        packet->fields[idx].in_use = 1;
        packet->fields[idx].size = toc[idx].size;
        packet->fields[idx].data = malloc(toc[idx].size);
        packet->fields[idx].data[0] = rec_id;

    } else {
        return -1;
    }


    return 1;
}

/*
 * 
 * Load JSON with category specifications
 * 
 */
int loadCat21Specs(void) {


    json_t *root;
    json_error_t error;
    json_t *data, *version, *items, *id, *frn, *size = NULL;
    //const char *version_text = NULL;

    if (asterix_spec_path == NULL) {
        return 0;
    }


    // Zero all itens
    for (int i = 0; i < MAX_CAT21_ITEMS; i++) {
        sprintf(cat21items[i].id, "\n");
    }

    char file_cat21[100] = {0};

    sprintf(file_cat21, "%s/%s", asterix_spec_path, cat21_spec);

    root = json_load_file(file_cat21, 0, &error);
    if (!root) {
        /* the error variable contains error information */
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return 0;
    }



    if (!json_is_array(root)) {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(root);
        return 0;
    }


    data = json_array_get(root, 0);
    if (!json_is_object(data)) {
        fprintf(stderr, "error: commit data %d is not an object\n", 0);
        json_decref(root);
        return 0;
    }



    version = json_object_get(data, "@version");
    if (!json_is_string(version)) {
        fprintf(stderr, "error: @version %d: @version is not a string\n", 0);
        json_decref(root);
        return 0;
    }


    const char *cat21_version_tmp;
    cat21_version_tmp = json_string_value(version);
    if (cat21_version != NULL) {
        free(cat21_version);
    }
    cat21_version = malloc(strlen(cat21_version_tmp) + 1);
    memcpy(cat21_version, cat21_version_tmp, strlen(cat21_version_tmp));
    //printf("Versão do CAT21: '%s'\n", cat21_version);


    items = json_object_get(data, "dataitem");
    if (!json_is_array(items)) {
        fprintf(stderr, "error: items %d: items is not a array\n", 0);
        json_decref(root);
        return 0;
    }



    /* array is a JSON array */
    size_t index;
    json_t *value;
    int int_frn = -1;
    unsigned int int_size = 0;
    const char *str_id = NULL;

    json_array_foreach(items, index, value) {
        /* block of code that uses index and value */

        id = json_object_get(value, "@id");
        frn = json_object_get(value, "@frn");
        size = json_object_get(value, "@length");

        int_frn = atoi(json_string_value(frn));
        str_id = json_string_value(id);
        int_size = atoi(json_string_value(size));

        sprintf(cat21items[int_frn].id, "%s", str_id);
        cat21items[int_frn].size = int_size;

        //printf("ID %s, FRN %d\n", json_string_value(id), atoi(json_string_value(frn)));

    }


    json_decref(root);

    cat21_loaded = 1;

    return 1;
}

int asterix_CreateUdpSocket() {


    //if (asterix_socket < 0) {
    //    return 0;
    //}

    if (asterix_socket > 0) {
        close(asterix_socket);
    }


    /* Create a datagram socket on which to send. */
    asterix_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (asterix_socket < 0) {
        airnav_log("Opening datagram socket error");
        return 0;
    } else
        airnav_log("Asterix Socket OK\n");

    /* Initialize the group sockaddr structure with a */
    /* group address of 225.1.1.1 and port 5555. */
    memset((char *) &asterix_groupSock, 0, sizeof (asterix_groupSock));
    asterix_groupSock.sin_family = AF_INET;


    char *hostname = malloc(strlen(asterix_host) + 1);
    strcpy(hostname, asterix_host);
    char ip[100] = {0};
    if (net_hostname_to_ip(hostname, ip)) { // Error
        airnav_log_level(2, "Could not resolve hostname for asterix....using default IP.\n");
        strcpy(ip, "192.168.0.255"); // Default IP
    }
    airnav_log_level(3, "(Asterix) Host %s resolved as %s\n", hostname, ip);
    free(hostname);
    inet_pton(AF_INET, ip, &(asterix_groupSock.sin_addr));
    asterix_groupSock.sin_port = htons(asterix_port);


    /* Set local interface for outbound multicast datagrams. */
    /* The IP address specified must be associated with a local, */
    /* multicast capable interface. */
    asterix_localInterface.s_addr = INADDR_ANY;
    if (setsockopt(asterix_socket, IPPROTO_IP, IP_MULTICAST_IF, (char *) &asterix_localInterface, sizeof (asterix_localInterface)) < 0) {
        airnav_log("Asterix setting local interface error");
        return 0;
    } else
        airnav_log("Asterix setting the local interface OK\n");


    /*
    if (sendto(sd, data_out, datalen, 0, (struct sockaddr*) &groupSock, sizeof (groupSock)) < 0)
    {
        perror("Sending datagram message error");
    }
    else
        printf("Sending datagram message...OK\n");
     */

    return 1;
}

int asterix_SendCat21Packet(struct asterixPacketDef_cat21 *packet) {

    // Socket not created
    if (asterix_socket < 0) {
        return 0;
    }

    airnav_log_level(3, "Sending Asterix Packet....\n");

    unsigned int data_size = 0;
    unsigned int fspec_size = 0;
    int datalen = 0;

    for (int i = 0; i < MAX_CAT21_ITEMS; i++) {
        //
        if (packet->fields[i].in_use == 1) {
            airnav_log_level(3, "Field ID %s (IDX: %d) is in use. Size: %d\n", cat21items[i].id, i, packet->fields[i].size);
            data_size = data_size + packet->fields[i].size;
        }
    }

    for (int i = 0; i < MAX_CAT21_FSPEC_BYTES; i++) {
        if (packet->fspec_byte[i].fspec_byte_inuse == 1) {
            airnav_log_level(3, "Byte %d em uso...\n", i + 1);
            fspec_size++;
        }
    }


    datalen = 3 + (data_size + fspec_size);
    airnav_log_level(3, "Data size: %u, FSPEC size: %u (Data Length: %d)\n", data_size, fspec_size, datalen);


    //return 0;

    char *data_out = malloc(datalen);
    data_out[0] = 21; // Categoria

    data_out[1] = 0; // Size 1
    data_out[2] = datalen; // Size 2

    int cur_idx = 3;

    //
    for (int i = 0; i < MAX_CAT21_FSPEC_BYTES; i++) {
        if (packet->fspec_byte[i].fspec_byte_inuse == 1) {
            data_out[cur_idx] = packet->fspec_byte[i].fspec_byte;
            cur_idx++;
        }
    }
    //printf("Current index: %d\n",cur_idx);


    for (int i = 0; i < MAX_CAT21_ITEMS; i++) {
        if (packet->fields[i].in_use == 1) {
            //printf("Field ID %s (IDX: %d) is in use. Size: %d\n", cat21items[i].id, i, packet->fields[i].size);
            //data_size = data_size + packet->fields[i].size;
            for (unsigned int x = 0; x < packet->fields[i].size; x++) {
                data_out[cur_idx] = packet->fields[i].data[x];
                cur_idx++;
            }
        }
    }


    if (sendto(asterix_socket, data_out, datalen, 0, (struct sockaddr*) &asterix_groupSock, sizeof (asterix_groupSock)) < 0) {
        airnav_log("Sending datagram message error");
        free(data_out);
        free(packet);
        return 0;
    }


    free(data_out);
    free(packet);
    return 1;
}

int loadAsterixConfiguration() {

    ini_getString(&asterix_host, configuration_file, "asterix", "host", "192.168.0.255");
    asterix_port = ini_getInteger(configuration_file, "asterix", "port", 50050);
    asterix_enabled = ini_getBoolean(configuration_file, "asterix", "enabled", 0);
    ini_getString(&cat21_spec, configuration_file, "asterix", "cat21_specs", "cat21_2.4.json");
    asterix_sic = ini_getInteger(configuration_file, "asterix", "sic", 1);
    asterix_sac = ini_getInteger(configuration_file, "asterix", "sac", 1);
    asterix_sid = ini_getInteger(configuration_file, "asterix", "sid", 1);
    asterix_sid_enabled = ini_getBoolean(configuration_file, "asterix", "send_sid", 0);

    // Specs folder
    ini_getString(&asterix_spec_path, configuration_file, "asterix", "specs_folder", "/opt/radarbox/specs");


    cat21_loaded = 0;

    if (asterix_enabled == 1) {

        if (loadCat21Specs() != 1) {
            //airnav_log("Error loading Asterix CAT021 specifications file.\n");
        } else {
            //airnav_log("Asterix CAT021 version loaded: %s\n", cat21_version);
            asterix_CreateUdpSocket();
        }

    }
    return 1;
}