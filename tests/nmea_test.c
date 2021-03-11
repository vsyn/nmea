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

#include "../nmea_float.h"

#include <stdio.h>
#include <string.h>

static void test_parse_string(struct nmea *const n, const char *s) {
  while (*s) {
    nmea_parse(n, *s);
    ++s;
  }
}

static int double_comp(const double a, const double b, const double thresh) {
  double diff = a - b;
  if (diff < 0) {
    diff = -diff;
  }
  return (diff < thresh) ? 0 : 1;
}

int test_rmc(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_TIME_MASK | NMEA_FIELD_RMC_ACTIVE_MASK |
      NMEA_FIELD_LATITUDE_MASK | NMEA_FIELD_LATITUDE_DIR_MASK |
      NMEA_FIELD_LONGITUDE_MASK | NMEA_FIELD_LONGITUDE_DIR_MASK |
      NMEA_FIELD_SPEED_MASK | NMEA_FIELD_TRUE_TRACK_MASK | NMEA_FIELD_DATE_MASK;

  char s[] = "$GPRMC,175456.00,A,5104.34432,N,00147.29814,W,34.075,213.73,"
             "080321,,,A*47";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: RMC did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: RMC set extra field flags %lx\n", n.state.received);
    return -1;
  }

  if (n.data.time != 0x604664f0) {
    printf("ERR: RMC time incorrect, received: 0x%llx, expected: 0x604664f0.\n",
           n.data.time);
    return -1;
  }

  if (n.data.rmc_active != NMEA_ACTIVE) {
    printf("ERR: RMC active incorrect, received: %u, expected: 1\n",
           n.data.rmc_active);
    return -1;
  }

  double lat = nmea_fxp_to_double(n.data.latitude, NMEA_FIELD_LATITUDE);
  double cor = 51.072405;
  if (double_comp(lat, cor, 0.000001) != 0) {
    printf("ERR: RMC latitude incorrect, received: %f, expected: %f\n", lat,
           cor);
    return -1;
  }

  double lon = nmea_fxp_to_double(n.data.longitude, NMEA_FIELD_LONGITUDE);
  cor = -1.788302;
  if (double_comp(lon, cor, 0.000001) != 0) {
    printf("ERR: RMC longitude incorrect, received: %f, expected: %f\n", lon,
           cor);
    return -1;
  }

  double speed = nmea_ufxp_to_double(n.data.speed, NMEA_FIELD_SPEED);
  cor = 34.074982;
  if (double_comp(speed, cor, 0.001) != 0) {
    printf("ERR: RMC speed incorrect, received: %f, expected: %f\n", speed,
           cor);
    return -1;
  }

  double track = nmea_ufxp_to_double(n.data.true_track, NMEA_FIELD_TRUE_TRACK);
  cor = 213.729996;
  if (double_comp(track, cor, 0.001) != 0) {
    printf("ERR: RMC true track incorrect, received: %f, expected: %f\n", track,
           cor);
    return -1;
  }

  double variation = nmea_ufxp_to_double(n.data.magnetic_variation,
                                         NMEA_FIELD_MAGNETIC_VARIATION);
  cor = 0;
  if (double_comp(variation, cor, 0.001) != 0) {
    printf("ERR: RMC magnetic variation incorrect, received: %f, "
           "expected: %f\n",
           variation, cor);
    return -1;
  }

  return 0;
}

int test_vtg(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_TRUE_TRACK_MASK | NMEA_FIELD_SPEED_MASK;

  char s[] = "$GPVTG,213.73,T,,M,34.075,N,63.106,K,A*3E";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: VTG did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: VTG set extra field flags %lx\n", n.state.received);
    return -1;
  }

  double true_track =
      nmea_fxp_to_double(n.data.true_track, NMEA_FIELD_TRUE_TRACK);
  double cor = 213.73;
  if (double_comp(true_track, cor, 0.001) != 0) {
    printf("ERR: VTG true track incorrect, received: %f, expected: %f\n",
           true_track, cor);
    return -1;
  }

  double mag_track =
      nmea_fxp_to_double(n.data.magnetic_track, NMEA_FIELD_MAGNETIC_TRACK);
  cor = 0;
  if (double_comp(mag_track, cor, 0.001) != 0) {
    printf("ERR: VTG magnetic track incorrect, received: %f, expected: %f\n",
           mag_track, cor);
    return -1;
  }

  double speed = nmea_ufxp_to_double(n.data.speed, NMEA_FIELD_SPEED);
  cor = 34.075;
  if (double_comp(speed, cor, 0.001) != 0) {
    printf("ERR: VTG speed incorrect, received: %f, expected: %f\n", speed,
           cor);
    return -1;
  }

  return 0;
}

int test_gga(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_TIME_MASK | NMEA_FIELD_LATITUDE_MASK |
      NMEA_FIELD_LATITUDE_DIR_MASK | NMEA_FIELD_LONGITUDE_MASK |
      NMEA_FIELD_LONGITUDE_DIR_MASK | NMEA_FIELD_FIX_QUALITY_MASK |
      NMEA_FIELD_SATELLITES_TRACKED_MASK | NMEA_FIELD_HDOP_MASK |
      NMEA_FIELD_ALTITUDE_MASK | NMEA_FIELD_GEOID_HEIGHT_MASK;

  char s[] = "$GPGGA,175456.00,5104.34432,N,00147.29814,W,1,03,2.88,61.8,M,"
             "47.5,M,,*74";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: GGA did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: GGA set extra field flags %lx\n", n.state.received);
    return -1;
  }

  if (n.data.time != 0xfbf0) {
    printf("ERR: GGA time incorrect, received: 0x%llx, expected: 0xfbf0.\n",
           n.data.time);
    return -1;
  }

  double lat = nmea_fxp_to_double(n.data.latitude, NMEA_FIELD_LATITUDE);
  double cor = 51.072405;
  if (double_comp(lat, cor, 0.000001) != 0) {
    printf("ERR: GGA latitude incorrect, received: %f, expected: %f\n", lat,
           cor);
    return -1;
  }

  double lon = nmea_fxp_to_double(n.data.longitude, NMEA_FIELD_LONGITUDE);
  cor = -1.788302;
  if (double_comp(lon, cor, 0.000001) != 0) {
    printf("ERR: GGA longitude incorrect, received: %f, expected: %f\n", lon,
           cor);
    return -1;
  }

  if (n.data.fix_quality != 1) {
    printf("ERR: GGA fix quality incorrect, received: 0x%u, expected: 1\n",
           n.data.fix_quality);
    return -1;
  }

  if (n.data.satellites_tracked != 3) {
    printf(
        "ERR: GGA satellites tracked incorrect, received: 0x%u, expected: 3\n",
        n.data.satellites_tracked);
    return -1;
  }

  double hdop = nmea_ufxp_to_double(n.data.hdop, NMEA_FIELD_HDOP);
  cor = 2.88;
  if (double_comp(hdop, cor, 0.001) != 0) {
    printf("ERR: GGA hdop incorrect, received: %f, expected: %f\n", hdop, cor);
    return -1;
  }

  double altitude = nmea_fxp_to_double(n.data.altitude, NMEA_FIELD_ALTITUDE);
  cor = 61.8;
  if (double_comp(altitude, cor, 0.001) != 0) {
    printf("ERR: GGA altitude incorrect, received: %f, expected: %f\n",
           altitude, cor);
    return -1;
  }

  double geoid_height =
      nmea_fxp_to_double(n.data.geoid_height, NMEA_FIELD_GEOID_HEIGHT);
  cor = 47.5;
  if (double_comp(geoid_height, cor, 0.001) != 0) {
    printf("ERR: GGA geoid height, received: %f, expected: %f\n", geoid_height,
           cor);
    return -1;
  }

  return 0;
}

int test_gsa(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_FIX_3D_MASK | NMEA_FIELD_PRNS_TRACKED_MASK |
      NMEA_FIELD_PDOP_MASK | NMEA_FIELD_HDOP_MASK | NMEA_FIELD_VDOP_MASK;

  char s[] = "$GPGSA,A,2,18,16,23,,,,,,,,,,3.05,2.88,1.00*09";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: GSA did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: GSA set extra field flags %lx\n", n.state.received);
    return -1;
  }

  if (n.data.fix_3d != NMEA_FIX_2D) {
    printf("ERR: GSA 3d fix incorrect, received: %u, expected: 2\n",
           n.data.fix_3d);
    return -1;
  }

  unsigned char cor_prns[] = {18, 16, 23};
  unsigned char i = 0;
  while (i < (sizeof(cor_prns) / sizeof(cor_prns[0]))) {
    if (n.data.prns_tracked[i] != cor_prns[i]) {
      printf("ERR: GSA prn %u incorrect, received: %u, expected: %u\n", i,
             n.data.prns_tracked[i], cor_prns[i]);
      return -1;
    }
    ++i;
  }

  double pdop = nmea_ufxp_to_double(n.data.pdop, NMEA_FIELD_PDOP);
  double cor = 3.05;
  if (double_comp(pdop, cor, 0.001) != 0) {
    printf("ERR: GSA pdop incorrect, received: %f, expected: %f\n", pdop, cor);
    return -1;
  }

  double hdop = nmea_ufxp_to_double(n.data.hdop, NMEA_FIELD_HDOP);
  cor = 2.88;
  if (double_comp(hdop, cor, 0.001) != 0) {
    printf("ERR: GSA hdop incorrect, received: %f, expected: %f\n", hdop, cor);
    return -1;
  }

  double vdop = nmea_ufxp_to_double(n.data.vdop, NMEA_FIELD_VDOP);
  cor = 1.0;
  if (double_comp(vdop, cor, 0.001) != 0) {
    printf("ERR: GSA vdop incorrect, received: %f, expected: %f\n", vdop, cor);
    return -1;
  }

  return 0;
}

int test_gsv(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_GSV_SENTENCES_TOTAL_MASK | NMEA_FIELD_SENTENCE_NO_MASK |
      NMEA_FIELD_SATELLITES_IN_VIEW_MASK | NMEA_FIELD_PRN_MASK |
      NMEA_FIELD_ELEVATION_MASK | NMEA_FIELD_AZIMUTH_MASK | NMEA_FIELD_SNR_MASK;

  char s[] = "$GPGSV,2,1,08,05,02,020,,07,,,33,16,76,272,33,18,58,069,31*48"
             "$GPGSV,2,2,08,20,,,24,23,28,121,20,26,,,31,27,46,274,32*7F";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: GSV did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: GSV set extra field flags %lx\n", n.state.received);
    return -1;
  }

  if (n.state.gsv_sentences_total != 2) {
    printf("ERR: GSV sentences total incorrect, received: %u, expected: 4\n",
           n.state.gsv_sentences_total);
    return -1;
  }

  if (n.data.satellites_in_view != 8) {
    printf("ERR: GSV sentences total incorrect, received: %u, expected: 14\n",
           n.data.satellites_in_view);
    return -1;
  }

  struct nmea_sat cor_sats[] = {
      {.prn = 05, .elevation = 2, .azimuth = 20, .snr = 0},
      {.prn = 7, .elevation = 0, .azimuth = 0, .snr = 33},
      {.prn = 16, .elevation = 76, .azimuth = 272, .snr = 33},
      {.prn = 18, .elevation = 58, .azimuth = 69, .snr = 31},
      {.prn = 20, .elevation = 0, .azimuth = 0, .snr = 24},
      {.prn = 23, .elevation = 28, .azimuth = 121, .snr = 20},
      {.prn = 26, .elevation = 0, .azimuth = 0, .snr = 31},
      {.prn = 27, .elevation = 46, .azimuth = 274, .snr = 32}};
  unsigned char i = 0;
  while (i < (sizeof(cor_sats) / sizeof(cor_sats[0]))) {
    if ((n.data.sats[i].prn != cor_sats[i].prn) ||
        (n.data.sats[i].elevation != cor_sats[i].elevation) ||
        (n.data.sats[i].azimuth != cor_sats[i].azimuth) ||
        (n.data.sats[i].snr != cor_sats[i].snr)) {
      printf("ERR: GSV sat %u incorrect\n", i);
      return -1;
    }
    ++i;
  }

  return 0;
}

int test_gll(void) {
  nmea_field_bitmap_t fields =
      NMEA_FIELD_LATITUDE_MASK | NMEA_FIELD_LATITUDE_DIR_MASK |
      NMEA_FIELD_LONGITUDE_MASK | NMEA_FIELD_LONGITUDE_DIR_MASK |
      NMEA_FIELD_TIME_MASK | NMEA_FIELD_GLL_ACTIVE_MASK;

  char s[] = "$GPGLL,5104.34432,N,00147.29814,W,175456.00,A,A*79";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (nmea_fields_ready(&n, fields) != 1) {
    printf("ERR: GLL did not set the required field flags %lx\n",
           n.state.received);
    return -1;
  }

  if ((n.state.received & ~NMEA_FIELD_IGNORE_MASK) != 0) {
    printf("ERR: GLL set extra field flags %lx\n", n.state.received);
    return -1;
  }

  double lat = nmea_fxp_to_double(n.data.latitude, NMEA_FIELD_LATITUDE);
  double cor = 51.072405;
  if (double_comp(lat, cor, 0.000001) != 0) {
    printf("ERR: GLL latitude incorrect, received: %f, expected: %f\n", lat,
           cor);
    return -1;
  }

  double lon = nmea_fxp_to_double(n.data.longitude, NMEA_FIELD_LONGITUDE);
  cor = -1.788302;
  if (double_comp(lon, cor, 0.000001) != 0) {
    printf("ERR: GLL longitude incorrect, received: %f, expected: %f\n", lon,
           cor);
    return -1;
  }

  if (n.data.time != 0xfbf0) {
    printf("ERR: GLL time incorrect, received: 0x%llx, expected: 0xfbf0.\n",
           n.data.time);
    return -1;
  }

  if (n.data.gll_active != NMEA_ACTIVE) {
    printf("ERR: GLL active incorrect, received: %u, expected: 1\n",
           n.data.gll_active);
    return -1;
  }

  return 0;
}

/* test unsupported sentence to make sure it is ignored */
int test_txt(void) {
  char s[] = "$GPTXT,01,01,02,ANTSTATUS=OK*3B";

  struct nmea n;
  nmea_init(&n);

  test_parse_string(&n, s);

  if (n.state.received != 0) {
    printf("ERR: TXT not ignored: %lx\n", n.state.received);
    return -1;
  }
  return 0;
}

static const unsigned long int TEST_RANDOM_REPS = 100000;
static const int TEST_RANDOM_SEED = 1;

/* test random data, there is always a *really* small chance of this failing
 * if a valid NMEA message is randomly generated
 */
int test_random(void) {
  struct nmea n;
  nmea_init(&n);

  srand(TEST_RANDOM_SEED);

  unsigned long int i = 0;
  while (i < TEST_RANDOM_REPS) {
    char c = rand() % 256;
    nmea_parse(&n, c);
    ++i;
  }

  if (n.state.received != 0) {
    printf("ERR: random data not ignored\n");
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int rc;

  rc = test_rmc();
  if (rc != 0) {
    return rc;
  }

  rc = test_vtg();
  if (rc != 0) {
    return rc;
  }

  rc = test_gga();
  if (rc != 0) {
    return rc;
  }

  rc = test_gsa();
  if (rc != 0) {
    return rc;
  }

  rc = test_gsv();
  if (rc != 0) {
    return rc;
  }

  rc = test_gll();
  if (rc != 0) {
    return rc;
  }

  rc = test_txt();
  if (rc != 0) {
    return rc;
  }

  rc = test_random();
  if (rc != 0) {
    return rc;
  }

  return 0;
}
