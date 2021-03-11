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

#include "nmea.h"

#include <limits.h>
#include <string.h>

static const unsigned long int SECONDS_IN_MINUTE = 60;
static const unsigned long int SECONDS_IN_HOUR = 3600;
static const unsigned long int SECONDS_IN_DAY = 86400;

static int check_post_multiply(const size_t a, const size_t b, const size_t q) {
  return ((b != 0) && ((q / b) != a)) ? -1 : 0;
}

static int check_multiply(unsigned long long int *const q,
                          const unsigned long long int a,
                          const unsigned long long int b) {
  *q = a * b;
  return check_post_multiply(a, b, *q);
}

static void ufxp_init(struct nmea_fxp_state *const state) {
  state->val = 0;
  state->dp = 0;
}

/* q is the number of fractional bits */
static void ufxp_from_ascii(struct nmea_fxp_state *const state, const char c,
                            const unsigned char q) {
  if (c == '.') {
    state->dp = 1;
    state->div = 10;
  } else {
    unsigned long long int inc = ((unsigned long long int)(c - '0')) << q;
    if (state->dp == 0) {
      state->val *= 10;
      state->val += inc;
      if (state->val < inc) {
        state->val = ULLONG_MAX;
      }
    } else {
      state->val += inc / state->div;
      if (check_multiply(&state->div, 10, state->div) != 0) {
        state->div = ULLONG_MAX;
      }
    }
  }
  return;
}

static unsigned long long int
ufxp_get_val(const struct nmea_fxp_state *const state) {
  return state->val;
}

static void fxp_init(struct nmea_fxp_state *const state) {
  state->neg = 0;
  ufxp_init(state);
}

static void fxp_from_ascii(struct nmea_fxp_state *const state, const char c,
                           const unsigned char q) {
  if (c == '-') {
    state->neg = 1;
  } else {
    ufxp_from_ascii(state, c, q);
  }
}

static long long int fxp_get_val(const struct nmea_fxp_state *const state) {
  unsigned long long int usval = ufxp_get_val(state);
  long long int val = (usval > LLONG_MAX) ? LLONG_MAX : (long long int)usval;
  return (state->neg != 0) ? -val : val;
}

static void ignore_handler(struct nmea *const n) { (void)n; }

static void ignore_char_handler(struct nmea *const n, const char c) {
  (void)n;
  (void)c;
}

static void ufxp_init_start_handler(struct nmea *const n) {
  ufxp_init(&n->state.fxpse.fxp);
}

static void fxp_init_start_handler(struct nmea *const n) {
  fxp_init(&n->state.fxpse.fxp);
}

static void ui_char_handler(struct nmea *const n, const char c) {
  ufxp_from_ascii(&n->state.fxpse.fxp, c, 0);
}

static void reset_cc_start_handler(struct nmea *n) { n->state.char_count = 0; }

static void longitude_start_handler(struct nmea *const n) {
  ufxp_init(&n->state.fxpse.fxp);
  n->state.char_count = 0;
  n->state.scratch = 0;
}

static void lon_lat_char_handler(struct nmea *const n, const char c,
                                 const unsigned char degc,
                                 const unsigned char q) {
  if (n->state.char_count < degc) {
    /* degrees */
    n->state.scratch *= 10;
    n->state.scratch += c - '0';
    ++n->state.char_count;
  } else {
    /* minutes */
    ufxp_from_ascii(&n->state.fxpse.fxp, c, q);
  }
}

static void longitude_char_handler(struct nmea *const n,
                                   const char c) { /* must hold +-180 Q9.55 */
  lon_lat_char_handler(n, c, 3, NMEA_FXP_FRACTIONALS[NMEA_FIELD_LONGITUDE]);
}

static void longitude_end_handler(struct nmea *const n) {
  n->state.scratch <<= NMEA_FXP_FRACTIONALS[NMEA_FIELD_LONGITUDE];
  n->data.longitude =
      n->state.scratch + (ufxp_get_val(&n->state.fxpse.fxp) / 60);
}

static void latitude_start_handler(struct nmea *const n) {
  ufxp_init(&n->state.fxpse.fxp);
  n->state.char_count = 0;
  n->state.scratch = 0;
}

void latitude_char_handler(struct nmea *const n,
                           const char c) { /* must hold +-90 Q8.56 */
  lon_lat_char_handler(n, c, 2, NMEA_FXP_FRACTIONALS[NMEA_FIELD_LATITUDE]);
}

void latitude_end_handler(struct nmea *const n) {
  n->state.scratch <<= NMEA_FXP_FRACTIONALS[NMEA_FIELD_LATITUDE];
  n->data.latitude =
      n->state.scratch + (ufxp_get_val(&n->state.fxpse.fxp) / 60);
}

static void latitude_dir_char_handler(struct nmea *const n, const char c) {
  if ((c == 'S') && (n->data.latitude > 0)) {
    n->data.latitude = -n->data.latitude;
  } else if ((c == 'N') && (n->data.latitude < 0)) {
    n->data.latitude = -n->data.latitude;
  }
  n->state.received &= ~(((nmea_field_bitmap_t)1) << NMEA_FIELD_LATITUDE_DIR);
}

static void longitude_dir_char_handler(struct nmea *const n, const char c) {
  if ((c == 'W') && (n->data.longitude > 0)) {
    n->data.longitude = -n->data.longitude;
  } else if ((c == 'E') && (n->data.longitude < 0)) {
    n->data.longitude = -n->data.longitude;
  }
  n->state.received &= ~(((nmea_field_bitmap_t)1) << NMEA_FIELD_LONGITUDE_DIR);
}

static void fix_quality_end_handler(struct nmea *const n) {
  n->data.fix_quality = ufxp_get_val(&n->state.fxpse.fxp);
}

static void satellites_tracked_end_handler(struct nmea *const n) {
  n->data.satellites_tracked = ufxp_get_val(&n->state.fxpse.fxp);
}

static void satellites_in_view_end_handler(struct nmea *const n) {
  n->data.satellites_in_view = ufxp_get_val(&n->state.fxpse.fxp);
}

static void altitude_char_handler(struct nmea *const n, const char c) {
  fxp_from_ascii(&n->state.fxpse.fxp, c,
                 NMEA_FXP_FRACTIONALS[NMEA_FIELD_ALTITUDE]);
}

static void altitude_end_handler(struct nmea *const n) {
  n->data.altitude = fxp_get_val(&n->state.fxpse.fxp);
}

static void geoid_height_char_handler(struct nmea *const n, const char c) {
  fxp_from_ascii(&n->state.fxpse.fxp, c,
                 NMEA_FXP_FRACTIONALS[NMEA_FIELD_GEOID_HEIGHT]);
}

static void geoid_height_end_handler(struct nmea *const n) {
  n->data.geoid_height = fxp_get_val(&n->state.fxpse.fxp);
}

static void fix_3d_end_handler(struct nmea *const n) {
  n->data.fix_3d = ufxp_get_val(&n->state.fxpse.fxp);
}

static void prns_tracked_end_handler(struct nmea *const n) {
  unsigned char prn = ufxp_get_val(&n->state.fxpse.fxp);
  unsigned char gsa_satellite_index = n->state.gsa_satellite_index;
  if ((prn != 0) && (gsa_satellite_index < NMEA_MAX_PRNS_TRACKED)) {
    /* prns are 1 relative */
    n->data.prns_tracked[gsa_satellite_index] = prn;
    ++n->state.gsa_satellite_index;
    n->state.gsa_satellite_count = n->state.gsa_satellite_index;
  }
}

static void pdop_char_handler(struct nmea *const n, const char c) {
  ufxp_from_ascii(&n->state.fxpse.fxp, c,
                  NMEA_FXP_FRACTIONALS[NMEA_FIELD_PDOP]);
}

static void pdop_end_handler(struct nmea *const n) {
  n->data.pdop = ufxp_get_val(&n->state.fxpse.fxp);
}

static void hdop_char_handler(struct nmea *const n, const char c) {
  ufxp_from_ascii(&n->state.fxpse.fxp, c,
                  NMEA_FXP_FRACTIONALS[NMEA_FIELD_HDOP]);
}

static void hdop_end_handler(struct nmea *const n) {
  n->data.hdop = ufxp_get_val(&n->state.fxpse.fxp);
}

static void vdop_char_handler(struct nmea *const n, const char c) {
  ufxp_from_ascii(&n->state.fxpse.fxp, c,
                  NMEA_FXP_FRACTIONALS[NMEA_FIELD_VDOP]);
}

static void vdop_end_handler(struct nmea *const n) {
  n->data.vdop = ufxp_get_val(&n->state.fxpse.fxp);
}

static void gll_active_char_handler(struct nmea *const n, const char c) {
  n->data.gll_active = (c == 'A') ? NMEA_ACTIVE : NMEA_VOID;
  n->state.received &= ~(((nmea_field_bitmap_t)1) << NMEA_FIELD_GLL_ACTIVE);
}

static void rmc_active_char_handler(struct nmea *const n, const char c) {
  n->data.rmc_active = (c == 'A') ? NMEA_ACTIVE : NMEA_VOID;
  n->state.received &= ~(((nmea_field_bitmap_t)1) << NMEA_FIELD_RMC_ACTIVE);
}

static void speed_char_handler(struct nmea *const n, const char c) {
  ufxp_from_ascii(&n->state.fxpse.fxp, c,
                  NMEA_FXP_FRACTIONALS[NMEA_FIELD_SPEED]);
}

static void speed_end_handler(struct nmea *const n) {
  n->data.speed = ufxp_get_val(&n->state.fxpse.fxp);
}

static void time_char_handler(struct nmea *const n, const char c) {
  unsigned long long int digit = c - '0';
  switch (n->state.char_count) {
  case 0:
    n->state.scratch = digit * 10 * SECONDS_IN_HOUR;
    break;
  case 1:
    n->state.scratch += digit * SECONDS_IN_HOUR;
    break;
  case 2:
    n->state.scratch += digit * 10 * SECONDS_IN_MINUTE;
    break;
  case 3:
    n->state.scratch += digit * SECONDS_IN_MINUTE;
    break;
  case 4:
    n->state.scratch += digit * 10;
    break;
  case 5:
    n->state.scratch += digit;
    break;
  default:
    break;
  }
  ++n->state.char_count;
}

static void time_end_handler(struct nmea *const n) {
  /* floor time to last day boundary */
  n->data.time = (n->data.time / SECONDS_IN_DAY) * SECONDS_IN_DAY;
  n->data.time += n->state.scratch;
}

static void date_char_handler(struct nmea *const n, const char c) {
  static const unsigned char month_days[] = {31, 28, 31, 30, 31, 30,
                                             31, 31, 30, 31, 30};

  unsigned long long int digit = c - '0';
  switch (n->state.char_count) {
  case 0: /* days e10 */
    n->state.scratch = digit * 10 * SECONDS_IN_DAY;
    break;
  case 1: /* days */
    n->state.scratch += digit * SECONDS_IN_DAY;
    n->state.scratch -= SECONDS_IN_DAY;
    break;
  case 2: /* months e10 */
    n->state.fxpse.se.scratch[0] = digit * 10;
    break;
  case 3:                                  /* months */
    n->state.fxpse.se.scratch[0] += digit; /* one relative */
    --n->state.fxpse.se.scratch[0]; /* how many months worth of days to add */
    if (n->state.fxpse.se.scratch[0] >
        (sizeof(month_days) / sizeof(month_days[0]))) {
      /* months > 12 */
      break;
    }
    unsigned char i = 0;
    while (i < n->state.fxpse.se.scratch[0]) {
      n->state.scratch += (month_days[i] * SECONDS_IN_DAY);
      ++i;
    }
    break;
  case 4: /* years e10 */
    n->state.fxpse.se.scratch[1] = digit * 10;
    break;
  case 5: /* years */
    n->state.fxpse.se.scratch[1] += digit;
    /* this also adds the leap day for this year, removed below if earlier than
     * Mar */
    n->state.scratch += (n->state.fxpse.se.scratch[1] * SECONDS_IN_DAY * 365) +
                        SECONDS_IN_DAY * (n->state.fxpse.se.scratch[1] / 4);
    if ((NMEA_CENTURY % 400) == 0) {
      /* century is leap year */
      n->state.scratch += SECONDS_IN_DAY;
    }
    /* remove 29th Feb for this year if earlier than Mar */
    if (((n->state.fxpse.se.scratch[1] % 4) == 0) &&
        (n->state.fxpse.se.scratch[0] < 3)) {
      n->state.scratch -= SECONDS_IN_DAY;
    }
    break;
  default:
    break;
  }
  ++n->state.char_count;
}

static void date_end_handler(struct nmea *const n) {
  n->data.time =
      (n->data.time % SECONDS_IN_DAY) + NMEA_CENTURY_OFFSET + n->state.scratch;
}

static void magnetic_variation_char_handler(struct nmea *const n,
                                            const char c) {
  fxp_from_ascii(&n->state.fxpse.fxp, c,
                 NMEA_FXP_FRACTIONALS[NMEA_FIELD_MAGNETIC_VARIATION]);
}

static void magnetic_variation_end_handler(struct nmea *const n) {
  n->data.magnetic_variation = fxp_get_val(&n->state.fxpse.fxp);
}

static void magnetic_variation_dir_char_handler(struct nmea *const n,
                                                const char c) {
  if ((c == 'W') && (n->data.magnetic_variation > 0)) {
    n->data.magnetic_variation = -n->data.magnetic_variation;
  } else if ((c == 'E') && (n->data.magnetic_variation < 0)) {
    n->data.magnetic_variation = -n->data.magnetic_variation;
  }
  n->state.received &=
      ~(((nmea_field_bitmap_t)1) << NMEA_FIELD_MAGNETIC_VARIATION_DIR);
}

static void gsv_sentences_total_end_handler(struct nmea *const n) {
  n->state.gsv_sentences_total = ufxp_get_val(&n->state.fxpse.fxp);
}

static void sentence_no_end_handler(struct nmea *const n) {
  n->state.gsv_sentence_no = ufxp_get_val(&n->state.fxpse.fxp);
}

static void prn_end_handler(struct nmea *const n) {
  unsigned int gsv_satellite_index = n->state.gsv_satellite_index;
  if (gsv_satellite_index >= NMEA_MAX_SATS) {
    return;
  }
  n->data.sats[gsv_satellite_index].prn = ufxp_get_val(&n->state.fxpse.fxp);
}

static void azimuth_end_handler(struct nmea *const n) {
  unsigned int gsv_satellite_index = n->state.gsv_satellite_index;
  if (gsv_satellite_index >= NMEA_MAX_SATS) {
    return;
  }
  n->data.sats[gsv_satellite_index].azimuth = ufxp_get_val(&n->state.fxpse.fxp);
}

static void elevation_end_handler(struct nmea *const n) {
  unsigned int gsv_satellite_index = n->state.gsv_satellite_index;
  if (gsv_satellite_index >= NMEA_MAX_SATS) {
    return;
  }
  n->data.sats[gsv_satellite_index].elevation =
      ufxp_get_val(&n->state.fxpse.fxp);
}

static void snr_end_handler(struct nmea *const n) {
  unsigned int gsv_satellite_index = n->state.gsv_satellite_index;
  if (gsv_satellite_index >= NMEA_MAX_SATS) {
    return;
  }
  n->data.sats[gsv_satellite_index].snr = ufxp_get_val(&n->state.fxpse.fxp);
  ++n->state.gsv_satellite_index;
  n->state.gsv_satellite_count = n->state.gsv_satellite_index;
}

static void true_track_char_handler(struct nmea *n, const char c) {
  fxp_from_ascii(&n->state.fxpse.fxp, c,
                 NMEA_FXP_FRACTIONALS[NMEA_FIELD_TRUE_TRACK]);
}

static void true_track_end_handler(struct nmea *n) {
  n->data.true_track = fxp_get_val(&n->state.fxpse.fxp);
}

static void magnetic_track_char_handler(struct nmea *n, const char c) {
  fxp_from_ascii(&n->state.fxpse.fxp, c,
                 NMEA_FXP_FRACTIONALS[NMEA_FIELD_MAGNETIC_TRACK]);
}

static void magnetic_track_end_handler(struct nmea *n) {
  n->data.magnetic_track = fxp_get_val(&n->state.fxpse.fxp);
}

static void header_start_handler(struct nmea *const n);
static void header_char_handler(struct nmea *const n, const char c);
static void header_end_handler(struct nmea *const n);

static const struct nmea_field_handlers HANDLER_LUT[] = {
    [NMEA_FIELD_LONGITUDE] =
        {
            &longitude_start_handler,
            &longitude_char_handler,
            &longitude_end_handler,
        },
    [NMEA_FIELD_LONGITUDE_DIR] = {&ignore_handler, &longitude_dir_char_handler,
                                  &ignore_handler},
    [NMEA_FIELD_LATITUDE_DIR] = {&ignore_handler, &latitude_dir_char_handler,
                                 &ignore_handler},
    [NMEA_FIELD_LATITUDE] = {&latitude_start_handler, &latitude_char_handler,
                             &latitude_end_handler},
    [NMEA_FIELD_FIX_QUALITY] = {&ufxp_init_start_handler, &ui_char_handler,
                                &fix_quality_end_handler},
    [NMEA_FIELD_SATELLITES_TRACKED] = {&ufxp_init_start_handler,
                                       &ui_char_handler,
                                       &satellites_tracked_end_handler},
    [NMEA_FIELD_SATELLITES_IN_VIEW] = {&ufxp_init_start_handler,
                                       &ui_char_handler,
                                       &satellites_in_view_end_handler},
    [NMEA_FIELD_ALTITUDE] = {&fxp_init_start_handler, &altitude_char_handler,
                             &altitude_end_handler},
    [NMEA_FIELD_GEOID_HEIGHT] = {&fxp_init_start_handler,
                                 &geoid_height_char_handler,
                                 &geoid_height_end_handler},
    [NMEA_FIELD_FIX_3D] = {&ufxp_init_start_handler, &ui_char_handler,
                           &fix_3d_end_handler},
    [NMEA_FIELD_PRNS_TRACKED] = {&ufxp_init_start_handler, &ui_char_handler,
                                 &prns_tracked_end_handler},
    [NMEA_FIELD_PRN] = {&ufxp_init_start_handler, &ui_char_handler,
                        &prn_end_handler},
    [NMEA_FIELD_PDOP] = {&ufxp_init_start_handler, &pdop_char_handler,
                         &pdop_end_handler},
    [NMEA_FIELD_HDOP] = {&ufxp_init_start_handler, &hdop_char_handler,
                         &hdop_end_handler},
    [NMEA_FIELD_VDOP] = {&ufxp_init_start_handler, &vdop_char_handler,
                         &vdop_end_handler},
    [NMEA_FIELD_GLL_ACTIVE] = {&ignore_handler, &gll_active_char_handler,
                               &ignore_handler},
    [NMEA_FIELD_RMC_ACTIVE] = {&ignore_handler, &rmc_active_char_handler,
                               &ignore_handler},
    [NMEA_FIELD_SPEED] = {&ufxp_init_start_handler, &speed_char_handler,
                          &speed_end_handler},
    [NMEA_FIELD_TIME] = {&reset_cc_start_handler, &time_char_handler,
                         &time_end_handler},
    [NMEA_FIELD_DATE] = {&reset_cc_start_handler, &date_char_handler,
                         &date_end_handler},
    [NMEA_FIELD_MAGNETIC_VARIATION] = {&fxp_init_start_handler,
                                       &magnetic_variation_char_handler,
                                       &magnetic_variation_end_handler},
    [NMEA_FIELD_MAGNETIC_VARIATION_DIR] = {&ignore_handler,
                                           &magnetic_variation_dir_char_handler,
                                           &ignore_handler},
    [NMEA_FIELD_IGNORE] = {&ignore_handler, &ignore_char_handler,
                           &ignore_handler},
    [NMEA_FIELD_GSV_SENTENCES_TOTAL] = {&ufxp_init_start_handler,
                                        &ui_char_handler,
                                        &gsv_sentences_total_end_handler},
    [NMEA_FIELD_SENTENCE_NO] = {&ufxp_init_start_handler, &ui_char_handler,
                                &sentence_no_end_handler},
    [NMEA_FIELD_AZIMUTH] = {&ufxp_init_start_handler, &ui_char_handler,
                            &azimuth_end_handler},
    [NMEA_FIELD_ELEVATION] = {&ufxp_init_start_handler, &ui_char_handler,
                              &elevation_end_handler},
    [NMEA_FIELD_SNR] = {&ufxp_init_start_handler, &ui_char_handler,
                        &snr_end_handler},
    [NMEA_FIELD_TRUE_TRACK] = {fxp_init_start_handler, true_track_char_handler,
                               true_track_end_handler},
    [NMEA_FIELD_MAGNETIC_TRACK] = {fxp_init_start_handler,
                                   magnetic_track_char_handler,
                                   magnetic_track_end_handler},
    [NMEA_FIELD_HEADER] = {&header_start_handler, &header_char_handler,
                           &header_end_handler}};

static const enum nmea_fields GGA_FIELDS[] = {NMEA_FIELD_TIME,
                                              NMEA_FIELD_LATITUDE,
                                              NMEA_FIELD_LATITUDE_DIR,
                                              NMEA_FIELD_LONGITUDE,
                                              NMEA_FIELD_LONGITUDE_DIR,
                                              NMEA_FIELD_FIX_QUALITY,
                                              NMEA_FIELD_SATELLITES_TRACKED,
                                              NMEA_FIELD_HDOP,
                                              NMEA_FIELD_ALTITUDE,
                                              NMEA_FIELD_IGNORE,
                                              NMEA_FIELD_GEOID_HEIGHT,
                                              NMEA_FIELD_IGNORE,
                                              NMEA_FIELD_IGNORE,
                                              NMEA_FIELD_IGNORE};

static const enum nmea_fields GLL_FIELDS[] = {
    NMEA_FIELD_LATITUDE,      NMEA_FIELD_LATITUDE_DIR, NMEA_FIELD_LONGITUDE,
    NMEA_FIELD_LONGITUDE_DIR, NMEA_FIELD_TIME,         NMEA_FIELD_GLL_ACTIVE,
    NMEA_FIELD_IGNORE};

static const enum nmea_fields GSA_FIELDS[] = {
    NMEA_FIELD_IGNORE,       NMEA_FIELD_FIX_3D,       NMEA_FIELD_PRNS_TRACKED,
    NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED,
    NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED,
    NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED,
    NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PRNS_TRACKED, NMEA_FIELD_PDOP,
    NMEA_FIELD_HDOP,         NMEA_FIELD_VDOP};

static const enum nmea_fields GSV_FIELDS[] = {NMEA_FIELD_GSV_SENTENCES_TOTAL,
                                              NMEA_FIELD_SENTENCE_NO,
                                              NMEA_FIELD_SATELLITES_IN_VIEW,
                                              NMEA_FIELD_PRN,
                                              NMEA_FIELD_ELEVATION,
                                              NMEA_FIELD_AZIMUTH,
                                              NMEA_FIELD_SNR,
                                              NMEA_FIELD_PRN,
                                              NMEA_FIELD_ELEVATION,
                                              NMEA_FIELD_AZIMUTH,
                                              NMEA_FIELD_SNR,
                                              NMEA_FIELD_PRN,
                                              NMEA_FIELD_ELEVATION,
                                              NMEA_FIELD_AZIMUTH,
                                              NMEA_FIELD_SNR,
                                              NMEA_FIELD_PRN,
                                              NMEA_FIELD_ELEVATION,
                                              NMEA_FIELD_AZIMUTH,
                                              NMEA_FIELD_SNR};

static const enum nmea_fields RMC_FIELDS[] = {NMEA_FIELD_TIME,
                                              NMEA_FIELD_RMC_ACTIVE,
                                              NMEA_FIELD_LATITUDE,
                                              NMEA_FIELD_LATITUDE_DIR,
                                              NMEA_FIELD_LONGITUDE,
                                              NMEA_FIELD_LONGITUDE_DIR,
                                              NMEA_FIELD_SPEED,
                                              NMEA_FIELD_TRUE_TRACK,
                                              NMEA_FIELD_DATE,
                                              NMEA_FIELD_MAGNETIC_VARIATION,
                                              NMEA_FIELD_MAGNETIC_VARIATION_DIR,
                                              NMEA_FIELD_IGNORE};

static const enum nmea_fields VTG_FIELDS[] = {
    NMEA_FIELD_TRUE_TRACK, NMEA_FIELD_IGNORE, NMEA_FIELD_MAGNETIC_TRACK,
    NMEA_FIELD_IGNORE,     NMEA_FIELD_SPEED,  NMEA_FIELD_IGNORE,
    NMEA_FIELD_IGNORE,     NMEA_FIELD_IGNORE, NMEA_FIELD_IGNORE};

static void gsa_start_handler(struct nmea *const n) {
  n->state.gsa_satellite_index = 0;
}

static void gsv_end_handler(struct nmea *const n) {
  /* update GSV_sentences_received */
  unsigned int gsv_sentence_no = n->state.gsv_sentence_no;
  if (gsv_sentence_no > (sizeof(nmea_gsv_bitmap_t) * CHAR_BIT)) {
    return;
  }
  n->state.gsv_sentences_received |= ((nmea_gsv_bitmap_t)1)
                                     << (gsv_sentence_no - 1);
  if (n->state.gsv_sentences_received ==
      ((((nmea_gsv_bitmap_t)1) << n->state.gsv_sentences_total) - 1)) {
    /* all GSV messages received */
    n->state.received |= n->state.field_bitmap;
    n->state.gsv_sentences_received = 0;
    n->state.gsv_satellite_index = 0;
  }
}

static void gsv_checksum_fail_handler(struct nmea *const n) {
  n->state.gsv_sentences_received = 0;
  n->state.gsv_satellite_index = 0;
}

static void generic_end_handler(struct nmea *const n) {
  n->state.received |= n->state.field_bitmap;
}

static const struct nmea_sentence_format SENTENCE_LUT[] = {
    {GGA_FIELDS, ignore_handler, generic_end_handler, ignore_handler, "GPGGA",
     sizeof(GGA_FIELDS) / sizeof(GGA_FIELDS[0])},
    {GLL_FIELDS, ignore_handler, generic_end_handler, ignore_handler, "GPGLL",
     sizeof(GLL_FIELDS) / sizeof(GLL_FIELDS[0])},
    {GSA_FIELDS, gsa_start_handler, generic_end_handler, ignore_handler,
     "GPGSA", sizeof(GSA_FIELDS) / sizeof(GSA_FIELDS[0])},
    {GSV_FIELDS, ignore_handler, gsv_end_handler, gsv_checksum_fail_handler,
     "GPGSV", sizeof(GSV_FIELDS) / sizeof(GSV_FIELDS[0])},
    {RMC_FIELDS, ignore_handler, generic_end_handler, ignore_handler, "GPRMC",
     sizeof(RMC_FIELDS) / sizeof(RMC_FIELDS[0])},
    {VTG_FIELDS, ignore_handler, generic_end_handler, ignore_handler, "GPVTG",
     sizeof(VTG_FIELDS) / sizeof(VTG_FIELDS[0])}};

static const struct nmea_sentence_format IGNORE_SENTENCE = {
    0, ignore_handler, ignore_handler, ignore_handler, "", 0};

static void field_update(struct nmea *n) {
  unsigned char comma_count = n->state.comma_count;
  if (comma_count < n->state.sentence->length) {
    const enum nmea_fields field = n->state.sentence->fields[comma_count];
    n->state.field = field;
    n->state.field_handlers = &HANDLER_LUT[field];
  } else {
    n->state.field = NMEA_FIELD_IGNORE;
    n->state.field_handlers = &HANDLER_LUT[NMEA_FIELD_IGNORE];
  }
}

static void header_start_handler(struct nmea *const n) {
  n->state.char_count = 0;
  n->state.comma_count = 0;
  n->state.checksum = 0;
  n->state.checksum_recording = 0;
  n->state.sentence_bitmap =
      (((nmea_sentence_bitmap_t)1)
       << (sizeof(SENTENCE_LUT) / sizeof(SENTENCE_LUT[0]))) -
      1;
}

static void header_char_handler(struct nmea *const n, const char c) {
  unsigned char i = 0;
  while (i < (sizeof(SENTENCE_LUT) / sizeof(SENTENCE_LUT[0]))) {
    if (SENTENCE_LUT[i].head[n->state.char_count] != c) {
      n->state.sentence_bitmap &= ~(((nmea_sentence_bitmap_t)1) << i);
    }
    ++i;
  }
  ++n->state.char_count;
}

static void header_end_handler(struct nmea *const n) {
  nmea_sentence_bitmap_t oh = n->state.sentence_bitmap;
  /* check that no more than one sentence identified */
  if (((oh & (oh - 1)) == 0) && (oh != 0)) {
    n->state.sentence = &SENTENCE_LUT[0];
    while (oh != 1) {
      ++n->state.sentence;
      oh >>= 1;
    }
    n->state.sentence->start_handler(n);
  } else {
    n->state.sentence = &IGNORE_SENTENCE;
  }
}

static unsigned char hex_to_nibble(const char c) {
  return (c <= '9') ? c - '0' : (c - 'A') + 10;
}

void nmea_parse(struct nmea *const n, const char c) {
  if (c == '$') {
    /* reset */
    n->state.field_bitmap = 0;
    n->state.field_handlers = &HANDLER_LUT[NMEA_FIELD_HEADER];
    n->state.field_handlers->start_handler(n);
  } else if (n->state.checksum_recording != 0) {
    if (n->state.char_count == 0) {
      n->state.fxpse.fxp.val = hex_to_nibble(c) << 4;
      ++n->state.char_count;
    } else {
      if ((n->state.fxpse.fxp.val | hex_to_nibble(c)) == n->state.checksum) {
        /* checksum pass */
        n->state.sentence->end_handler(n);
      } else {
        /* checksum fail */
        n->state.sentence->checksum_fail_handler(n);
      }
      n->state.checksum_recording = 0;
    }
  } else if (c == ',') {
    n->state.received &= ~(((nmea_field_bitmap_t)1) << n->state.field);
    n->state.field_handlers->end_handler(n);
    field_update(n);
    ++n->state.comma_count;
    n->state.field_handlers->start_handler(n);
    n->state.checksum ^= c;
  } else if (c == '*') {
    n->state.received &= ~(((nmea_field_bitmap_t)1) << n->state.field);
    n->state.field_handlers->end_handler(n);
    n->state.checksum_recording = 1;
    n->state.char_count = 0;
  } else {
    /* call handler */
    n->state.field_bitmap |= ((nmea_field_bitmap_t)1) << n->state.field;
    n->state.field_handlers->char_handler(n, c);
    n->state.checksum ^= c;
  }
}

char nmea_fields_ready(struct nmea *const n, const nmea_field_bitmap_t fields) {
  if ((n->state.received & fields) == fields) {
    n->state.received ^= fields;
    return 1;
  }
  return 0;
}

void nmea_init(struct nmea *const n) {
  memset(n, 0, sizeof(*n));
  n->state.sentence = &IGNORE_SENTENCE;
  n->state.field = NMEA_FIELD_IGNORE;
  n->state.field_handlers = &HANDLER_LUT[NMEA_FIELD_IGNORE];
}
