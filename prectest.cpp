/* prectest.cpp: demonstrates/shows results from precession funcs

Copyright (C) 2010, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.    */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "watdefs.h"
#include "afuncs.h"
#include "date.h"
#include "lunar.h"         /* for obliquity( ) prototype */

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923
static const double arcsec_to_radians = PI / (180. * 3600.);
static const double marcsec_to_radians = PI / (180. * 3600.e+3);

/* Verify the final results of the 'eop_prec.cpp' functions for computing
precise Earth orientation against matrices generated by the USNO's on-line
earth orientation matrix calculator at

http://maia.usno.navy.mil/
https://hpiers.obspm.fr/eop-pc/index.php?index=matrice

(look down the first page for "earth orientation matrix calculator").
Agreement is within about a centimeter.  I don't know why it isn't exact. I
rather wish it were;  I have no practical need for accuracy better than a
few meters,  but an "exact" match to the USNO EOP calculator would be a
reassuring unit test. */

double default_td_minus_ut( const double jd);      /* delta_t.cpp */

int main( const int argc, const char **argv)
{
   const double utc = get_time_from_string( 0., (argc < 2 ? "now" : argv[1]),
                       FULL_CTIME_YMD, NULL);
   const double tdt = utc + td_minus_utc( utc) / seconds_per_day;
   int i, pass, loop;
   const double J2000 = 2451545.;  /* JD 2451545 = 2000 Jan 1.5 */
   double year = 2000. + (tdt - J2000) / 365.25;
   const char *eop_filename = (argc < 3 ? "finals.all" : argv[2]);
   int eop_rval = load_earth_orientation_params( eop_filename, NULL);
   earth_orientation_params eo_params;
   char tbuff[80];

   if( eop_rval <= 0)
      printf( "Problem loading EOPs from '%s':  rval %d\n",
                                    eop_filename, eop_rval);
   else
      {
      int eop_range[3];

      load_earth_orientation_params( NULL, eop_range);
      for( i = 0; i < 3; i++)
         {
         const char *format[3] = { "EOPs start MJD %d = %s\n",
                      "EOPs run to MJD %d = %s (including predictions)\n",
                      "EOPs run to MJD %d = %s (without extrapolation)\n" };

         full_ctime( tbuff, 2400000.5 + (double)eop_range[i],
                  FULL_CTIME_DATE_ONLY | FULL_CTIME_YMD);
         printf( format[i], eop_range[i], tbuff);
         }
      }
   full_ctime( tbuff, utc, FULL_CTIME_YMD);
   printf( "For JD %f = %s UTC\n", utc, tbuff);
   eop_rval = get_earth_orientation_params( tdt, &eo_params, 31);
   printf( "Polar motion: %f x, %f y (arcseconds)\n",
               eo_params.dX / arcsec_to_radians,
               eo_params.dY / arcsec_to_radians);
   printf( "TDT - UT1 = %f seconds\n", eo_params.tdt_minus_ut1);
   printf( "TDT - UT1 = %f seconds (from 'standard' function)\n",
               default_td_minus_ut( tdt));
   printf( "dPsi %f; dEps %f (milliarcseconds)\n",
               eo_params.dPsi / marcsec_to_radians,
               eo_params.dEps / marcsec_to_radians);
   printf( "UT1 - UTC = %f\n", td_minus_utc( tdt) - eo_params.tdt_minus_ut1);
   if( eop_rval)
      printf( "Couldn't get some/all EOPs for that date : %d\n", eop_rval);

   for( loop = (eop_rval ? 1 : 0); loop < 2; loop++)
      {
      printf( "With%s EOPs :\n", (loop ? "out" : ""));
      for( pass = 0; pass < 3; pass++)
         {
         double matrix[9];
         const char *labels[3] = { "IAU1976 precession,  no nutation:", "With IAU1980 nutation:",
                        "Full orientation" };

         printf( "%s\n", labels[pass]);
         switch( pass)
            {
            case 0:
               setup_precession( matrix, 2000., year);
               break;
            case 1:
               setup_precession_with_nutation( matrix, year);
               break;
            case 2:
               setup_precession_with_nutation_eops( matrix, year);
               break;
            }
         for( i = 0; i < 9; i++)            /* print out the precession matrix */
            printf( "%15.11f%s", matrix[i], (i % 3 == 2) ? "\n" : " ");
         }
      if( !loop)
         load_earth_orientation_params( NULL, NULL);
      }
   return( 0);
}

/*
phred@phreddie:~/lunar$ ./prectest 2016aug1.25 ~/gps_ephem/finals.all
EOPs run to MJD 59328 = 2021 Apr 24 (including predictions)
EOPs run to MJD 58955 = 2020 Apr 16 (without extrapolation)
For JD 2457601.750000 = 2016 Aug  1  6:00:00 UTC
Polar motion: 0.213166 x, 0.449051 y (arcseconds)
TDT - UT1 = 68.409540 seconds
TDT - UT1 = 68.353379 seconds (from 'standard' function)
dPsi -103.915177; dEps -13.507789 (milliarcseconds)
UT1 - UTC = -0.225540
      ('without' and 'with' swapped below to put 'with' next to
      the results from obspm.fr,  to make comparison easier)
Without EOPs :
IAU1976 precession,  no nutation:
  0.99999182608  -0.00370830410  -0.00161128711
  0.00370830409   0.99999312421  -0.00000299239
  0.00161128713  -0.00000298278   0.99999870187
With IAU1980 nutation:
  0.99999183661  -0.00370830411  -0.00160474335
  0.00370837498   0.99999312311   0.00004119141
  0.00160457957  -0.00004714206   0.99999871155
Full orientation
  0.76528979113   0.64368478411  -0.00119762528
 -0.64368389829   0.76529072677   0.00106892079
  0.00160457957  -0.00004714206   0.99999871155
With EOPs :
IAU1976 precession,  no nutation:
  0.99999182608  -0.00370830410  -0.00161128711
  0.00370830409   0.99999312421  -0.00000299239
  0.00161128713  -0.00000298278   0.99999870187
With IAU1980 nutation:
  0.99999183661  -0.00370830411  -0.00160474335
  0.00370837498   0.99999312311   0.00004119141
  0.00160457957  -0.00004714206   0.99999871155
Full orientation
  0.76529242939   0.64368164969  -0.00119640100
 -0.64368076755   0.76529336318   0.00106665925
  0.00160218674  -0.00004620594   0.99999871543
From https://hpiers.obspm.fr/eop-pc/index.php?index=matrice
  0.765292427840  0.643681651530 -0.001196400550
 -0.643680769394  0.765293361626  0.001066660273
  0.001602187045 -0.000046207004  0.999998715430

      Note that a unit change in the last digit from the software
      corresponds to about 0.06 millimeters on the earth's surface
      (10^-11 earth radii,  with the earth's radius being about 6378140 m.)
*/
