/*
 * Copyright (c) 2020 - AirNav Systems
 * 
 * https://www.radarbox.com
 * 
 * More info: https://github.com/airnavsystems/rbfeeder
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

void geomag_geomg1(double alt, double glat, double glon, double time, double *dec, double *dip, double *ti, double *gv);
int geomag_my_isnan(double d);
void geomag_initialize();
