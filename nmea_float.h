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

#ifndef NMEA_FLOAT_H
#define NMEA_FLOAT_H

#include "nmea.h"

static inline double nmea_ufxp_to_double(const unsigned long long int fxp,
                                         const enum nmea_fields field) {
  return (double)fxp /
         (double)(((unsigned long long int)1) << NMEA_FXP_FRACTIONALS[field]);
}

static inline double nmea_fxp_to_double(const long long int fxp,
                                        const enum nmea_fields field) {
  return (double)fxp /
         (double)(((unsigned long long int)1) << NMEA_FXP_FRACTIONALS[field]);
}

#endif
