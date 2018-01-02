/***************************************************************************/
/*                                                                         */
/*      HUGEMEM.H - (c) 1992 by R. E. Frazier - all rights reserved        */
/*                                                                         */
/*            special HUGE MEMORY copy,compare,swap,sort,etc.              */
/* (these functions will handle structures that cross segment boundaries)  */
/*                                                                         */
/***************************************************************************/


    /** LINK IN 'HUGEMEM.LIB' for small, medium model 'C' programs **/
   /** LINK IN 'HUGEMEML.LIB' for large, compact model 'C' programs **/



/*=========================================================================*/
/*   _lqsort() - function to do a 'qsort' on far/huge data buffer          */
/*                                                                         */
/*=========================================================================*/

       /* QUICK-SORT algorithm - uses median of 3 optimization   */
       /* 'compare' function is actually passed 'HUGE' pointers  */
       /* but LPSTR is used to simplify declarations.            */
       /* 'compare' function should take into consideration that */
       /* structures may cross segment boundaries if the total   */
       /* size of the "sorted" block exceeds 64k.                */
       /*                                                        */
       /* passing a NULL for 'compare' will use a *default* fun- */
       /* ction which essentially calls '_hmemcmp()'.            */


typedef int (FAR _cdecl LQSORT_COMPARE_CALLBACK)(LPSTR lp1, LPSTR lp2);
typedef int (FAR _cdecl LPLQSORT_COMPARE_CALLBACK)(LPSTR lp1, LPSTR lp2);

void FAR _cdecl _lqsort(char _huge *base, DWORD num, DWORD width,
                        LPLQSORT_COMPARE_CALLBACK compare);


/*=========================================================================*/
/* _hmemcpy() - similar to '_fmemcpy()', except that it uses huge pointers */
/*              and will cross segment boundaries when needed.  The start  */
/*              addresses need not have the same offset, nor is the data   */
/*              limited to a single segment.  Segment boundaries are there-*/
/*              for 'transparent' and can be essentially ignored.          */
/*=========================================================================*/


void FAR _cdecl _hmemcpy(char _huge *hpNewData, char _huge *hpOldData,
                            DWORD maxcount);


/*=========================================================================*/
/* _hmemset() - similar to '_fmemset()', except that it uses huge pointers */
/*              and will cross segment boundaries when needed.             */
/*=========================================================================*/


void FAR _cdecl _hmemset(char _huge *hpData, WORD value, DWORD maxcount);



/*=========================================================================*/
/*  _hmemswap() - for use with sort operations, mostly.  Similar to above  */
/*                in that it swaps two HUGE blocks of memory, which may    */
/*                each cross segment boundaries at any point.              */
/*=========================================================================*/

void FAR _cdecl _hmemswap(char _huge *hpData1, char _huge *hpData2,
                          DWORD maxcount);


/*=========================================================================*/
/* _hmemcmp() - like '_hmemcpy()' (above) compares memory between two huge */
/*              memory blocks, not necessarily starting at the same point  */
/*              and possibly extending between segments.                   */
/*=========================================================================*/


int FAR _cdecl _hmemcmp(char _huge *hpData1, char _huge *hpData2,
                         DWORD maxcount);
