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

#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Takes 2 args: The serial port pathname, and the baud rate\n");
    return -1;
  }

  unsigned long int baudrate = strtol(argv[2], 0, 0);

  struct termios options;
  int fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    printf("Failed to open serial port\n");
    return -1;
  }

  if (tcgetattr(fd, &options) != 0) {
    printf("Failed to get tty attributes\n");
    return -1;
  }

  cfsetispeed(&options, (speed_t)baudrate);
  /* cfsetospeed(&options, (speed_t)baudrate); */

  options.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
  options.c_cflag |= CS8 | CLOCAL | CREAD;
  options.c_iflag &= ~(IXON | IXOFF | IXANY | ICANON | ECHO | ECHOE | ISIG);
  /* options.c_oflag &= ~OPOST; */
  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 1;

  if (tcflush(fd, TCIFLUSH) == -1) {
    printf("Failed to flush tty\n");
    return -1;
  }
  if (tcsetattr(fd, TCSANOW, &options) != 0) {
    printf("Failed to set tty attributes\n");
    return -1;
  }

  struct nmea n;
  nmea_init(&n);

  const nmea_field_bitmap_t fields =
      NMEA_FIELD_LONGITUDE_MASK | NMEA_FIELD_LATITUDE_MASK;

  char c;
  while (1) {
    int r = read(fd, &c, 1);
    if (r == -1) {
      printf("Failed to read from serial port\n");
      return -1;
    } else if (r == 1) {
      nmea_parse(&n, c);
      if (nmea_fields_ready(&n, fields) == 1) {
        printf("tim:\t%llu\n", n.data.time);
        printf("lat:\t%f\n",
               nmea_fxp_to_double(n.data.latitude, NMEA_FIELD_LATITUDE));
        printf("lon:\t%f\n",
               nmea_fxp_to_double(n.data.longitude, NMEA_FIELD_LONGITUDE));
        printf("hdp:\t%f\n", nmea_fxp_to_double(n.data.hdop, NMEA_FIELD_HDOP));
        printf("pdp:\t%f\n", nmea_fxp_to_double(n.data.pdop, NMEA_FIELD_PDOP));
        printf("vdp:\t%f\n", nmea_fxp_to_double(n.data.vdop, NMEA_FIELD_VDOP));
        printf("spd:\t%f\n",
               nmea_fxp_to_double(n.data.speed, NMEA_FIELD_SPEED));
        printf("tt:\t%f\n",
               nmea_fxp_to_double(n.data.true_track, NMEA_FIELD_TRUE_TRACK));
        printf("mt:\t%f\n", nmea_fxp_to_double(n.data.magnetic_track,
                                               NMEA_FIELD_MAGNETIC_TRACK));
        printf("mv:\t%f\n", nmea_fxp_to_double(n.data.magnetic_variation,
                                               NMEA_FIELD_MAGNETIC_VARIATION));
        printf("alt:\t%f\n",
               nmea_fxp_to_double(n.data.altitude, NMEA_FIELD_ALTITUDE));
        printf("gh:\t%f\n", nmea_fxp_to_double(n.data.geoid_height,
                                               NMEA_FIELD_GEOID_HEIGHT));
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
          while ((j < n.data.satellites_tracked) &&
                 (j < NMEA_MAX_PRNS_TRACKED)) {
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
  }
  close(fd);
  return 0;
}
