#ifndef ASMINLINE
#line 2 "../src/kernel/none/addll.h"
/* $Id: addll.h 7867 2006-04-14 15:26:51Z kb $

Copyright (C) 2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file originally adapted from gmp-3.1.1 (from T. Granlund), files
 * longlong.h and gmp-impl.h

  Copyright (C) 2000 Free Software Foundation, Inc. */

#undef LOCAL_OVERFLOW
#define LOCAL_OVERFLOW
extern ulong overflow;

#if !defined(INLINE)
extern long addll(ulong x, ulong y);
extern long addllx(ulong x, ulong y);
extern long subll(ulong x, ulong y);
extern long subllx(ulong x, ulong y);
#else

#if defined(__GNUC__) && !defined(DISABLE_INLINE)
#undef LOCAL_OVERFLOW
#define LOCAL_OVERFLOW register ulong overflow

#define addll(a, b)                                             \
({ ulong __arg1 = (a), __arg2 = (b), __value = __arg1 + __arg2; \
   overflow = (__value < __arg1);                               \
   __value;                                                     \
})

#define addllx(a, b)                                          \
({ ulong __arg1 = (a), __arg2 = (b), __value, __tmp = __arg1 + overflow;\
   overflow = (__tmp < __arg1);                               \
   __value = __tmp + __arg2;                                  \
   overflow |= (__value < __tmp);                             \
   __value;                                                   \
})

#define subll(a, b)                                           \
({ ulong __arg1 = (a), __arg2 = (b);                          \
   overflow = (__arg2 > __arg1);                              \
   __arg1 - __arg2;                                           \
})

#define subllx(a, b)                                  \
({ ulong __arg1 = (a), __arg2 = (b), __value, __tmp = __arg1 - overflow;\
   overflow = (__arg1 < overflow);                    \
   __value = __tmp - __arg2;                          \
   overflow |= (__arg2 > __tmp);                      \
   __value;                                           \
})

#else /* __GNUC__ */

INLINE long
addll(ulong x, ulong y)
{
  const ulong z = x+y;
  overflow=(z<x);
  return (long) z;
}

INLINE long
addllx(ulong x, ulong y)
{
  const ulong z = x+y+overflow;
  overflow = (z<x || (z==x && overflow));
  return (long) z;
}

INLINE long
subll(ulong x, ulong y)
{
  const ulong z = x-y;
  overflow = (z>x);
  return (long) z;
}

INLINE long
subllx(ulong x, ulong y)
{
  const ulong z = x-y-overflow;
  overflow = (z>x || (z==x && overflow));
  return (long) z;
}

#endif /* __GNUC__ */

#endif
#line 2 "../src/kernel/none/mulll.h"
/* $Id: mulll.h 7867 2006-04-14 15:26:51Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#undef  LOCAL_HIREMAINDER
#define LOCAL_HIREMAINDER
extern ulong hiremainder;

/* Version Peter Montgomery */
/*
 *      Assume (for presentation) that BITS_IN_LONG = 32.
 *      Then 0 <= xhi, xlo, yhi, ylo <= 2^16 - 1.  Hence
 *
 * -2^31 + 2^16 <= (xhi-2^15)*(ylo-2^15) + (xlo-2^15)*(yhi-2^15) <= 2^31.
 *
 *      If xhi*ylo + xlo*yhi = 2^32*overflow + xymid, then
 *
 * -2^32 + 2^16 <= 2^32*overflow + xymid - 2^15*(xhi + ylo + xlo + yhi) <= 0.
 *
 * 2^16*overflow <= (xhi+xlo+yhi+ylo)/2 - xymid/2^16 <= 2^16*overflow + 2^16-1
 *
 *       This inequality was derived using exact (rational) arithmetic;
 *       it remains valid when we truncate the two middle terms.
 */

#if !defined(INLINE)
extern long mulll(ulong x, ulong y);
extern long addmul(ulong x, ulong y);
#else

#if defined(__GNUC__) && !defined(DISABLE_INLINE)
#undef LOCAL_HIREMAINDER
#define LOCAL_HIREMAINDER register ulong hiremainder

#define mulll(x, y) \
({ \
  const ulong __x = (x), __y = (y);\
  const ulong __xlo = LOWWORD(__x), __xhi = HIGHWORD(__x); \
  const ulong __ylo = LOWWORD(__y), __yhi = HIGHWORD(__y); \
  ulong __xylo,__xymid,__xyhi,__xymidhi,__xymidlo; \
  ulong __xhl,__yhl; \
 \
  __xylo = __xlo*__ylo; __xyhi = __xhi*__yhi; \
  __xhl = __xhi+__xlo; __yhl = __yhi+__ylo; \
  __xymid = __xhl*__yhl - (__xyhi+__xylo); \
 \
  __xymidhi = HIGHWORD(__xymid); \
  __xymidlo = __xymid << BITS_IN_HALFULONG; \
 \
  __xylo += __xymidlo; \
  hiremainder = __xyhi + __xymidhi + (__xylo < __xymidlo) \
     + ((((__xhl + __yhl) >> 1) - __xymidhi) & HIGHMASK); \
 \
  __xylo; \
})

#define addmul(x, y) \
({ \
  const ulong __x = (x), __y = (y);\
  const ulong __xlo = LOWWORD(__x), __xhi = HIGHWORD(__x); \
  const ulong __ylo = LOWWORD(__y), __yhi = HIGHWORD(__y); \
  ulong __xylo,__xymid,__xyhi,__xymidhi,__xymidlo; \
  ulong __xhl,__yhl; \
 \
  __xylo = __xlo*__ylo; __xyhi = __xhi*__yhi; \
  __xhl = __xhi+__xlo; __yhl = __yhi+__ylo; \
  __xymid = __xhl*__yhl - (__xyhi+__xylo); \
 \
  __xylo += hiremainder; __xyhi += (__xylo < hiremainder); \
 \
  __xymidhi = HIGHWORD(__xymid); \
  __xymidlo = __xymid << BITS_IN_HALFULONG; \
 \
  __xylo += __xymidlo; \
  hiremainder = __xyhi + __xymidhi + (__xylo < __xymidlo) \
     + ((((__xhl + __yhl) >> 1) - __xymidhi) & HIGHMASK); \
 \
  __xylo; \
})

#else

INLINE long
mulll(ulong x, ulong y)
{
  const ulong xlo = LOWWORD(x), xhi = HIGHWORD(x);
  const ulong ylo = LOWWORD(y), yhi = HIGHWORD(y);
  ulong xylo,xymid,xyhi,xymidhi,xymidlo;
  ulong xhl,yhl;

  xylo = xlo*ylo; xyhi = xhi*yhi;
  xhl = xhi+xlo; yhl = yhi+ylo;
  xymid = xhl*yhl - (xyhi+xylo);

  xymidhi = HIGHWORD(xymid);
  xymidlo = xymid << BITS_IN_HALFULONG;

  xylo += xymidlo;
  hiremainder = xyhi + xymidhi + (xylo < xymidlo)
     + ((((xhl + yhl) >> 1) - xymidhi) & HIGHMASK);

  return xylo;
}

INLINE long
addmul(ulong x, ulong y)
{
  const ulong xlo = LOWWORD(x), xhi = HIGHWORD(x);
  const ulong ylo = LOWWORD(y), yhi = HIGHWORD(y);
  ulong xylo,xymid,xyhi,xymidhi,xymidlo;
  ulong xhl,yhl;

  xylo = xlo*ylo; xyhi = xhi*yhi;
  xhl = xhi+xlo; yhl = yhi+ylo;
  xymid = xhl*yhl - (xyhi+xylo);

  xylo += hiremainder; xyhi += (xylo < hiremainder);

  xymidhi = HIGHWORD(xymid);
  xymidlo = xymid << BITS_IN_HALFULONG;

  xylo += xymidlo;
  hiremainder = xyhi + xymidhi + (xylo < xymidlo)
     + ((((xhl + yhl) >> 1) - xymidhi) & HIGHMASK);

  return xylo;
}
#endif

#endif
#line 2 "../src/kernel/none/bfffo.h"
/* $Id: bfffo.h 7867 2006-04-14 15:26:51Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#if !defined(INLINE)
extern int  bfffo(ulong x);
#else

#if defined(__GNUC__) && !defined(DISABLE_INLINE)

#ifdef LONG_IS_64BIT
#  define bfffo(x) \
({\
  static int __bfffo_tabshi[16]={4,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0};\
  int __value = BITS_IN_LONG - 4; \
  ulong __arg1=(x); \
  if (__arg1 & ~0xffffffffUL) {__value -= 32; __arg1 >>= 32;}\
  if (__arg1 & ~0xffffUL) {__value -= 16; __arg1 >>= 16;} \
  if (__arg1 & ~0x00ffUL) {__value -= 8; __arg1 >>= 8;} \
  if (__arg1 & ~0x000fUL) {__value -= 4; __arg1 >>= 4;} \
  __value + __bfffo_tabshi[__arg1]; \
})
#else
#  define bfffo(x) \
({\
  static int __bfffo_tabshi[16]={4,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0};\
  int __value = BITS_IN_LONG - 4; \
  ulong __arg1=(x); \
  if (__arg1 & ~0xffffUL) {__value -= 16; __arg1 >>= 16;} \
  if (__arg1 & ~0x00ffUL) {__value -= 8; __arg1 >>= 8;} \
  if (__arg1 & ~0x000fUL) {__value -= 4; __arg1 >>= 4;} \
  __value + __bfffo_tabshi[__arg1]; \
})
#endif

#else

INLINE int
bfffo(ulong x)
{
  static int tabshi[16]={4,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0};
  int value = BITS_IN_LONG - 4;
  ulong arg1=x;
#ifdef LONG_IS_64BIT
  if (arg1 & ~0xffffffffUL) {value -= 32; arg1 >>= 32;}
#endif
  if (arg1 & ~0xffffUL) {value -= 16; arg1 >>= 16;}
  if (arg1 & ~0x00ffUL) {value -= 8; arg1 >>= 8;}
  if (arg1 & ~0x000fUL) {value -= 4; arg1 >>= 4;}
  return value + tabshi[arg1];
}
#endif

#endif
#line 2 "../src/kernel/none/divll.h"
/* $Id: divll.h 7867 2006-04-14 15:26:51Z kb $

Copyright (C) 2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file originally adapted from gmp-3.1.1 (from T. Granlund), files
 * longlong.h and gmp-impl.h

  Copyright (C) 2000 Free Software Foundation, Inc. */

#undef  LOCAL_HIREMAINDER
#define LOCAL_HIREMAINDER
extern ulong hiremainder;

#if !defined(INLINE)
extern long divll(ulong x, ulong y);
#else

#define __GLUE(hi, lo) (((hi) << BITS_IN_HALFULONG) | (lo))
#define __SPLIT(a, b, c) b = HIGHWORD(a); c = LOWWORD(a)
#define __LDIV(a, b, q, r) q = a / b; r = a - q*b
extern ulong hiremainder;

/* divide (hiremainder * 2^BITS_IN_LONG + n0) by d; assume hiremainder < d.
 * Return quotient, set hiremainder to remainder */

#if defined(__GNUC__) && !defined(DISABLE_INLINE)
#undef LOCAL_HIREMAINDER
#define LOCAL_HIREMAINDER register ulong hiremainder

#define divll(n0, d) 							\
({ 									\
  ulong __d1, __d0, __q1, __q0, __r1, __r0, __m, __n1, __n0;	        \
  ulong __k, __d;                                                       \
                                                                        \
  __n1 = hiremainder; __n0 = n0; __d = d;                               \
  if (__n1 == 0)                                                        \
  { /* Only one division needed */                                      \
    __LDIV(__n0, __d, __q1, hiremainder);                               \
  }                                                                     \
  else if (__d < LOWMASK)                                               \
  { /* Two half-word divisions  */                                      \
    __n1 = __GLUE(__n1, HIGHWORD(__n0));                                \
    __LDIV(__n1, __d, __q1, __r1);                                      \
    __n1 = __GLUE(__r1,  LOWWORD(__n0));                                \
    __LDIV(__n1, __d, __q0, hiremainder);                               \
    __q1 = __GLUE(__q1, __q0);                                          \
  }                                                                     \
  else                                                                  \
  { /* General case */                                                  \
    if (__d & HIGHBIT)                                                  \
    {                                                                   \
      __k = 0; __SPLIT(__d, __d1, __d0);                                \
    }                                                                   \
    else                                                                \
    {                                                                   \
      __k = bfffo(__d);                                                 \
      __n1 = (__n1 << __k) | (__n0 >> (BITS_IN_LONG - __k));            \
      __n0 <<= __k;                                                     \
      __d = __d << __k; __SPLIT(__d, __d1, __d0);                       \
    }                                                                   \
    __LDIV(__n1, __d1, __q1, __r1);                                     \
    __m =  __q1 * __d0; 					        \
    __r1 = __GLUE(__r1, HIGHWORD(__n0));  				\
    if (__r1 < __m)							\
    {									\
      __q1--, __r1 += __d;						\
      if (__r1 >= __d) /* we didn't get carry when adding to __r1 */    \
        if (__r1 < __m)	__q1--, __r1 += __d;				\
    }									\
    __r1 -= __m;							\
    __LDIV(__r1, __d1, __q0, __r0);                                     \
    __m =  __q0 * __d0;  					        \
    __r0 = __GLUE(__r0, LOWWORD(__n0));   				\
    if (__r0 < __m)							\
    {									\
      __q0--, __r0 += __d;						\
      if (__r0 >= __d)			        			\
        if (__r0 < __m)	__q0--, __r0 += __d;				\
    }									\
    hiremainder = (__r0 - __m) >> __k;		                        \
    __q1 = __GLUE(__q1, __q0);                 				\
  }                                   				        \
  __q1;					                                \
})

#else /* __GNUC__ */

INLINE long
divll(ulong n0, ulong d) 							
{
  ulong __d1, __d0, __q1, __q0, __r1, __r0, __m, __n1, __n0;
  ulong __k, __d;

  __n1 = hiremainder; __n0 = n0; __d = d;

  if (__n1 == 0)
  { /* Only one division needed */
    __LDIV(__n0, __d, __q1, hiremainder);
  }
  else if (__d < LOWMASK)
  { /* Two half-word divisions  */
    __n1 = __GLUE(__n1, HIGHWORD(__n0));
    __LDIV(__n1, __d, __q1, __r1);
    __n1 = __GLUE(__r1,  LOWWORD(__n0));
    __LDIV(__n1, __d, __q0, hiremainder);
    __q1 = __GLUE(__q1, __q0);
  }
  else
  { /* General case */
    if (__d & HIGHBIT)
    {
      __k = 0; __SPLIT(__d, __d1, __d0);
    }
    else
    {
      __k = bfffo(__d);
      __n1 = (__n1 << __k) | (__n0 >> (BITS_IN_LONG - __k));
      __n0 = __n0 << __k;
      __d = __d << __k; __SPLIT(__d, __d1, __d0);
    }
    __LDIV(__n1, __d1, __q1, __r1);
    __m =  __q1 * __d0;
    __r1 = __GLUE(__r1, HIGHWORD(__n0));
    if (__r1 < __m)
      {
        __q1--, __r1 += __d;
        if (__r1 >= __d) /* we didn't get carry when adding to __r1 */
          if (__r1 < __m) __q1--, __r1 += __d;
      }
    __r1 -= __m;
    __LDIV(__r1, __d1, __q0, __r0);
    __m =  __q0 * __d0;
    __r0 = __GLUE(__r0, LOWWORD(__n0));
    if (__r0 < __m)
      {
        __q0--, __r0 += __d;
        if (__r0 >= __d)
          if (__r0 < __m) __q0--, __r0 += __d;
      }
    hiremainder = (__r0 - __m) >> __k;
    __q1 = __GLUE(__q1, __q0);
  }
  return __q1;
}

#endif /* __GNUC__ */

#endif
#endif
#line 2 "../src/kernel/ix86/level0.h"
/* $Id: asm0.h 9123 2007-10-29 10:47:21Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file defines some "level 0" kernel functions for Intel ix86  */
/* It is intended for use with an external "asm" definition          */

/*
ASM addll mulll bfffo divll
*/
#ifdef ASMINLINE
/* Written by Bruno Haible, 1996-1998. */

/* This file can assume the GNU C extensions.
   (It is included only if __GNUC__ is defined.) */

/* Use local variables whenever possible. */
#define LOCAL_HIREMAINDER  register ulong hiremainder
#define LOCAL_OVERFLOW     register ulong overflow

#define addll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ("addl %3,%0 ; adcl %1,%1" \
	: "=r" (__value), "=r" (overflow) \
	: "0" (__arg1), "g" (__arg2), "1" ((ulong)0) \
	: "cc"); \
  __value; \
})

#define addllx(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b), __temp; \
   __asm__ ("subl %5,%2 ; adcl %4,%0 ; adcl %1,%1" \
	: "=r" (__value), "=&r" (overflow), "=r" (__temp) \
	: "0" (__arg1), "g" (__arg2), "g" (overflow), "1" ((ulong)0), "2" ((ulong)0) \
	: "cc"); \
  __value; \
})


#define subll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ("subl %3,%0 ; adcl %1,%1" \
	: "=r" (__value), "=r" (overflow) \
	: "0" (__arg1), "g" (__arg2), "1" ((ulong)0) \
	: "cc"); \
  __value; \
})

#define subllx(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b), __temp; \
   __asm__ ("subl %5,%2 ; sbbl %4,%0 ; adcl %1,%1" \
	: "=r" (__value), "=&r" (overflow), "=r" (__temp) \
	: "0" (__arg1), "g" (__arg2), "g" (overflow), "1" ((ulong)0), "2" ((ulong)0) \
	: "cc"); \
  __value; \
})

#define mulll(a,b) \
({ ulong __valuelo, __arg1 = (a), __arg2 = (b); \
   __asm__ ("mull %3" \
	: "=a" /* %eax */ (__valuelo), "=d" /* %edx */ (hiremainder) \
	: "0" (__arg1), "rm" (__arg2)); \
   __valuelo; \
})

#define addmul(a,b) \
({ ulong __valuelo, __arg1 = (a), __arg2 = (b), __temp; \
   __asm__ ("mull %4 ; addl %5,%0 ; adcl %6,%1" \
	: "=a" /* %eax */ (__valuelo), "=&d" /* %edx */ (hiremainder), "=r" (__temp) \
	: "0" (__arg1), "rm" (__arg2), "g" (hiremainder), "2" ((ulong)0)); \
   __valuelo; \
})

#define divll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ("divl %4" \
	: "=a" /* %eax */ (__value), "=&d" /* %edx */ (hiremainder) \
	: "0" /* %eax */ (__arg1), "1" /* %edx */ (hiremainder), "mr" (__arg2)); \
   __value; \
})

#define bfffo(x) \
({ ulong __arg = (x); \
   int leading_one_position; \
  __asm__ ("bsrl %1,%0" : "=r" (leading_one_position) : "rm" (__arg)); \
  31 - leading_one_position; \
})

#endif
/* tuned on laurent5.polytechnique.fr (Athlon 2200+) */
#define __DIVRR_GMP_LIMIT       -1 /* unused */
#define __Flx_INVMONTGOMERY_LIMIT  6000
#define __Flx_MUL_LIMIT             100
#define __Flx_POW_MONTGOMERY_LIMIT 1000
#define __Flx_SQR_LIMIT             200
#define __INVMOD_GMP_LIMIT      -1 /* unused */
#define __KARATSUBA_MULI_LIMIT  32
#define __KARATSUBA_MULR_LIMIT 294
#define __KARATSUBA_SQRI_LIMIT  62
#define __EXPNEWTON_LIMIT       87
#define __LOGAGM_LIMIT          24
#define __LOGAGMCX_LIMIT        90
#define __AGM_ATAN_LIMIT       130
#define __MONTGOMERY_LIMIT      70
#define __REMIIMUL_LIMIT       139
#define __RgX_MUL_LIMIT         10
#define __RgX_SQR_LIMIT          6
#line 2 "../src/kernel/none/int.h"
/* $Id: int.h 5644 2004-06-21 13:36:28Z bill $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#define int_MSW(x) ((x)+2)
/*x being a t_INT, return a pointer to the most significant word of x.*/

#define int_LSW(x) ((x)+lgefint((x))-1)
/*x being a t_INT, return a pointer to the least significant word of x.*/

#define int_precW(x) ((x)+1)
/*x pointing to a mantissa word, return the previous (less significant)
 * mantissa word.*/

#define int_nextW(x) ((x)-1)
/*x pointing to a mantissa word, return the next (more significant) mantissa
 * word.*/

#define int_W(x,l) ((x)+lgefint((x))-1-(l))
/*x being a t_INT, return a pointer to the l-th least significant word of x.*/

#define PARI_KERNEL_NONE
/*This macro should not be used in libpari itself.*/
#line 2 "../src/kernel/none/level1.h"
/* $Id: level1.h 7892 2006-04-19 16:18:26Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file defines "level 1" kernel functions.
 * These functions can be inline; if not they are defined externally in
 * level1.c, which includes this file and never needs to be changed
 * The following lines are necessary for level0.c and level1.c */

#if !defined(INLINE)
GEN    mkcol(GEN x);
GEN    mkcol2(GEN x, GEN y);
GEN    mkcolcopy(GEN x);
GEN    mkcomplex(GEN x, GEN y);
GEN    mkfrac(GEN x, GEN y);
GEN    mkintmod(GEN x, GEN y);
GEN    mkpolmod(GEN x, GEN y);
GEN    mkintmodu(ulong x, ulong y);
GEN    mkmat(GEN x);
GEN    mkmat2(GEN x, GEN y);
GEN    mkmatcopy(GEN x);
GEN    mkrfrac(GEN x, GEN y);
GEN    mkvec(GEN x);
GEN    mkvec2(GEN x, GEN y);
GEN    mkvec2s(long x, long y);
GEN    mkvec2copy(GEN x, GEN y);
GEN    mkvec3(GEN x, GEN y, GEN z);
GEN    mkvec3s(long x, long y, long z);
GEN    mkvec4(GEN x, GEN y, GEN z, GEN t);
GEN    mkveccopy(GEN x);
GEN    mkvecs(long x);
GEN    mkvecsmall(long x);
GEN    mkvecsmall2(long x,long y);
GEN    mkvecsmall3(long x,long y, long z);
void   affiz(GEN x, GEN y);
void   affsz(long x, GEN y);
GEN    addii(GEN x, GEN y);
GEN    addir(GEN x, GEN y);
GEN    addrr(GEN x, GEN y);
GEN    addsi(long x, GEN y);
void   affii(GEN x, GEN y);
void   affsi(long s, GEN x);
void   affsr(long s, GEN x);
void   affui(long s, GEN x);
void   affur(ulong s, GEN x);
GEN    cgetc(long x);
GEN    cgetg_copy(long lx, GEN x);
GEN    cgetg(long x, long y);
GEN    cgeti(long x);
GEN    cgetr(long x);
int    cmpir(GEN x, GEN y);
int    cmpsr(long x, GEN y);
GEN    constant_term(GEN x);
GEN    ctofp(GEN x, long prec);
void   divrrz(GEN x, GEN y, GEN z);
GEN    divsi_rem(long x, GEN y, long *rem);
void   divsiz(long x, GEN y, GEN z);
GEN    divss(long x, long y);
GEN    divss_rem(long x, long y, long *rem);
void   divssz(long x, long y, GEN z);
ulong  Fl_add(ulong a, ulong b, ulong p);
long   Fl_center(ulong u, ulong p, ulong ps2);
ulong  Fl_div(ulong a, ulong b, ulong p);
ulong  Fl_mul(ulong a, ulong b, ulong p);
ulong  Fl_neg(ulong x, ulong p);
ulong  Fl_sqr(ulong a, ulong p);
ulong  Fl_sub(ulong a, ulong b, ulong p);
int    dvdii(GEN x, GEN y);
int    dvdiiz(GEN x, GEN y, GEN z);
int    dvdisz(GEN x, long y, GEN z);
int    dvdiuz(GEN x, ulong y, GEN z);
void   dvmdiiz(GEN x, GEN y, GEN z, GEN t);
GEN    dvmdis(GEN x, long y, GEN *z);
void   dvmdisz(GEN x, long y, GEN z, GEN t);
GEN    dvmdsi(long x, GEN y, GEN *z);
void   dvmdsiz(long x, GEN y, GEN z, GEN t);
GEN    dvmdss(long x, long y, GEN *z);
void   dvmdssz(long x, long y, GEN z, GEN t);
long   evalexpo(long x);
long   evallg(long x);
long   evalvalp(long x);
long   expi(GEN x);
GEN    fractor(GEN x, long prec);
double gtodouble(GEN x);
GEN    icopy_av(GEN x, GEN y);
GEN    icopy(GEN x);
GEN    init_gen_op(GEN x, long tx, long *lx, long *i);
GEN    itor(GEN x, long prec);
long   itos(GEN x);
long   itos_or_0(GEN x);
ulong  itou(GEN x);
ulong  itou_or_0(GEN x);
GEN    leading_term(GEN x);
long   maxss(long x, long y);
long   minss(long x, long y);
GEN    modis(GEN x, long y);
GEN    modsi(long x, GEN y);
GEN    modss(long x, long y);
GEN    mpabs(GEN x);
GEN    mpadd(GEN x, GEN y);
void   mpaff(GEN x, GEN y);
GEN    mpceil(GEN x);
int    mpcmp(GEN x, GEN y);
GEN    mpcopy(GEN x);
GEN    mpdiv(GEN x, GEN y);
GEN    mpfloor(GEN x);
GEN    mpmul(GEN x, GEN y);
GEN    mpneg(GEN x);
GEN    mpround(GEN x);
GEN    mpsub(GEN x, GEN y);
GEN    mptrunc(GEN x);
GEN    new_chunk(size_t x);
long   random_bits(long k);
GEN    rdivii(GEN x, GEN y, long prec);
GEN    rdiviiz(GEN x, GEN y, GEN z);
GEN    rdivis(GEN x, long y, long prec);
GEN    rdivsi(long x, GEN y, long prec);
GEN    rdivss(long x, long y, long prec);
GEN    real2n(long n, long prec);
GEN    real_1(long prec);
GEN    real_m1(long prec);
GEN    real_0_bit(long bitprec);
GEN    real_0(long prec);
void   remiiz(GEN x, GEN y, GEN z);
GEN    remis(GEN x, long y);
GEN    remsi(long x, GEN y);
GEN    remss(long x, long y);
GEN    rtor(GEN x, long prec);
long   sdivsi(long x, GEN y)
long   sdivsi_rem(long x, GEN y, long *rem);
long   sdivss_rem(long x, long y, long *rem);
void   shift_left2(GEN z2, GEN z1, long min, long M, ulong f, ulong sh, ulong m);
void   shift_right2(GEN z2, GEN z1, long min, long M, ulong f, ulong sh, ulong m);
ulong  shiftl(ulong x, ulong y);
ulong  shiftlr(ulong x, ulong y);
GEN    shiftr(GEN x, long n);
long   smodis(GEN x, long y);
long   smodss(long x, long y);
void   stackdummy(pari_sp av, pari_sp ltop);
GEN    stoi(long x);
GEN    stor(long x, long prec);
GEN    subii(GEN x, GEN y);
GEN    subir(GEN x, GEN y);
GEN    subri(GEN x, GEN y);
GEN    subrr(GEN x, GEN y);
GEN    subsi(long x, GEN y);
ulong  umodui(ulong x, GEN y);
GEN    utoi(ulong x);
GEN    utoineg(ulong x);
GEN    utoipos(ulong x);
GEN    utor(ulong s, long prec);
long   vali(GEN x);
GEN    zerocol(long n);
GEN    zeromat(long m, long n);
GEN    zeromatcopy(long m, long n);
GEN    zeropadic(GEN p, long e);
GEN    zeropol(long v);
GEN    zeroser(long v, long e);
GEN    zerovec(long n);
GEN    col_ei(long n, long i);
GEN    vec_ei(long n, long i);

#else

INLINE long
evallg(long x)
{
  if (x & ~LGBITS) pari_err(errlg);
  return _evallg(x);
}

INLINE long
evalvalp(long x)
{
  const long v = _evalvalp(x);
  if (v & ~VALPBITS) pari_err(errvalp);
  return v;
}

INLINE long
evalexpo(long x)
{
  const long v = _evalexpo(x);
  if (v & ~EXPOBITS) pari_err(errexpo);
  return v;
}

INLINE GEN
constant_term(GEN x) { return signe(x)? gel(x,2): gen_0; }
INLINE GEN
leading_term(GEN x) { return lg(x) == 2? gen_0: gel(x,lg(x)-1); }

/* Inhibit some area gerepile-wise: declare it to be a non recursive
 * type, of length l. Thus gerepile won't inspect the zone, just copy it.
 * For the following situation:
 *   z = cgetg(t,a); av = avma; garbage(); ltop = avma;
 *   for (i=1; i<HUGE; i++) gel(z,i) = blah();
 *   stackdummy(av,ltop);
 * loses (av-ltop) words but save a costly gerepile.
 */
INLINE void
stackdummy(pari_sp av, pari_sp ltop) {
  long l = ((GEN)av) - ((GEN)ltop);
  if (l > 0) {
    GEN z = (GEN)ltop;
    z[0] = evaltyp(t_VECSMALL) | evallg(l);
#ifdef DEBUG
    { long i; for (i = 1; i < l; i++) z[i] = 0; }
#endif
  }
}

INLINE GEN
new_chunk(size_t x) /* x is a number of bytes */
{
  const GEN z = ((GEN) avma) - x;
  if (x > ((avma-bot)>>TWOPOTBYTES_IN_LONG)) pari_err(errpile);
#if defined(_WIN32) || defined(__CYGWIN32__)
  if (win32ctrlc) dowin32ctrlc();
#endif
  avma = (pari_sp)z;

#ifdef MEMSTEP
  if (DEBUGMEM && memused != DISABLE_MEMUSED) {
    long d = (long)memused - (long)z;
    if (d > 4*MEMSTEP || d < -4*MEMSTEP)
    {
      memused = (pari_sp)z;
      fprintferr("...%4.0lf Mbytes used\n",(top-memused)/1048576.);
    }
  }
#endif
  return z;
}

/* cgetg(lg(x), typ(x)), assuming lx = lg(x). Implicit unsetisclone() */
INLINE GEN
cgetg_copy(long lx, GEN x) {
  GEN y = new_chunk((size_t)lx);
  y[0] = x[0] & (TYPBITS|LGBITS); return y;
}
INLINE GEN
init_gen_op(GEN x, long tx, long *lx, long *i) {
  GEN y;
  *lx = lg(x); y = cgetg_copy(*lx, x);
  if (lontyp[tx] == 1) *i = 1; else { y[1] = x[1]; *i = 2; }
  return y;
}

INLINE GEN
cgetg(long x, long y)
{
  const GEN z = new_chunk((size_t)x);
  z[0] = evaltyp(y) | evallg(x);
  return z;
}

INLINE GEN
cgeti(long x)
{
  const GEN z = new_chunk((size_t)x);
  z[0] = evaltyp(t_INT) | evallg(x);
  return z;
}

INLINE GEN
cgetr(long x)
{
  const GEN z = new_chunk((size_t)x);
  z[0] = evaltyp(t_REAL) | evallg(x);
  return z;
}
INLINE GEN
cgetc(long l)
{
  GEN u = cgetg(3,t_COMPLEX); gel(u,1) = cgetr(l); gel(u,2) = cgetr(l);
  return u;
}
INLINE GEN
mkintmod(GEN x, GEN y) { GEN v = cgetg(3, t_INTMOD);
  gel(v,1) = y; gel(v,2) = x; return v; }
INLINE GEN
mkpolmod(GEN x, GEN y) { GEN v = cgetg(3, t_POLMOD);
  gel(v,1) = y; gel(v,2) = x; return v; }
INLINE GEN
mkfrac(GEN x, GEN y) { GEN v = cgetg(3, t_FRAC);
  gel(v,1) = x; gel(v,2) = y; return v; }
INLINE GEN
mkrfrac(GEN x, GEN y) { GEN v = cgetg(3, t_RFRAC);
  gel(v,1) = x; gel(v,2) = y; return v; }
INLINE GEN
mkcomplex(GEN x, GEN y) { GEN v = cgetg(3, t_COMPLEX);
  gel(v,1) = x; gel(v,2) = y; return v; }
INLINE GEN
mkvec(GEN x) { GEN v = cgetg(2, t_VEC); gel(v,1) = x; return v; }
INLINE GEN
mkvecsmall(long x) { GEN v = cgetg(2, t_VECSMALL); v[1] = x; return v; }
INLINE GEN
mkvecsmall2(long x,long y) { GEN v = cgetg(3, t_VECSMALL);
  v[1]=x; v[2]=y; return v; }
INLINE GEN
mkvecsmall3(long x,long y, long z) { GEN v = cgetg(4, t_VECSMALL);
  v[1]=x; v[2]=y; v[3]=z; return v; }
INLINE GEN
mkveccopy(GEN x) { GEN v = cgetg(2, t_VEC); gel(v,1) = gcopy(x); return v; }
INLINE GEN
mkvec2(GEN x, GEN y) {
  GEN v = cgetg(3,t_VEC); gel(v,1) = x; gel(v,2) = y; return v; }
INLINE GEN
mkvec3(GEN x, GEN y, GEN z) {
  GEN v=cgetg(4,t_VEC); gel(v,1) = x; gel(v,2) = y; gel(v,3) = z; return v; }
INLINE GEN
mkvec4(GEN x, GEN y, GEN z, GEN t) {
  GEN v=cgetg(5,t_VEC); gel(v,1) = x; gel(v,2) = y; gel(v,3) = z; gel(v,4) = t;
  return v; }
INLINE GEN
mkvec2copy(GEN x, GEN y) {
  GEN v = cgetg(3,t_VEC); gel(v,1) = gcopy(x); gel(v,2) = gcopy(y); return v; }
INLINE GEN
mkcol(GEN x) { GEN v = cgetg(2, t_COL); gel(v,1) = x; return v; }
INLINE GEN
mkcol2(GEN x, GEN y) {
  GEN v = cgetg(3,t_COL); gel(v,1) = x; gel(v,2) = y; return v; }
INLINE GEN
mkcolcopy(GEN x) { GEN v = cgetg(2, t_COL); gel(v,1) = gcopy(x); return v; }
INLINE GEN
mkmat(GEN x) { GEN v = cgetg(2, t_MAT); gel(v,1) = x; return v; }
INLINE GEN
mkmat2(GEN x, GEN y) { GEN v=cgetg(3,t_MAT); gel(v,1)=x; gel(v,2)=y; return v; }
INLINE GEN
mkmatcopy(GEN x) { GEN v = cgetg(2, t_MAT); gel(v,1) = gcopy(x); return v; }

/***   ZERO   ***/
/* O(p^e) */
INLINE GEN
zeropadic(GEN p, long e)
{
  GEN y = cgetg(5,t_PADIC);
  gel(y,4) = gen_0;
  gel(y,3) = gen_1;
  copyifstack(p,y[2]);
  y[1] = evalvalp(e) | evalprecp(0);
  return y;
}
/* O(pol_x[v]^e) */
INLINE GEN
zeroser(long v, long e)
{
  GEN x = cgetg(2, t_SER);
  x[1] = evalvalp(e) | evalvarn(v); return x;
}
/* 0 * pol_x[v] */
INLINE GEN
zeropol(long v)
{
  GEN x = cgetg(2,t_POL);
  x[1] = evalvarn(v); return x;
}
/* vector(n) */
INLINE GEN
zerocol(long n)
{
  GEN y = cgetg(n+1,t_COL);
  long i; for (i=1; i<=n; i++) gel(y,i) = gen_0;
  return y;
}
/* vectorv(n) */
INLINE GEN
zerovec(long n)
{
  GEN y = cgetg(n+1,t_VEC);
  long i; for (i=1; i<=n; i++) gel(y,i) = gen_0;
  return y;
}
/* matrix(m, n) */
INLINE GEN
zeromat(long m, long n)
{
  GEN y = cgetg(n+1,t_MAT);
  GEN v = zerocol(m);
  long i; for (i=1; i<=n; i++) gel(y,i) = v;
  return y;
}
/* matrix(m, n) */
INLINE GEN
zeromatcopy(long m, long n)
{
  GEN y = cgetg(n+1,t_MAT);
  long i; for (i=1; i<=n; i++) gel(y,i) = zerocol(m);
  return y;
}
/* i-th vector in the standard basis */
INLINE GEN
col_ei(long n, long i) { GEN e = zerocol(n); gel(e,i) = gen_1; return e; }
INLINE GEN
vec_ei(long n, long i) { GEN e = zerovec(n); gel(e,i) = gen_1; return e; }

/* cannot do memcpy because sometimes x and y overlap */
INLINE GEN
mpcopy(GEN x)
{
  register long lx = lg(x);
  const GEN y = cgetg_copy(lx, x);
  while (--lx > 0) y[lx] = x[lx];
  return y;
}

INLINE GEN
icopy(GEN x)
{
  register long lx = lgefint(x);
  const GEN y = cgeti(lx);
  while (--lx > 0) y[lx] = x[lx];
  return y;
}

/* copy integer x as if we had avma = av */
INLINE GEN
icopy_av(GEN x, GEN y)
{
  register long lx = lgefint(x);
  register long ly = lx;
  y -= lx; 
  while (--lx > 0) y[lx]=x[lx];
  y[0] = evaltyp(t_INT)|evallg(ly);
  return y;
}

INLINE GEN
mpneg(GEN x)
{
  const GEN y=mpcopy(x);
  setsigne(y,-signe(x)); return y;
}

INLINE GEN
mpabs(GEN x)
{
  const GEN y=mpcopy(x);
  if (signe(x)<0) setsigne(y,1);
  return y;
}

INLINE long
smodis(GEN x, long y)
{
  long rem;
  const pari_sp av=avma; (void)divis_rem(x,y, &rem); avma=av;
  return (rem >= 0) ? rem: labs(y) + rem;
}

INLINE long
smodss(long x, long y)
{ 
  long rem = x%y;
  return (rem >= 0)? rem: labs(y) + rem;
}

/* assume x != 0, return -x as a t_INT */
INLINE GEN
utoineg(ulong x)
{
  GEN y = cgeti(3);
  y[1] = evalsigne(-1)| evallgefint(3); y[2] = x; return y;
}
/* assume x != 0, return utoi(x) */
INLINE GEN
utoipos(ulong x)
{
  GEN y = cgeti(3);
  y[1] = evalsigne(1)| evallgefint(3); y[2] = x; return y;
}
INLINE GEN
utoi(ulong x) { return x? utoipos(x): gen_0; }
INLINE GEN
stoi(long x)
{
  if (!x) return gen_0;
  return x > 0? utoipos((ulong)x): utoineg((ulong)-x);
}
INLINE GEN
mkvecs(long x) { GEN v = cgetg(2, t_VEC); gel(v,1) = stoi(x); return v; }
INLINE GEN
mkvec2s(long x, long y) {
  GEN v = cgetg(3,t_VEC); gel(v,1) = stoi(x); gel(v,2) = stoi(y); return v; }
INLINE GEN
mkvec3s(long x, long y, long z) {
  GEN v=cgetg(4,t_VEC);
  gel(v,1)=stoi(x); gel(v,2)=stoi(y); gel(v,3)=stoi(z); return v;
}
INLINE GEN
mkintmodu(ulong x, ulong y) {
  GEN v = cgetg(3,t_INTMOD);
  gel(v,1) = utoipos(y);
  gel(v,2) = utoi(x); return v;
}

INLINE GEN
stosmall(long x)
{
  if (labs(x) & SMALL_MASK) return stoi(x);
  return (GEN) (1 | (x<<1));
}

INLINE long
itos(GEN x)
{
  const long s = signe(x);
  long u;

  if (!s) return 0;
  u = x[2]; if (lgefint(x) > 3 || u < 0) pari_err(affer2);
  return (s>0) ? u : -u;
}

/* as itos, but return 0 if too large. Cf is_bigint */
INLINE long
itos_or_0(GEN x) {
  long n;
  if (lgefint(x) != 3 || (n = x[2]) & HIGHBIT) return 0;
  return signe(x) > 0? n: -n;
}
/* as itou, but return 0 if too large. Cf is_bigint */
INLINE ulong
itou_or_0(GEN x) {
  if (lgefint(x) != 3) return 0;
  return (ulong)x[2];
}

INLINE GEN
modss(long x, long y) { return stoi(smodss(x, y)); }

INLINE GEN
remss(long x, long y) { return stoi(x % y); }

INLINE void
affii(GEN x, GEN y)
{
  long lx;

  if (x==y) return;
  lx=lgefint(x); if (lg(y)<lx) pari_err(affer3);
  while (--lx) y[lx]=x[lx];
}

INLINE void
affsi(long s, GEN x)
{
  if (!s) x[1] = evalsigne(0) | evallgefint(2);
  else
  {
    if (s > 0) { x[1] = evalsigne( 1) | evallgefint(3); x[2] =  s; }
    else       { x[1] = evalsigne(-1) | evallgefint(3); x[2] = -s; }
  }
}

INLINE void
affsr(long x, GEN y)
{
  long sh, i, ly = lg(y);

  if (!x)
  {
    y[1] = evalexpo(-bit_accuracy(ly));
    return;
  }
  if (x < 0) {
    x = -x; sh = bfffo(x);
    y[1] = evalsigne(-1) | _evalexpo((BITS_IN_LONG-1)-sh);
  }
  else
  {
    sh = bfffo(x);
    y[1] = evalsigne(1) | _evalexpo((BITS_IN_LONG-1)-sh);
  }
  y[2] = x<<sh; for (i=3; i<ly; i++) y[i]=0;
}

INLINE void
affur(ulong x, GEN y)
{
  long sh, i, ly = lg(y);

  if (!x)
  {
    y[1] = evalexpo(-bit_accuracy(ly));
    return;
  }
  sh = bfffo(x);
  y[1] = evalsigne(1) | _evalexpo((BITS_IN_LONG-1)-sh);
  y[2] = x<<sh; for (i=3; i<ly; i++) y[i] = 0;
}

INLINE void
affiz(GEN x, GEN y) { if (typ(y)==t_INT) affii(x,y); else affir(x,y); }
INLINE void
affsz(long x, GEN y) { if (typ(y)==t_INT) affsi(x,y); else affsr(x,y); }
INLINE void
mpaff(GEN x, GEN y) { if (typ(x)==t_INT) affiz(x, y); else affrr(x,y); }

INLINE GEN
real_0_bit(long bitprec) { GEN x=cgetr(2); x[1]=evalexpo(bitprec); return x; }
INLINE GEN
real_0(long prec) { return real_0_bit(-bit_accuracy(prec)); }
INLINE GEN
real_1(long prec) {
  GEN x = cgetr(prec);
  long i;
  x[1] = evalsigne(1) | _evalexpo(0);
  x[2] = (long)HIGHBIT; for (i=3; i<prec; i++) x[i] = 0;
  return x;
}
INLINE GEN
real_m1(long prec) {
  GEN x = cgetr(prec);
  long i;
  x[1] = evalsigne(-1) | _evalexpo(0);
  x[2] = (long)HIGHBIT; for (i=3; i<prec; i++) x[i] = 0;
  return x;
}
/* 2.^n */
INLINE GEN
real2n(long n, long prec) { GEN z = real_1(prec); setexpo(z, n); return z; }
INLINE GEN
stor(long s, long prec) { GEN z = cgetr(prec); affsr(s,z); return z; }
INLINE GEN
utor(ulong s, long prec){ GEN z = cgetr(prec); affur(s,z); return z; }
INLINE GEN
itor(GEN x, long prec) { GEN z = cgetr(prec); affir(x,z); return z; }
INLINE GEN
rtor(GEN x, long prec) { GEN z = cgetr(prec); affrr(x,z); return z; }
INLINE GEN
ctofp(GEN x, long prec) { GEN z = cgetg(3,t_COMPLEX);
  gel(z,1) = gtofp(gel(x,1),prec);
  gel(z,2) = gtofp(gel(x,2),prec); return z;
}

INLINE GEN
shiftr(GEN x, long n)
{
  const long e = evalexpo(expo(x)+n);
  const GEN y = rcopy(x);

  if (e & ~EXPOBITS) pari_err(talker,"overflow in real shift");
  y[1] = (y[1]&~EXPOBITS) | e; return y;
}

INLINE int
cmpir(GEN x, GEN y)
{
  pari_sp av;
  GEN z;

  if (!signe(x)) return -signe(y);
  if (!signe(y)) return  signe(x);
  av=avma; z = itor(x, lg(y)); avma=av;
  return cmprr(z,y); /* cmprr does no memory adjustment */
}

INLINE int
cmpsr(long x, GEN y)
{
  pari_sp av;
  GEN z;

  if (!x) return -signe(y);
  av=avma; z = stor(x, 3); avma=av;
  return cmprr(z,y);
}	

INLINE long 
maxss(long x, long y) { return x>y?x:y; }
INLINE long 
minss(long x, long y) { return x<y?x:y; }

INLINE GEN
subii(GEN x, GEN y)
{
  if (x==y) return gen_0; /* frequent with x = y = gen_0 */
  return addii_sign(x, signe(x), y, -signe(y));
}
INLINE GEN
addii(GEN x, GEN y) { return addii_sign(x, signe(x), y, signe(y)); }
INLINE GEN
addrr(GEN x, GEN y) { return addrr_sign(x, signe(x), y, signe(y)); }
INLINE GEN
subrr(GEN x, GEN y) { return addrr_sign(x, signe(x), y, -signe(y)); }
INLINE GEN
addir(GEN x, GEN y) { return addir_sign(x, signe(x), y, signe(y)); }
INLINE GEN
subir(GEN x, GEN y) { return addir_sign(x, signe(x), y, -signe(y)); }
INLINE GEN
subri(GEN x, GEN y) { return addir_sign(y, -signe(y), x, signe(x)); }
INLINE GEN
addsi(long x, GEN y) { return addsi_sign(x, y, signe(y)); }
INLINE GEN
subsi(long x, GEN y) { return addsi_sign(x, y, -signe(y)); }

INLINE long
vali(GEN x)
{
  long i;
  GEN xp;

  if (!signe(x)) return -1;
  xp=int_LSW(x); 
  for (i=0; !*xp; i++) xp=int_nextW(xp);
  return (i<<TWOPOTBITS_IN_LONG) + vals(*xp);
}

INLINE GEN
divss(long x, long y) { return stoi(x / y); }

INLINE long
sdivss_rem(long x, long y, long *rem)
{
  long q;
  LOCAL_HIREMAINDER;
  if (!y) pari_err(gdiver);
  hiremainder = 0; q = divll((ulong)labs(x),(ulong)labs(y));
  if (x < 0) { hiremainder = -((long)hiremainder); q = -q; }
  if (y < 0) q = -q;
  *rem = hiremainder; return q;
}

INLINE GEN
divss_rem(long x, long y, long *rem) { return stoi(sdivss_rem(x,y,rem)); }

INLINE GEN
dvmdss(long x, long y, GEN *z)
{
  long rem;
  const GEN q = divss_rem(x,y, &rem);
  *z = stoi(rem); return q;
}

INLINE long
sdivsi_rem(long x, GEN y, long *rem)
{
  long q, s = signe(y);
  LOCAL_HIREMAINDER;

  if (!s) pari_err(gdiver);
  if (!x || lgefint(y)>3 || ((long)y[2]) < 0) { *rem = x; return 0; }
  hiremainder=0; q = (long)divll(labs(x), (ulong)y[2]);
  if (x < 0) { hiremainder = -((long)hiremainder); q = -q; }
  if (s < 0) q = -q;
  *rem = hiremainder; return q;
}

INLINE long
sdivsi(long x, GEN y)
{
  long q, s = signe(y);

  if (!s) pari_err(gdiver);
  if (!x || lgefint(y)>3 || ((long)y[2]) < 0) return 0;
  q = labs(x) / y[2];
  if (x < 0) q = -q;
  if (s < 0) q = -q;
  return q;
}

INLINE GEN
modsi(long x, GEN y) {
  long r;
  (void)sdivsi_rem(x, y, &r);
  return (r >= 0)? stoi(r): addsi_sign(r, y, 1);
}

INLINE GEN
divsi_rem(long s, GEN y, long *rem) { return stoi(sdivsi_rem(s,y,rem)); }

INLINE GEN
dvmdsi(long x, GEN y, GEN *z)
{
  long rem;
  const GEN q = divsi_rem(x,y, &rem);
  *z = stoi(rem); return q;
}

INLINE GEN
dvmdis(GEN x, long y, GEN *z)
{
  long rem;
  const GEN q = divis_rem(x,y, &rem);
  *z = stoi(rem); return q;
}

INLINE void
dvmdssz(long x, long y, GEN z, GEN t)
{
  long rem;
  const pari_sp av=avma; affiz(divss_rem(x,y, &rem), z); avma=av;
  affsi(rem,t);
}

INLINE void
dvmdsiz(long x, GEN y, GEN z, GEN t)
{
  long rem;
  const pari_sp av = avma; affiz(divsi_rem(x,y, &rem), z); avma = av;
  affsi(rem,t);
}

INLINE void
dvmdisz(GEN x, long y, GEN z, GEN t)
{
  long rem;
  const pari_sp av=avma; affiz(divis_rem(x,y, &rem),z); avma=av;
  affsz(rem,t);
}

INLINE void
dvmdiiz(GEN x, GEN y, GEN z, GEN t)
{
  const pari_sp av=avma;
  GEN p;

  affiz(dvmdii(x,y,&p),z); affiz(p,t); avma=av;
}

INLINE GEN
modis(GEN x, long y)
{
  return stoi(smodis(x,y));
}

INLINE ulong
umodui(ulong x, GEN y)
{
  LOCAL_HIREMAINDER;
  if (!signe(y)) pari_err(gdiver);
  if (!x || lgefint(y) > 3) return x;
  hiremainder = 0; (void)divll(x, y[2]); return hiremainder;
}

INLINE GEN
remsi(long x, GEN y)
{
  long rem;
  const pari_sp av=avma; (void)divsi_rem(x,y, &rem); avma=av;
  return stoi(rem);
}

INLINE GEN
remis(GEN x, long y)
{
  long rem;
  const pari_sp av=avma; (void)divis_rem(x,y, &rem); avma=av;
  return stoi(rem);
}

INLINE GEN
rdivis(GEN x, long y, long prec)
{
  GEN z = cgetr(prec);
  const pari_sp av = avma;
  affrr(divrs(itor(x,prec), y),z);
  avma = av; return z;
}

INLINE void
divsiz(long x, GEN y, GEN z)
{
  long junk;
  affsi(sdivsi_rem(x,y,&junk), z);
}

INLINE void
divssz(long x, long y, GEN z) { affsi(x/y, z); }

INLINE GEN
rdivsi(long x, GEN y, long prec)
{
  GEN z = cgetr(prec);
  const pari_sp av = avma;
  affrr(divsr(x, itor(y,prec)), z);
  avma = av; return z;
}

INLINE GEN
rdivss(long x, long y, long prec)
{
  GEN z = cgetr(prec);
  const pari_sp av = avma;
  affrr(divrs(stor(x, prec), y), z);
  avma = av; return z;
}

INLINE GEN
rdiviiz(GEN x, GEN y, GEN z)
{
  const pari_sp av = avma;
  long prec = lg(z);
  affir(x, z);
  if (!is_bigint(y)) {
    affrr(divrs(z, y[2]), z);
    if (signe(y) < 0) setsigne(z, -signe(z));
  }
  else
    affrr(divrr(z, itor(y,prec)), z);
  avma = av; return z;
}

INLINE GEN 
rdivii(GEN x, GEN y, long prec) { return rdiviiz(x, y, cgetr(prec)); }
INLINE GEN
fractor(GEN x, long prec) { return rdivii(gel(x,1), gel(x,2), prec); }

INLINE void
divrrz(GEN x, GEN y, GEN z)
{
  const pari_sp av=avma;
  affrr(divrr(x,y),z); avma=av;
}

INLINE void
remiiz(GEN x, GEN y, GEN z)
{
  const pari_sp av=avma;
  affii(remii(x,y),z); avma=av;
}

INLINE int
dvdii(GEN x, GEN y)
{
  const pari_sp av=avma;
  const GEN p1=remii(x,y);
  avma=av; return p1 == gen_0;
}

INLINE int
mpcmp(GEN x, GEN y)
{
  if (typ(x)==t_INT)
    return (typ(y)==t_INT) ? cmpii(x,y) : cmpir(x,y);
  return (typ(y)==t_INT) ? -cmpir(y,x) : cmprr(x,y);
}

INLINE GEN
mptrunc(GEN x) { return typ(x)==t_INT? icopy(x): truncr(x); }
INLINE GEN
mpfloor(GEN x) { return typ(x)==t_INT? icopy(x): floorr(x); }
INLINE GEN
mpceil(GEN x) { return typ(x)==t_INT? icopy(x): ceilr(x); }
INLINE GEN
mpround(GEN x) { return typ(x) == t_INT? icopy(x): roundr(x); }

INLINE GEN
mpadd(GEN x, GEN y)
{
  if (typ(x)==t_INT)
    return (typ(y)==t_INT) ? addii(x,y) : addir(x,y);
  return (typ(y)==t_INT) ? addir(y,x) : addrr(x,y);
}

INLINE GEN
mpsub(GEN x, GEN y)
{
  if (typ(x)==t_INT)
    return (typ(y)==t_INT) ? subii(x,y) : subir(x,y);
  return (typ(y)==t_INT) ? subri(x,y) : subrr(x,y);
}

INLINE GEN
mpmul(GEN x, GEN y)
{
  if (typ(x)==t_INT)
    return (typ(y)==t_INT) ? mulii(x,y) : mulir(x,y);
  return (typ(y)==t_INT) ? mulir(y,x) : mulrr(x,y);
}

INLINE GEN
mpdiv(GEN x, GEN y)
{
  if (typ(x)==t_INT)
    return (typ(y)==t_INT) ? divii(x,y) : divir(x,y);
  return (typ(y)==t_INT) ? divri(x,y) : divrr(x,y);
}

INLINE int
dvdiiz(GEN x, GEN y, GEN z)
{
  const pari_sp av=avma;
  GEN p2;
  const GEN p1=dvmdii(x,y,&p2);

  if (signe(p2)) { avma=av; return 0; }
  affii(p1,z); avma=av; return 1;
}

/* assume 0 <= k < 32. Return random 0 <= x < (1<<k) */
INLINE long
random_bits(long k) { return pari_rand31() >> (31 - k); }

INLINE ulong
itou(GEN x)
{
  const long s = signe(x);

  if (!s) return 0;
  if (lgefint(x) > 3) pari_err(affer2);
  return x[2];
}

INLINE void
affui(ulong u, GEN x)
{
  if (!u) x[1] = evalsigne(0) | evallgefint(2);
  else  { x[1] = evalsigne(1) | evallgefint(3); x[2] = u; }
}

INLINE int
dvdisz(GEN x, long y, GEN z)
{
  const pari_sp av = avma;
  long rem;
  GEN p1 = divis_rem(x,y, &rem);
  avma = av; if (rem) return 0;
  affii(p1,z); return 1;
}

INLINE int
dvdiuz(GEN x, ulong y, GEN z)
{
  const pari_sp av = avma;
  ulong rem;
  GEN p1 = diviu_rem(x,y, &rem);
  avma = av; if (rem) return 0;
  affii(p1,z); return 1;
}

INLINE double
gtodouble(GEN x)
{
  static long reel4[4]={ evaltyp(t_REAL) | _evallg(4),0,0,0 };

  if (typ(x)==t_REAL) return rtodbl(x);
  gaffect(x,(GEN)reel4); return rtodbl((GEN)reel4);
}

/* same as Fl_add, assume p <= 2^(BIL - 1), so that overflow can't occur */
INLINE ulong
Fl_add_noofl(ulong a, ulong b, ulong p)
{
  ulong res = a + b;
  return (res >= p) ? res - p : res;
}
INLINE ulong
Fl_add(ulong a, ulong b, ulong p)
{
  ulong res = a + b;
  return (res >= p || res < a) ? res - p : res;
}
INLINE ulong
Fl_neg(ulong x, ulong p) { return x ? p - x: 0; }

INLINE ulong
Fl_sub(ulong a, ulong b, ulong p)
{
  ulong res = a - b;
  return (res > a) ? res + p: res;
}

/* centerlift(u mod p) */
INLINE long
Fl_center(ulong u, ulong p, ulong ps2) { return (long) (u > ps2)? u - p: u; }

INLINE ulong
Fl_mul(ulong a, ulong b, ulong p)
{
  LOCAL_HIREMAINDER;
  {
    register ulong x = mulll(a,b);
    (void)divll(x,p);
  }
  return hiremainder;
}
INLINE ulong
Fl_sqr(ulong a, ulong p)
{
  LOCAL_HIREMAINDER;
  {
    register ulong x = mulll(a,a);
    (void)divll(x,p);
  }
  return hiremainder;
}
INLINE ulong
Fl_div(ulong a, ulong b, ulong p)
{
  return Fl_mul(a, Fl_inv(b, p), p);
}

INLINE long
expi(GEN x)
{
  const long lx=lgefint(x);
  return lx==2? -(long)HIGHEXPOBIT: bit_accuracy(lx)-(long)bfffo(*int_MSW(x))-1;
}

/* z2[imin..imax] := z1[imin..imax].f shifted left sh bits
 * (feeding f from the right). Assume sh > 0 */
INLINE void
shift_left2(GEN z2, GEN z1, long imin, long imax, ulong f, ulong sh, ulong m)
{
  GEN sb = z1 + imin, se = z1 + imax, te = z2 + imax;
  ulong l, k = f >> m;
  while (se > sb) {
    l     = *se--;
    *te-- = (l << sh) | k;
    k     = l >> m;
  }
  *te = (*se << sh) | k;
}
/* z2[imin..imax] := f.z1[imin..imax-1] shifted right sh bits
 * (feeding f from the left). Assume sh > 0 */
INLINE void
shift_right2(GEN z2, GEN z1, long imin, long imax, ulong f, ulong sh, ulong m)
{
  GEN sb = z1 + imin, se = z1 + imax, tb = z2 + imin;
  ulong k, l = *sb++;
  *tb++ = (l >> sh) | (f << m);
  while (sb < se) {
    k     = l << m;
    l     = *sb++;
    *tb++ = (l >> sh) | k;
  }
}

/* Backward compatibility. Inefficient && unused */
extern ulong hiremainder;
INLINE ulong
shiftl(ulong x, ulong y)
{
  hiremainder=x>>(BITS_IN_LONG-y);
  return (x<<y);
}
INLINE ulong
shiftlr(ulong x, ulong y)
{
  hiremainder=x<<(BITS_IN_LONG-y);
  return (x>>y);
}
#endif
