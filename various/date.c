// date.c - calculate days since 1900-01-01 and day of week
//
// No license - public domain

#define FOUR_HUNDRED_YEARS (400L * 365L + 24L * 4L + 1L)
                                           /* number of days in 400 years */


static char month_days[13]={0,31,28,31,30, 31, 30, 31, 31, 30, 31, 30, 31};
static int  total_days[13]={0,31,59,90,120,151,181,212,243,273,304,334,365};
static char leap_days[13] ={0,31,29,31,30, 31, 30, 31, 31, 30, 31, 30, 31};
static int  total_leap[13]={0,31,60,91,121,152,182,213,244,274,305,335,366};

typedef struct __date        /* used by data conversion utilities   */
{
  short year;             /* equivalent structure to 'dosdate_t' */
  char month, day;

};





long days(struct __date *d0)       /* convert date to # days since 1/1/1900 */
{                                  /* beginning with 1 for 1/1/1900         */
                                   /* because 1/1/1900 was a MONDAY!        */
long l;
int n_years;
long adjustment;


  adjustment = 0;              /* initially don't adjust for century */

  n_years = d0.year - 1900;    /* # of years since 1900 */

  while(n_years<0)
  {
    n_years += 400;                       /* add 400 years */

    adjustment -= FOUR_HUNDRED_YEARS;    /* number of days in 400 years */
  }

  while(n_years >=400)
  {
    n_years -= 400;

    adjustment += FOUR_HUNDRED_YEARS;
  }

    /*    CALCULATE THE TOTAL NUMBER OF DAYS UP TO 1/1 THIS YEAR    */
    /* terms:  # of days + # of "Feb/29"s - # of non-leap centuries */

  l = 365L * n_years + ((n_years>4)?((n_years - 1) >> 2):0)
     + ((n_years>100)?(1 - (n_years - 1)/ 100):0);


  if(((d0.year % 400)==0 || (d0.year % 100)!=0) && (d0.year % 4)==0)
  {
                         /** LEAP YEAR **/

    l += total_leap[d0.month - 1];    /* month-to-date totals (leap) */
  }
  else
  {
                        /** NORMAL YEAR **/

    l += total_days[d0.month - 1];    /* a slightly faster method! */
  }


  l += d0.day;        /* l is now updated for exact # of days! */

  return(l + adjustment);         /* thdya thdya thdya that's all folks! */
}



int DayOfWeek(struct __date * d)
{
long dayz;

  dayz = days(d);

  if(dayz == 0x80000000L) /* an error */
  {
    return(-1);
  }
  else if(days>=0)
  {
    return((int)(dayz % 7));
  }
  else
  {
    return(7 + (int)(dayz % 7));
  }

}


