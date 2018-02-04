/***************************************************************************/
/*                                                                         */
/*      'fixpoint' class and member functions (all inline functions!)      */
/*                                                                         */
/***************************************************************************/



class fixpoint
{
   public:

      fixpoint() __far                 { CheckFor386(); lValue = 0; }  // default constructor
      fixpoint(const int i) __far      { CheckFor386(); lValue = (long)i << 8; }
      fixpoint(const long i) __far     { CheckFor386(); lValue = (long)i << 8; }
      fixpoint(const DWORD i) __far    { CheckFor386(); lValue = (long)i << 8; }
      fixpoint(const double d) __far   { CheckFor386(); lValue = (long)(d * 256.0 + .5); }

      fixpoint(const fixpoint __far &fp) __far { lValue = fp.lValue; }

      operator=(const fixpoint __far &fp) __far { this->lValue = fp.lValue; return(*this); }
      operator=(const int i) __far    { this->lValue = (long)i << 8; return(*this); }
      operator=(const long i) __far   { this->lValue = (long)i << 8; return(*this); }
      operator=(const DWORD i) __far  { this->lValue = (long)i << 8; return(*this); }
      operator=(const double d) __far { this->lValue = (long)(d * 256.0 + .5); return(*this); }

      operator int() const __far            { return((int)(lValue >> 8)); }
      operator unsigned short() const __far { return((WORD)(lValue >> 8)); }
      operator double() const __far         { return((double)lValue * (1.0 / 256.0)); }

      fixpoint operator++() __far     { this->lValue += 256; return(*this); }
      fixpoint operator++(int) __far  {fixpoint fp = *this; this->lValue += 256; return(fp); }

      fixpoint operator--() __far     { this->lValue -= 256; return(*this); }
      fixpoint operator--(int) __far  {fixpoint fp = *this; this->lValue -= 25; return(fp); }


      // overload the unary '-' operator

      friend fixpoint operator-(fixpoint fp);


      friend fixpoint __far &operator+=(fixpoint __far &fp1, fixpoint fp2);
      friend fixpoint __far &operator+=(fixpoint __far &fp1, int i);
      friend fixpoint __far &operator+=(fixpoint __far &fp1, double d);
      friend fixpoint __far &operator-=(fixpoint __far &fp1, fixpoint fp2);
      friend fixpoint __far &operator-=(fixpoint __far &fp1, int i);
      friend fixpoint __far &operator-=(fixpoint __far &fp1, double d);
      friend fixpoint __far &operator*=(fixpoint __far &fp1, fixpoint fp2);
      friend fixpoint __far &operator*=(fixpoint __far &fp1, int i);
      friend fixpoint __far &operator*=(fixpoint __far &fp1, double d);
      friend fixpoint __far &operator/=(fixpoint __far &fp1, fixpoint fp2);
      friend fixpoint __far &operator/=(fixpoint __far &fp1, int i);
      friend fixpoint __far &operator/=(fixpoint __far &fp1, double d);

      friend fixpoint operator+(fixpoint fp1, fixpoint fp2);
      friend fixpoint operator+(fixpoint fp1, int i);
      friend fixpoint operator+(int i, fixpoint fp2);
      friend fixpoint operator+(fixpoint fp1, double d);
      friend fixpoint operator+(double d, fixpoint fp2);
      friend fixpoint operator-(fixpoint fp1, fixpoint fp2);
      friend fixpoint operator-(fixpoint fp1, int i);
      friend fixpoint operator-(int i, fixpoint fp2);
      friend fixpoint operator-(fixpoint fp1, double d);
      friend fixpoint operator-(double d, fixpoint fp2);
      friend fixpoint operator*(fixpoint fp1, fixpoint fp2);
      friend fixpoint operator*(fixpoint fp1, int i);
      friend fixpoint operator*(int i, fixpoint fp2);
      friend fixpoint operator*(fixpoint fp1, double d);
      friend fixpoint operator*(double d, fixpoint fp2);
      friend fixpoint operator/(fixpoint fp1, fixpoint fp2);
      friend fixpoint operator/(fixpoint fp1, int i);
      friend fixpoint operator/(int i, fixpoint fp2);
      friend fixpoint operator/(fixpoint fp1, double d);
      friend fixpoint operator/(double d, fixpoint fp2);

      friend int operator<(fixpoint , fixpoint );
      friend int operator<(fixpoint fp1, int i);
      friend int operator<(fixpoint fp1, unsigned u);
      friend int operator<(fixpoint fp1, double d);
      friend int operator<(int i, fixpoint fp2);
      friend int operator<(unsigned u, fixpoint fp2);
      friend int operator<(double d, fixpoint fp2);

      friend int operator>(fixpoint , fixpoint );
      friend int operator>(fixpoint fp1, int i);
      friend int operator>(fixpoint fp1, unsigned u);
      friend int operator>(fixpoint fp1, double d);
      friend int operator>(int i, fixpoint fp2);
      friend int operator>(unsigned u, fixpoint fp2);
      friend int operator>(double d, fixpoint fp2);

      friend int operator<=(fixpoint , fixpoint );
      friend int operator<=(fixpoint fp1, int i);
      friend int operator<=(fixpoint fp1, unsigned u);
      friend int operator<=(fixpoint fp1, double d);
      friend int operator<=(int i, fixpoint fp2);
      friend int operator<=(unsigned u, fixpoint fp2);
      friend int operator<=(double d, fixpoint fp2);

      friend int operator>=(fixpoint , fixpoint );
      friend int operator>=(fixpoint fp1, int i);
      friend int operator>=(fixpoint fp1, unsigned u);
      friend int operator>=(fixpoint fp1, double d);
      friend int operator>=(int i, fixpoint fp2);
      friend int operator>=(unsigned u, fixpoint fp2);
      friend int operator>=(double d, fixpoint fp2);

      friend int operator==(fixpoint , fixpoint );
      friend int operator==(fixpoint fp1, int i);
      friend int operator==(fixpoint fp1, unsigned u);
      friend int operator==(fixpoint fp1, double d);
      friend int operator==(int i, fixpoint fp2);
      friend int operator==(unsigned u, fixpoint fp2);
      friend int operator==(double d, fixpoint fp2);

      friend int operator!=(fixpoint , fixpoint );
      friend int operator!=(fixpoint fp1, int i);
      friend int operator!=(fixpoint fp1, unsigned u);
      friend int operator!=(fixpoint fp1, double d);
      friend int operator!=(int i, fixpoint fp2);
      friend int operator!=(unsigned u, fixpoint fp2);
      friend int operator!=(double d, fixpoint fp2);

      friend fixpoint fpsqrt(fixpoint __far &);  // GLOBAL 'sqrt' proc, overridden


   private:

      long lValue;                  // value as SCALED LONG;

      static BOOL Is386;
      static BOOL Is287;

      static void CheckFor287(void) { if(Is287 == (BOOL)-1) Is287 = !(GetWinFlags() & WF_80x87); }
      static void CheckFor386(void) { CheckFor287(); if(Is386 == (BOOL)-1) Is386 = !(GetWinFlags() & WF_CPU286); }

};


BOOL fixpoint::Is386 = -1;
BOOL fixpoint::Is287 = -1;


#ifdef DEBUG
static char temp_buf[128];
#endif /* DEBUG */


static inline fixpoint operator-(fixpoint fp)
{
   fp.lValue = -fp.lValue;

   return(fp);
}
static inline fixpoint __far &operator+=(fixpoint __far &fp1, fixpoint fp2)
{
   fp1.lValue += fp2.lValue;

   return(fp1);
}
static inline fixpoint __far &operator+=(fixpoint __far &fp1, int i)
{
fixpoint fp2(i);

   return(fp1 += fp2);
}
static inline fixpoint __far &operator+=(fixpoint __far &fp1, double d)
{
fixpoint fp2(d);

   return(fp1 += fp2);
}


static inline fixpoint __far &operator-=(fixpoint __far &fp1, fixpoint fp2)
{
   fp1.lValue -= fp2.lValue;

   return(fp1);
}
static inline fixpoint __far &operator-=(fixpoint __far &fp1, int i)
{
fixpoint fp2(i);

   return(fp1 -= fp2);
}
static inline fixpoint __far &operator-=(fixpoint __far &fp1, double d)
{
fixpoint fp2(d);

   return(fp1 -= fp2);
}

static inline fixpoint __far &operator*=(fixpoint __far &fp1, fixpoint fp2a)
{
long l0, l1, l2, l3;
fixpoint fp2;
BOOL sign = 0;


   // TO AVOID PROBLEMS we need to multiply POSITIVE values, so figure
   // out ahead of time whether the result should be negative or positive
   // and adjust the sign of each term to be POSITIVE!


   if(fp1.lValue & 0x80000000L)
   {
      sign = !0;
      fp1 = -(fp1);
   }


   fp2 = fp2a;

   if(fp2.lValue & 0x80000000L)
   {
      sign = !sign;

      fp2 = -(fp2);
   }


   if(!fixpoint::Is386)
   {
      l0 = fp1.lValue;    // value1 * 256
      l1 = fp2.lValue;    // value2 * 256

      _asm mov ax, WORD PTR l0
      _asm mul WORD PTR l1
      _asm mov WORD PTR l2, ax
      _asm mov WORD PTR l2 + 2, dx

      _asm mov ax, WORD PTR l0 + 2
      _asm mul WORD PTR l1
      _asm add WORD PTR l2 + 2, ax
      _asm adc dx, 0
      _asm mov WORD PTR l3, dx

      _asm mov ax, WORD PTR l0
      _asm mul WORD PTR l1 + 2
      _asm add WORD PTR l2 + 2, ax
      _asm adc WORD PTR l3, dx

      _asm mov ax, WORD PTR l0 + 2
      _asm mul WORD PTR l1 + 2
      _asm add WORD PTR l3, ax

      if(LOWORD(l2) & 0x8000)  // roundoff!!
      {
         _asm inc WORD PTR l2 + 2
         _asm adc WORD PTR l3, 0
      }

      fp1.lValue = (long)((DWORD)((l2 >> 8) & 0xffffffL) +
                          (DWORD)((l3 & 0xffL) << 24));
   }
   else
   {
      l0 = fp1.lValue;    // value1 * 256
      l1 = fp2.lValue;    // value2 * 256

      _asm _emit(0x66) _asm mov ax, WORD PTR l0
      _asm _emit(0x66) _asm mul WORD PTR l1
      _asm _emit(0x66) _asm mov WORD PTR l3, dx
      _asm _emit(0x66) _asm mov WORD PTR l2, ax

      if(LOWORD(l2) & 0x8000)  // roundoff!!
      {
         _asm inc WORD PTR l2 + 2
         _asm adc WORD PTR l3, 0
      }

      fp1.lValue = (long)((DWORD)((l2 >> 8) & 0xffffffL) +
                          (DWORD)((l3 & 0xffL) << 24));
   }

   if(sign) fp1 = -(fp1);

   return(fp1);
}
static inline fixpoint __far &operator*=(fixpoint __far &fp1, int i)
{
fixpoint fp2(i);

   return(fp1 *= fp2);
}
static inline fixpoint __far &operator*=(fixpoint __far &fp1, double d)
{
fixpoint fp2(d);

   return(fp1 *= fp2);
}

static inline fixpoint __far &operator/=(fixpoint __far &fp1, fixpoint fp2)
{
double rv;

#ifdef DEBUG
   sprintf(temp_buf, "%f / %f = ", (double)fp1, (double)fp2);
   OutputDebugString(temp_buf);
#endif /* DEBUG */

   if(fp2.lValue == 0)
   {
      if(fp1.lValue & 0x80000000L)
      {
         fp1.lValue = 0xffffffffL;
      }
      else
      {
         fp1.lValue = 0x7fffffffL;
      }
   }
   else if(!fixpoint::Is386)
   {
      rv = ((double)fp1) / ((double)fp2); // does the divide operation
      fp1 = fixpoint(rv); // does the conversion from double to 'fixpoint'
   }
   else
   {
    long l0, l1, l2;
    BOOL bSign=0;

      l0 = fp1.lValue;
      l1 = fp2.lValue;

      if(l0<0)
      {
         l0 = -l0;
         bSign = !0;
      }
      if(l1<0)
      {
         l1 = -l1;
         bSign = !bSign;
      }

      _asm _emit(0x66) _asm mov ax, WORD PTR l0
      _asm _emit(0x66) _asm shl ax, 8
      _asm _emit(0x66) _asm xor dx, dx
                       _asm mov dl, BYTE PTR l0+3

      _asm _emit(0x66) _asm div WORD PTR l1;

      _asm _emit(0x66) _asm mov WORD PTR l0, ax
      _asm _emit(0x66) _asm mov WORD PTR l2, dx


      l2 = l2 << 1;  // double value of remainder

      if(l2 >= l1) l0++;  // round-off


      if(bSign) l0 = -l0;

      fp1.lValue = l0;
   }

#ifdef DEBUG
   sprintf(temp_buf, "%f\r\n", (double)fp1);
   OutputDebugString(temp_buf);
#endif /* DEBUG */

   return(fp1);
}
static inline fixpoint __far &operator/=(fixpoint __far &fp1, int i)
{
fixpoint fp2(i);

   return(fp1 /= fp2);
}
static inline fixpoint __far &operator/=(fixpoint __far &fp1, double d)
{
fixpoint fp2(d);

   return(fp1 /= fp2);
}


static inline fixpoint operator+(fixpoint fp1, fixpoint fp2)
{
fixpoint fp = fp1;

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(fixpoint fp1, int i)
{
fixpoint fp = fp1, fp2(i);

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(int i, fixpoint fp2)
{
fixpoint fp(i);

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(fixpoint fp1, unsigned u)
{
fixpoint fp = fp1, fp2 = (long)u;

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(unsigned u, fixpoint fp2)
{
fixpoint fp = (long)u;

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(fixpoint fp1, double d)
{
fixpoint fp = fp1, fp2(d);

   fp += fp2;
   return(fp);
}
static inline fixpoint operator+(double d, fixpoint fp2)
{
fixpoint fp(d);

   fp += fp2;
   return(fp);
}

static inline fixpoint operator-(fixpoint fp1, fixpoint fp2)
{
fixpoint fp = fp1;

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(fixpoint fp1, int i)
{
fixpoint fp = fp1, fp2(i);

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(int i, fixpoint fp2)
{
fixpoint fp(i);

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(fixpoint fp1, unsigned u)
{
fixpoint fp = fp1, fp2 = (long)u;

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(unsigned u, fixpoint fp2)
{
fixpoint fp = (long)u;

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(fixpoint fp1, double d)
{
fixpoint fp = fp1, fp2(d);

   fp -= fp2;
   return(fp);
}
static inline fixpoint operator-(double d, fixpoint fp2)
{
fixpoint fp(d);

   fp -= fp2;
   return(fp);
}

static inline fixpoint operator*(fixpoint fp1, fixpoint fp2)
{
fixpoint fp = fp1;

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(fixpoint fp1, int i)
{
fixpoint fp = fp1, fp2(i);

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(fixpoint fp1, unsigned u)
{
fixpoint fp = fp1, fp2 = (long)u;

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(unsigned u, fixpoint fp2)
{
fixpoint fp = (long)u;

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(int i, fixpoint fp2)
{
fixpoint fp(i);

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(fixpoint fp1, double d)
{
fixpoint fp = fp1, fp2(d);

   fp *= fp2;
   return(fp);
}
static inline fixpoint operator*(double d, fixpoint fp2)
{
fixpoint fp(d);

   fp *= fp2;
   return(fp);
}

static inline fixpoint operator/(fixpoint fp1, fixpoint fp2)
{
fixpoint fp = fp1;

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(fixpoint fp1, int i)
{
fixpoint fp = fp1, fp2(i);

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(int i, fixpoint fp2)
{
fixpoint fp(i);

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(fixpoint fp1, unsigned u)
{
fixpoint fp = fp1, fp2 = (long)u;

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(unsigned u, fixpoint fp2)
{
fixpoint fp = (long)u;

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(fixpoint fp1, double d)
{
fixpoint fp = fp1, fp2(d);

   fp /= fp2;
   return(fp);
}
static inline fixpoint operator/(double d, fixpoint fp2)
{
fixpoint fp(d);

   fp /= fp2;
   return(fp);
}



static inline int operator==(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue == fp2.lValue);
}
static inline int operator==(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue == fp2.lValue);
}



static inline int operator!=(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue != fp2.lValue);
}
static inline int operator!=(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue != fp2.lValue);
}



static inline int operator>(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue > fp2.lValue);
}
static inline int operator>(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue > fp2.lValue);
}



static inline int operator<(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue < fp2.lValue);
}
static inline int operator<(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue < fp2.lValue);
}



static inline int operator>=(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue >= fp2.lValue);
}
static inline int operator>=(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue >= fp2.lValue);
}



static inline int operator<=(fixpoint fp1, fixpoint fp2)
{
   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(fixpoint fp1, int i)
{
fixpoint fp2(i);

   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(fixpoint fp1, unsigned u)
{
fixpoint fp2 = (long)u;

   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(fixpoint fp1, double d)
{
fixpoint fp2(d);

   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(int i, fixpoint fp2)
{
fixpoint fp1(i);

   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(unsigned u, fixpoint fp2)
{
fixpoint fp1 = (long)u;

   return(fp1.lValue <= fp2.lValue);
}
static inline int operator<=(double d, fixpoint fp2)
{
fixpoint fp1(d);

   return(fp1.lValue <= fp2.lValue);
}




#ifndef fpsqrt

static inline fixpoint fpsqrt(fixpoint __far &fp)
{
long l1, l2;
fixpoint fp1, fp2, fp3;


   if(fp.lValue <= 0)        // not valid for NEGATIVE numbers!!
   {
      fp1.lValue = 0;

#ifdef DEBUG
      sprintf(temp_buf, "sqrt(%f) = %f\r\n", (double)fp, (double)fp1);
      OutputDebugString(temp_buf);
#endif /* DEBUG */

      return(fp1);
   }


   // use CONVERGENCE method.  Determine optimum start value

   l1 = fp.lValue;

   if(*(((unsigned short FAR *)&l1)+1) > 0)
   {
      if(*(((unsigned char FAR *)&l1)+3) > 0)
      {
         if     (l1 >= 0x40000000L) l2 = 0x80000;
         else if(l1 >= 0x10000000L) l2 = 0x40000;
         else if(l1 >= 0x4000000L)  l2 = 0x20000;
         else /*if(l1 >= 0x1000000L)*/l2 = 0x10000;
      }
      else if(l1 >= 0x400000L)   l2 = 0x8000;
      else if(l1 >= 0x100000L)   l2 = 0x4000;
      else if(l1 >= 0x40000L)    l2 = 0x2000;
      else /*if(l1 >= 0x10000L)*/l2 = 0x1000;
   }
   else if(*(((unsigned char FAR *)&l1)+1) > 0)
   {
      if     (l1 >= 0x4000L)     l2 = 0x800;
      else if(l1 >= 0x1000L)     l2 = 0x400;
      else if(l1 >= 0x400L)      l2 = 0x200;
      else /* if(l1 >= 0x100L)*/ l2 = 0x100;
   }
   else if(l1 >= 0x40L)       l2 = 0x80;
   else if(l1 >= 0x10L)       l2 = 0x40;
   else if(l1 >= 0x4L)        l2 = 0x20;
   else                       l2 = 0x10;

   fp1.lValue = l2;

   l1 = l2 >> 1;  // halfsplit increment value, initially half of optimum
                  // start value 'fp1'.

   fp2.lValue = l1;


   // NEXT, begin convergence onto the correct answer by half-splitting.

   while(fp2 > 0)
   {
      fp3 = fp1 * fp1;   // test to see if I'm done yet...

      if(fp3.lValue > fp.lValue)
      {
         fp1 -= fp2;     // subtract current 'increment' value.
      }
      else
      {              // see if resulting square is equal (within roundoff)

         if((l1 = (fp3.lValue - fp.lValue)) <= 1L && l1 >= -1L)
         {
            break;   // 'fp1' contains the correct value.  Quit now!
         }
         else        // otherwise, calculated value is too small...
         {
            fp1 += fp2;
         }
      }

      // divide 'increment' value by 2.  When I reach zero, I'm done.
      // This is a maximum of 64 passes.  Not too bad, eh?

      fp2.lValue = fp2.lValue >> 1;

#ifdef DEBUG
      sprintf(temp_buf,"sqrt() pass: fp2 = %f, fp1 = %f,\r\n",
              (double)fp2, (double)fp1);
      OutputDebugString(temp_buf);

      sprintf(temp_buf,"             fp3 = %f, fp = %f\r\n",
              (double)fp3, (double)fp);
      OutputDebugString(temp_buf);
#endif /* DEBUG */
   }

#ifdef DEBUG
   sprintf(temp_buf, "sqrt(%f) = %f\r\n", (double)fp, (double)fp1);
   OutputDebugString(temp_buf);
#endif /* DEBUG */

   return(fp1);

}

#else  /* fpsqrt */


static inline fixpoint fpsqrt(fixpoint __far &fp)
{
double d1;
fixpoint fp1;


   d1 = fp;
   d1 = sqrt(d1);

   fp1 = (fixpoint) d1;

   return(fp1);
}


#endif /* fpsqrt */
