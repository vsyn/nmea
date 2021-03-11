/* Copyright 2021 Julian Ingram
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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Takes 1 arg: The file to read from\n");
    return -1;
  }

  struct nmea n;
  nmea_init(&n);

  FILE *fd = fopen(argv[1], "rb");
  if (fd == 0) {
    printf("Failed to open file\n");
    return -1;
  }

  const nmea_field_bitmap_t fields =
      NMEA_FIELD_LONGITUDE_MASK | NMEA_FIELD_LATITUDE_MASK;

  while (1) {
    int c = getc(fd);
    if (c == EOF) {
      break;
    }
    nmea_parse(&n, (char)c);
    if (nmea_fields_ready(&n, fields) == 1) {
      printf("tim:\t%llu\n", n.data.time);
      printf("lat:\t%f\n",
             nmea_fxp_to_double(n.data.latitude, NMEA_FIELD_LATITUDE));
      printf("lon:\t%f\n",
             nmea_fxp_to_double(n.data.longitude, NMEA_FIELD_LONGITUDE));
      printf("hdp:\t%f\n", nmea_fxp_to_double(n.data.hdop, NMEA_FIELD_HDOP));
      printf("pdp:\t%f\n", nmea_fxp_to_double(n.data.pdop, NMEA_FIELD_PDOP));
      printf("vdp:\t%f\n", nmea_fxp_to_double(n.data.vdop, NMEA_FIELD_VDOP));
      printf("spd:\t%f\n", nmea_fxp_to_double(n.data.speed, NMEA_FIELD_SPEED));
      printf("tt:\t%f\n",
             nmea_fxp_to_double(n.data.true_track, NMEA_FIELD_TRUE_TRACK));
      printf("mt:\t%f\n", nmea_fxp_to_double(n.data.magnetic_track,
                                             NMEA_FIELD_MAGNETIC_TRACK));
      printf("mv:\t%f\n", nmea_fxp_to_double(n.data.magnetic_variation,
                                             NMEA_FIELD_MAGNETIC_VARIATION));
      printf("alt:\t%f\n",
             nmea_fxp_to_double(n.data.altitude, NMEA_FIELD_ALTITUDE));
      printf("gh:\t%f\n",
             nmea_fxp_to_double(n.data.geoid_height, NMEA_FIELD_GEOID_HEIGHT));
      printf("st:\t%u\n", n.data.satellites_tracked);
      printf("siv:\t%u\n", n.data.satellites_in_view);
      printf("fq:\t%u\n", n.data.fix_quality);
      printf("3d:\t%u\n", n.data.fix_3d);
      printf("ga:\t%u\n", n.data.gll_active);
      printf("ra:\t%u\n", n.data.rmc_active);
      unsigned int i = 0;
      while ((i < n.data.satellites_in_view) && (i < NMEA_MAX_SATS)) {
        struct nmea_sat *sat = &n.data.sats[i];
        printf("sat:\t%u\taz:\t%d\tel:\t%d\tsnr:\t%d", sat->prn, sat->azimuth,
               sat->elevation, sat->snr);
        unsigned int j = 0;
        while ((j < n.data.satellites_tracked) && (j < NMEA_MAX_PRNS_TRACKED)) {
          if (sat->prn == n.data.prns_tracked[j]) {
            printf("\ttkd");
            break;
          }
          ++j;
        }
        printf("\n");
        ++i;
      }
      printf("\n");
    }
  }
  fclose(fd);
  return 0;
}
