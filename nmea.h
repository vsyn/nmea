/* Copyright 2019 Julian Ingram
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NMEA_H
#define NMEA_H

#include <stdint.h>
#include <stdlib.h>

#define NMEA_MAX_SATS (20)
#define NMEA_MAX_PRNS_TRACKED (12)

static const unsigned long int NMEA_CENTURY = 2000;
static const unsigned long int NMEA_CENTURY_OFFSET = 946684800ul;

typedef unsigned short int nmea_sentence_bitmap_t;
typedef unsigned long int nmea_field_bitmap_t;
/* can accommodate 8 GSV messages */
typedef unsigned char nmea_gsv_bitmap_t;

enum nmea_fields {
  NMEA_FIELD_LONGITUDE = 0,
  NMEA_FIELD_LONGITUDE_DIR,
  NMEA_FIELD_LATITUDE_DIR,
  NMEA_FIELD_LATITUDE,

  NMEA_FIELD_FIX_QUALITY,
  NMEA_FIELD_SATELLITES_TRACKED,
  NMEA_FIELD_SATELLITES_IN_VIEW,
  NMEA_FIELD_ALTITUDE,

  NMEA_FIELD_GEOID_HEIGHT,
  NMEA_FIELD_FIX_3D,
  NMEA_FIELD_PRNS_TRACKED,
  NMEA_FIELD_PRN,

  NMEA_FIELD_PDOP,
  NMEA_FIELD_HDOP,
  NMEA_FIELD_VDOP,
  NMEA_FIELD_GLL_ACTIVE,

  NMEA_FIELD_RMC_ACTIVE,
  NMEA_FIELD_SPEED,
  NMEA_FIELD_TIME,
  NMEA_FIELD_DATE,

  NMEA_FIELD_MAGNETIC_VARIATION,
  NMEA_FIELD_MAGNETIC_VARIATION_DIR,
  NMEA_FIELD_IGNORE,
  NMEA_FIELD_GSV_SENTENCES_TOTAL,

  NMEA_FIELD_SENTENCE_NO,
  NMEA_FIELD_AZIMUTH,
  NMEA_FIELD_ELEVATION,
  NMEA_FIELD_SNR,

  NMEA_FIELD_TRUE_TRACK,
  NMEA_FIELD_MAGNETIC_TRACK,
  NMEA_FIELD_HEADER
};

static const nmea_field_bitmap_t NMEA_FB1 = 1;
static const nmea_field_bitmap_t NMEA_FIELD_LONGITUDE_MASK =
    NMEA_FB1 << NMEA_FIELD_LONGITUDE;
static const nmea_field_bitmap_t NMEA_FIELD_LONGITUDE_DIR_MASK =
    NMEA_FB1 << NMEA_FIELD_LONGITUDE_DIR;
static const nmea_field_bitmap_t NMEA_FIELD_LATITUDE_DIR_MASK =
    NMEA_FB1 << NMEA_FIELD_LATITUDE_DIR;
static const nmea_field_bitmap_t NMEA_FIELD_LATITUDE_MASK =
    NMEA_FB1 << NMEA_FIELD_LATITUDE;

static const nmea_field_bitmap_t NMEA_FIELD_FIX_QUALITY_MASK =
    NMEA_FB1 << NMEA_FIELD_FIX_QUALITY;
static const nmea_field_bitmap_t NMEA_FIELD_SATELLITES_TRACKED_MASK =
    NMEA_FB1 << NMEA_FIELD_SATELLITES_TRACKED;
static const nmea_field_bitmap_t NMEA_FIELD_SATELLITES_IN_VIEW_MASK =
    NMEA_FB1 << NMEA_FIELD_SATELLITES_IN_VIEW;
static const nmea_field_bitmap_t NMEA_FIELD_ALTITUDE_MASK =
    NMEA_FB1 << NMEA_FIELD_ALTITUDE;

static const nmea_field_bitmap_t NMEA_FIELD_GEOID_HEIGHT_MASK =
    NMEA_FB1 << NMEA_FIELD_GEOID_HEIGHT;
static const nmea_field_bitmap_t NMEA_FIELD_FIX_3D_MASK = NMEA_FB1
                                                          << NMEA_FIELD_FIX_3D;
static const nmea_field_bitmap_t NMEA_FIELD_PRNS_TRACKED_MASK =
    NMEA_FB1 << NMEA_FIELD_PRNS_TRACKED;
static const nmea_field_bitmap_t NMEA_FIELD_PRN_MASK = NMEA_FB1
                                                       << NMEA_FIELD_PRN;

static const nmea_field_bitmap_t NMEA_FIELD_PDOP_MASK = NMEA_FB1
                                                        << NMEA_FIELD_PDOP;
static const nmea_field_bitmap_t NMEA_FIELD_HDOP_MASK = NMEA_FB1
                                                        << NMEA_FIELD_HDOP;
static const nmea_field_bitmap_t NMEA_FIELD_VDOP_MASK = NMEA_FB1
                                                        << NMEA_FIELD_VDOP;
static const nmea_field_bitmap_t NMEA_FIELD_GLL_ACTIVE_MASK =
    NMEA_FB1 << NMEA_FIELD_GLL_ACTIVE;

static const nmea_field_bitmap_t NMEA_FIELD_RMC_ACTIVE_MASK =
    NMEA_FB1 << NMEA_FIELD_RMC_ACTIVE;
static const nmea_field_bitmap_t NMEA_FIELD_SPEED_MASK = NMEA_FB1
                                                         << NMEA_FIELD_SPEED;
static const nmea_field_bitmap_t NMEA_FIELD_TIME_MASK = NMEA_FB1
                                                        << NMEA_FIELD_TIME;
static const nmea_field_bitmap_t NMEA_FIELD_DATE_MASK = NMEA_FB1
                                                        << NMEA_FIELD_DATE;

static const nmea_field_bitmap_t NMEA_FIELD_MAGNETIC_VARIATION_MASK =
    NMEA_FB1 << NMEA_FIELD_MAGNETIC_VARIATION;
static const nmea_field_bitmap_t NMEA_FIELD_MAGNETIC_VARIATION_DIR_MASK =
    NMEA_FB1 << NMEA_FIELD_MAGNETIC_VARIATION_DIR;
static const nmea_field_bitmap_t NMEA_FIELD_IGNORE_MASK = NMEA_FB1
                                                          << NMEA_FIELD_IGNORE;
static const nmea_field_bitmap_t NMEA_FIELD_GSV_SENTENCES_TOTAL_MASK =
    NMEA_FB1 << NMEA_FIELD_GSV_SENTENCES_TOTAL;

static const nmea_field_bitmap_t NMEA_FIELD_SENTENCE_NO_MASK =
    NMEA_FB1 << NMEA_FIELD_SENTENCE_NO;
static const nmea_field_bitmap_t NMEA_FIELD_AZIMUTH_MASK =
    NMEA_FB1 << NMEA_FIELD_AZIMUTH;
static const nmea_field_bitmap_t NMEA_FIELD_ELEVATION_MASK =
    NMEA_FB1 << NMEA_FIELD_ELEVATION;
static const nmea_field_bitmap_t NMEA_FIELD_SNR_MASK = NMEA_FB1
                                                       << NMEA_FIELD_SNR;

static const nmea_field_bitmap_t NMEA_FIELD_TRUE_TRACK_MASK =
    NMEA_FB1 << NMEA_FIELD_TRUE_TRACK;
static const nmea_field_bitmap_t NMEA_FIELD_MAGNETIC_TRACK_MASK =
    NMEA_FB1 << NMEA_FIELD_MAGNETIC_TRACK;

/* Specifies how many fractional bits to use for the fixed point values of each
 * specific field, 0 indicates an integer */
static const unsigned char NMEA_FXP_FRACTIONALS[] = {
    [NMEA_FIELD_LONGITUDE] = 55,
    [NMEA_FIELD_LATITUDE] = 56,
    [NMEA_FIELD_ALTITUDE] = 10,
    [NMEA_FIELD_GEOID_HEIGHT] = 10,
    [NMEA_FIELD_PDOP] = 16,
    [NMEA_FIELD_HDOP] = 16,
    [NMEA_FIELD_VDOP] = 16,
    [NMEA_FIELD_SPEED] = 16,
    [NMEA_FIELD_MAGNETIC_VARIATION] = 24,
    [NMEA_FIELD_TRUE_TRACK] = 16,
    [NMEA_FIELD_MAGNETIC_TRACK] = 16};

enum nmea_fix_quality {
  NMEA_FIX_INVALID = 0,
  NMEA_FIX_GPS_FIX,
  NMEA_FIX_DGPS_FIX,
  NMEA_FIX_PPS_FIX,
  NMEA_FIX_REAL_TIME_KINEMATIC,
  NMEA_FIX_FLOAT_RTK,
  NMEA_FIX_ESTIMATED,
  NMEA_FIX_MANUAL_INPUT_MODE,
  NMEA_FIX_SIMULATION_MODE
};

enum nmea_fix_3d { NMEA_FIX_NONE = 1, NMEA_FIX_2D, NMEA_FIX_3D };

enum nmea_active { NMEA_VOID = 0, NMEA_ACTIVE };

struct nmea_sat {
  unsigned short int azimuth;
  unsigned char prn;
  unsigned char elevation;
  unsigned char snr;
};

struct nmea_fxp_state {
  unsigned long long int val;
  unsigned long long int div;
  unsigned int dp : 1;
  unsigned int neg : 1;
};

struct nmea_scratch_extra {
  unsigned long long int scratch[2];
};

union nmea_fxpse {
  struct nmea_fxp_state fxp;
  struct nmea_scratch_extra se;
};

struct nmea;

struct nmea_field_handlers {
  void (*start_handler)(struct nmea *const n);
  void (*char_handler)(struct nmea *const n, const char c);
  void (*end_handler)(struct nmea *const n);
};

struct nmea_sentence_format {
  const enum nmea_fields *const fields;
  void (*start_handler)(struct nmea *const n);
  void (*end_handler)(struct nmea *const n);
  void (*checksum_fail_handler)(struct nmea *const n);
  const char head[6];
  unsigned char length;
};

struct nmea_state {
  union nmea_fxpse fxpse;
  unsigned long long int scratch;
  const struct nmea_field_handlers *field_handlers;
  const struct nmea_sentence_format *sentence;
  nmea_field_bitmap_t received;
  enum nmea_fields field;
  unsigned int checksum_recording : 1;
  nmea_sentence_bitmap_t sentence_bitmap;
  nmea_field_bitmap_t field_bitmap;
  nmea_gsv_bitmap_t gsv_sentences_received;
  unsigned short int gsv_satellite_count;
  unsigned char gsv_satellite_index;
  unsigned char gsv_sentence_no;
  unsigned char gsv_sentences_total;
  unsigned char gsa_satellite_index;
  unsigned char gsa_satellite_count;
  unsigned char char_count;
  unsigned char comma_count;
  unsigned char checksum;
};

struct nmea_data {
  long long int longitude;
  long long int latitude;
  /* time in seconds since the epoch, updated by both date and time fields */
  long long int time;
  unsigned long int hdop;
  unsigned long int pdop;
  unsigned long int vdop;
  unsigned long int speed;
  long int true_track;
  long int magnetic_track;
  long int magnetic_variation;
  long int altitude;
  long int geoid_height;
  enum nmea_fix_quality fix_quality;
  enum nmea_fix_3d fix_3d;
  enum nmea_active gll_active;
  enum nmea_active rmc_active;
  struct nmea_sat sats[NMEA_MAX_SATS];
  unsigned char prns_tracked[NMEA_MAX_PRNS_TRACKED];
  unsigned short int satellites_tracked;
  unsigned short int satellites_in_view;
};

struct nmea {
  struct nmea_state state;
  struct nmea_data data;
};

/*
 * Pass the NMEA sentence into the c argument one char at a time.
 */
void nmea_parse(struct nmea *const n, const char c);

/*
 * If this function returns 1, the fields passed in through the fields argument
 * are considered valid to be read from the data struct until the next call to
 * nmea_parse.
 *
 * The fields are passed in as 1 shifted by the field's enum value.
 *
 * for example:
 *   nmea_fields_ready(&n, NMEA_FIELD_TIME_MASK | NMEA_FIELD_DATE_MASK);
 */
char nmea_fields_ready(struct nmea *const n, const nmea_field_bitmap_t fields);

/*
 * Called once on startup, also resets the parser if required.
 */
void nmea_init(struct nmea *const n);

#endif
