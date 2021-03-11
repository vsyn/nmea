# NMEA Parser #

A configurable NMEA parseing library supporting GGA, GLL, GSA, GSV, VTG and RMC
out of the box. Designed to minimise buffering and to break up the parsing task
as much as possible.

Uses fixed point representation, but provides a floating point wrapper in
nmea_float.h.

Tested using a u-blox NEO-6M.

Function descriptions in nmea.h, examples available in the examples dir.
