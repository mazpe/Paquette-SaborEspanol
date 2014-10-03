#line 2 "../src/kernel/none/mp.c"
/* $Id: mp.c 8000 2006-08-24 21:51:50Z kb $

Copyright (C) 2000-2003 The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/***********************************************************************/
/**								      **/
/**		         MULTIPRECISION KERNEL           	      **/
/**                                                                   **/
/***********************************************************************/
#include "pari.h"
#include "paripriv.h"
#include "../src/kernel/none/tune-gen.h"

int pari_kernel_init(void) { return 0; } /*nothing to do*/

/* NOTE: arguments of "spec" routines (muliispec, addiispec, etc.) aren't
 * GENs but pairs (long *a, long na) representing a list of digits (in basis
 * BITS_IN_LONG) : a[0], ..., a[na-1]. [ In ordre to facilitate splitting: no
 * need to reintroduce codewords ] */

/* Normalize a non-negative integer */
GEN
int_normalize(GEN x, long known_zero_words)
{
  long lx = lgefint(x);
  long i = 2 + known_zero_words;
  for ( ; i < lx; i++)
    if (x[i])
    {
      if (i != 2)
      {
        GEN x0 = x;
        i -= 2; x += i;
        if (x0 == (GEN)avma) avma = (pari_sp)x;
        else stackdummy((pari_sp)(x0+i), (pari_sp)x0);
        lx -= i;
        x[0] = evaltyp(t_INT) | evallg(lx);
        x[1] = evalsigne(1) | evallgefint(lx);
      }
      return x;
    }
  x[1] = evalsigne(0) | evallgefint(2); return x;
}

/***********************************************************************/
/**								      **/
/**		         ADDITION / SUBTRACTION          	      **/
/**                                                                   **/
/***********************************************************************/

GEN
setloop(GEN a)
{
  GEN z0 = (GEN)avma; (void)cgetg(lgefint(a) + 3, t_VECSMALL);
  return icopy_av(a, z0); /* two cells of extra space before a */
}

/* we had a = setloop(?), then some incloops. Reset a to b */
GEN
resetloop(GEN a, GEN b) {
  long lb = lgefint(b);
  a += lgefint(a) - lb;
  a[0] = evaltyp(t_INT) | evallg(lb);
  affii(b, a); return a;
}

/* assume a > 0, initialized by setloop. Do a++ */
static GEN
incpos(GEN a)
{
  long i, l = lgefint(a);
  for (i=l-1; i>1; i--)
    if (++a[i]) return a;
  l++; a--; /* use extra cell */
  a[0]=evaltyp(t_INT) | _evallg(l);
  a[1]=evalsigne(1) | evallgefint(l);
  a[2]=1; return a;
}

/* assume a < 0, initialized by setloop. Do a++ */
static GEN
incneg(GEN a)
{
  long l = lgefint(a)-1;
  if (a[l]--)
  {
    if (l == 2 && !a[2])
    {
      a++; /* save one cell */
      a[0] = evaltyp(t_INT) | _evallg(2);
      a[1] = evalsigne(0) | evallgefint(2);
    }
    return a;
  }
  for (l--; l>1; l--)
    if (a[l]--) break;
  l++; a++; /* save one cell */
  a[0] = evaltyp(t_INT) | _evallg(l);
  a[1] = evalsigne(-1) | evallgefint(l);
  return a;
}

/* assume a initialized by setloop. Do a++ */
GEN
incloop(GEN a)
{
  switch(signe(a))
  {
    case 0: a--; /* use extra cell */
      a[0]=evaltyp(t_INT) | _evallg(3);
      a[1]=evalsigne(1) | evallgefint(3);
      a[2]=1; return a;
    case -1: return incneg(a);
    default: return incpos(a);
  }
}

INLINE GEN
addsispec(long s, GEN x, long nx)
{
  GEN xd, zd = (GEN)avma;
  long lz;

  lz = nx+3; (void)new_chunk(lz);
  xd = x + nx;
  *--zd = *--xd + s;
  if ((ulong)*zd < (ulong)s)
    for(;;)
    {
      if (xd == x) { *--zd = 1; break; } /* enlarge z */
      *--zd = ((ulong)*--xd) + 1;
      if (*zd) { lz--; break; }
    }
  else lz--;
  while (xd > x) *--zd = *--xd;
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

static GEN
addiispec(GEN x, GEN y, long nx, long ny)
{
  GEN xd,yd,zd;
  long lz;
  LOCAL_OVERFLOW;

  if (nx < ny) swapspec(x,y, nx,ny);
  if (ny == 1) return addsispec(*y,x,nx);
  zd = (GEN)avma;
  lz = nx+3; (void)new_chunk(lz);
  xd = x + nx;
  yd = y + ny;
  *--zd = addll(*--xd, *--yd);
  while (yd > y) *--zd = addllx(*--xd, *--yd);
  if (overflow)
    for(;;)
    {
      if (xd == x) { *--zd = 1; break; } /* enlarge z */
      *--zd = ((ulong)*--xd) + 1;
      if (*zd) { lz--; break; }
    }
  else lz--;
  while (xd > x) *--zd = *--xd;
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

/* assume x >= y */
INLINE GEN
subisspec(GEN x, long s, long nx)
{
  GEN xd, zd = (GEN)avma;
  long lz;
  LOCAL_OVERFLOW;

  lz = nx+2; (void)new_chunk(lz);
  xd = x + nx;
  *--zd = subll(*--xd, s);
  if (overflow)
    for(;;)
    {
      *--zd = ((ulong)*--xd) - 1;
      if (*xd) break;
    }
  if (xd == x)
    while (*zd == 0) { zd++; lz--; } /* shorten z */
  else
    do  *--zd = *--xd; while (xd > x);
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

/* assume x > y */
static GEN
subiispec(GEN x, GEN y, long nx, long ny)
{
  GEN xd,yd,zd;
  long lz;
  LOCAL_OVERFLOW;

  if (ny==1) return subisspec(x,*y,nx);
  zd = (GEN)avma;
  lz = nx+2; (void)new_chunk(lz);
  xd = x + nx;
  yd = y + ny;
  *--zd = subll(*--xd, *--yd);
  while (yd > y) *--zd = subllx(*--xd, *--yd);
  if (overflow)
    for(;;)
    {
      *--zd = ((ulong)*--xd) - 1;
      if (*xd) break;
    }
  if (xd == x)
    while (*zd == 0) { zd++; lz--; } /* shorten z */
  else
    do  *--zd = *--xd; while (xd > x);
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

static void
roundr_up_ip(GEN x, long l)
{
  long i = l;
  for(;;)
  {
    if (++x[--i]) break;
    if (i == 2) { x[2] = HIGHBIT; setexpo(x, expo(x)+1); break; }
  }
}

void
affir(GEN x, GEN y)
{
  const long s = signe(x), ly = lg(y);
  long lx, sh, i;

  if (!s)
  {
    y[1] = evalexpo(-bit_accuracy(ly));
    return;
  }

  lx = lgefint(x); sh = bfffo(x[2]);
  y[1] = evalsigne(s) | evalexpo(bit_accuracy(lx)-sh-1);
  if (sh) {
    if (lx <= ly)
    {
      for (i=lx; i<ly; i++) y[i]=0;
      shift_left(y,x,2,lx-1, 0,sh);
      return;
    }
    shift_left(y,x,2,ly-1, x[ly],sh);
    /* lx > ly: round properly */
    if ((x[ly]<<sh) & HIGHBIT) roundr_up_ip(y, ly);
  }
  else {
    if (lx <= ly)
    {
      for (i=2; i<lx; i++) y[i]=x[i];
      for (   ; i<ly; i++) y[i]=0;
      return;
    }
    for (i=2; i<ly; i++) y[i]=x[i];
    /* lx > ly: round properly */
    if (x[ly] & HIGHBIT) roundr_up_ip(y, ly);
  }
}

static GEN
shifti_spec(GEN x, long lx, long n)
{
  long ly, i, m, s = signe(x);
  GEN y;
  if (!s) return gen_0;
  if (!n)
  {
    y = cgeti(lx);
    y[1] = evalsigne(s) | evallgefint(lx);
    while (--lx > 1) y[lx]=x[lx];
    return y;
  }
  if (n > 0)
  {
    GEN z = (GEN)avma;
    long d = n>>TWOPOTBITS_IN_LONG;

    ly = lx+d; y = new_chunk(ly);
    for ( ; d; d--) *--z = 0;
    m = n & (BITS_IN_LONG-1);
    if (!m) for (i=2; i<lx; i++) y[i]=x[i];
    else
    {
      register const ulong sh = BITS_IN_LONG - m;
      shift_left2(y,x, 2,lx-1, 0,m,sh);
      i = ((ulong)x[2]) >> sh;
      /* Extend y on the left? */
      if (i) { ly++; y = new_chunk(1); y[2] = i; }
    }
  }
  else
  {
    n = -n;
    ly = lx - (n>>TWOPOTBITS_IN_LONG);
    if (ly<3) return gen_0;
    y = new_chunk(ly);
    m = n & (BITS_IN_LONG-1);
    if (m) {
      shift_right(y,x, 2,ly, 0,m);
      if (y[2] == 0)
      {
        if (ly==3) { avma = (pari_sp)(y+3); return gen_0; }
        ly--; avma = (pari_sp)(++y);
      }
    } else {
      for (i=2; i<ly; i++) y[i]=x[i];
    }
  }
  y[1] = evalsigne(s)|evallgefint(ly);
  y[0] = evaltyp(t_INT)|evallg(ly); return y;
}

GEN
shifti(GEN x, long n)
{
  return shifti_spec(x, lgefint(x), n);
}

GEN
ishiftr_lg(GEN x, long lx, long n)
{ /*This is a kludge since x is not an integer*/
  return shifti_spec(x, lx, n);
}

GEN
truncr(GEN x)
{
  long d,e,i,s,m;
  GEN y;

  if ((s=signe(x)) == 0 || (e=expo(x)) < 0) return gen_0;
  d = (e>>TWOPOTBITS_IN_LONG) + 3;
  m = e & (BITS_IN_LONG-1);
  if (d > lg(x)) pari_err(precer, "truncr (precision loss in truncation)");

  y=cgeti(d); y[1] = evalsigne(s) | evallgefint(d);
  if (++m == BITS_IN_LONG)
    for (i=2; i<d; i++) y[i]=x[i];
  else
  {
    register const ulong sh = BITS_IN_LONG - m;
    shift_right2(y,x, 2,d,0, sh,m);
  }
  return y;
}

/* integral part */
GEN
floorr(GEN x)
{
  long d,e,i,lx,m;
  GEN y;

  if (signe(x) >= 0) return truncr(x);
  if ((e=expo(x)) < 0) return gen_m1;
  d = (e>>TWOPOTBITS_IN_LONG) + 3;
  m = e & (BITS_IN_LONG-1);
  lx=lg(x); if (d>lx) pari_err(precer, "floorr (precision loss in truncation)");
  y = new_chunk(d);
  if (++m == BITS_IN_LONG)
  {
    for (i=2; i<d; i++) y[i]=x[i];
    i=d; while (i<lx && !x[i]) i++;
    if (i==lx) goto END;
  }
  else
  {
    register const ulong sh = BITS_IN_LONG - m;
    shift_right2(y,x, 2,d,0, sh,m);
    if (x[d-1]<<m == 0)
    {
      i=d; while (i<lx && !x[i]) i++;
      if (i==lx) goto END;
    }
  }
  /* set y:=y+1 */
  for (i=d-1; i>=2; i--) { y[i]++; if (y[i]) goto END; }
  y=new_chunk(1); y[2]=1; d++;
END:
  y[1] = evalsigne(-1) | evallgefint(d);
  y[0] = evaltyp(t_INT) | evallg(d); return y;
}

INLINE int
absi_cmp_lg(GEN x, GEN y, long l)
{
  long i=2;
  while (i<l && x[i]==y[i]) i++;
  if (i==l) return 0;
  return ((ulong)x[i] > (ulong)y[i])? 1: -1;
}

INLINE int
absi_equal_lg(GEN x, GEN y, long l)
{
  long i = l-1; while (i>1 && x[i]==y[i]) i--;
  return i==1;
}

/***********************************************************************/
/**								      **/
/**		          MULTIPLICATION                 	      **/
/**                                                                   **/
/***********************************************************************/
GEN
mulss(long x, long y)
{
  long s,p1;
  GEN z;
  LOCAL_HIREMAINDER;

  if (!x || !y) return gen_0;
  if (x<0) { s = -1; x = -x; } else s=1;
  if (y<0) { s = -s; y = -y; }
  p1 = mulll(x,y);
  if (hiremainder)
  {
    z=cgeti(4); z[1] = evalsigne(s) | evallgefint(4);
    z[2]=hiremainder; z[3]=p1; return z;
  }
  z=cgeti(3); z[1] = evalsigne(s) | evallgefint(3);
  z[2]=p1; return z;
}

GEN
muluu(ulong x, ulong y)
{
  long p1;
  GEN z;
  LOCAL_HIREMAINDER;

  if (!x || !y) return gen_0;
  p1 = mulll(x,y);
  if (hiremainder)
  {
    z=cgeti(4); z[1] = evalsigne(1) | evallgefint(4);
    z[2]=hiremainder; z[3]=p1; return z;
  }
  z=cgeti(3); z[1] = evalsigne(1) | evallgefint(3);
  z[2]=p1; return z;
}

/* assume ny > 0 */
INLINE GEN
muluispec(ulong x, GEN y, long ny)
{
  GEN yd, z = (GEN)avma;
  long lz = ny+3;
  LOCAL_HIREMAINDER;

  (void)new_chunk(lz);
  yd = y + ny; *--z = mulll(x, *--yd);
  while (yd > y) *--z = addmul(x,*--yd);
  if (hiremainder) *--z = hiremainder; else lz--;
  *--z = evalsigne(1) | evallgefint(lz);
  *--z = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)z; return z;
}

/* a + b*|Y| */
GEN
addumului(ulong a, ulong b, GEN Y)
{
  GEN yd,y,z;
  long ny,lz;
  LOCAL_HIREMAINDER;
  LOCAL_OVERFLOW;

  if (!signe(Y)) return utoi(a);

  y = Y+2; z = (GEN)avma;
  ny = lgefint(Y)-2;
  lz = ny+3;

  (void)new_chunk(lz);
  yd = y + ny; *--z = addll(a, mulll(b, *--yd));
  if (overflow) hiremainder++; /* can't overflow */
  while (yd > y) *--z = addmul(b,*--yd);
  if (hiremainder) *--z = hiremainder; else lz--;
  *--z = evalsigne(1) | evallgefint(lz);
  *--z = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)z; return z;
}

GEN muliispec(GEN a, GEN b, long na, long nb);
/*#define KARAMULR_VARIANT*/
#define muliispec_mirror muliispec

/***********************************************************************/
/**								      **/
/**		          DIVISION                       	      **/
/**                                                                   **/
/***********************************************************************/

ulong
umodiu(GEN y, ulong x)
{
  long sy=signe(y),ly,i;
  LOCAL_HIREMAINDER;

  if (!x) pari_err(gdiver);
  if (!sy) return 0;
  ly = lgefint(y);
  if (x <= (ulong)y[2]) hiremainder=0;
  else
  {
    if (ly==3) return (sy > 0)? (ulong)y[2]: x - (ulong)y[2];
    hiremainder=y[2]; ly--; y++;
  }
  for (i=2; i<ly; i++) (void)divll(y[i],x);
  if (!hiremainder) return 0;
  return (sy > 0)? hiremainder: x - hiremainder;
}

/* return |y| \/ x */
GEN
diviu_rem(GEN y, ulong x, ulong *rem)
{
  long ly,i;
  GEN z;
  LOCAL_HIREMAINDER;

  if (!x) pari_err(gdiver);
  if (!signe(y)) { *rem = 0; return gen_0; }

  ly = lgefint(y);
  if (x <= (ulong)y[2]) hiremainder=0;
  else
  {
    if (ly==3) { *rem = (ulong)y[2]; return gen_0; }
    hiremainder=y[2]; ly--; y++;
  }
  z = cgeti(ly); z[1] = evallgefint(ly) | evalsigne(1);
  for (i=2; i<ly; i++) z[i]=divll(y[i],x);
  *rem = hiremainder; return z;
}

GEN
divis_rem(GEN y, long x, long *rem)
{
  long sy=signe(y),ly,s,i;
  GEN z;
  LOCAL_HIREMAINDER;

  if (!x) pari_err(gdiver);
  if (!sy) { *rem=0; return gen_0; }
  if (x<0) { s = -sy; x = -x; } else s = sy;

  ly = lgefint(y);
  if ((ulong)x <= (ulong)y[2]) hiremainder=0;
  else
  {
    if (ly==3) { *rem = itos(y); return gen_0; }
    hiremainder=y[2]; ly--; y++;
  }
  z = cgeti(ly); z[1] = evallgefint(ly) | evalsigne(s);
  for (i=2; i<ly; i++) z[i]=divll(y[i],x);
  if (sy<0) hiremainder = - ((long)hiremainder);
  *rem = (long)hiremainder; return z;
}

GEN
divis(GEN y, long x)
{
  long sy=signe(y),ly,s,i;
  GEN z;
  LOCAL_HIREMAINDER;

  if (!x) pari_err(gdiver);
  if (!sy) return gen_0;
  if (x<0) { s = -sy; x = -x; } else s = sy;

  ly = lgefint(y);
  if ((ulong)x <= (ulong)y[2]) hiremainder=0;
  else
  {
    if (ly==3) return gen_0;
    hiremainder=y[2]; ly--; y++;
  }
  z = cgeti(ly); z[1] = evallgefint(ly) | evalsigne(s);
  for (i=2; i<ly; i++) z[i]=divll(y[i],x);
  return z;
}

GEN
divrr(GEN x, GEN y)
{
  long sx=signe(x), sy=signe(y), lx,ly,lr,e,i,j;
  ulong y0,y1;
  GEN r, r1;

  if (!sy) pari_err(gdiver);
  e = expo(x) - expo(y);
  if (!sx) return real_0_bit(e);
  if (sy<0) sx = -sx;

  lx=lg(x); ly=lg(y);
  if (ly==3)
  {
    ulong k = x[2], l = (lx>3)? x[3]: 0;
    LOCAL_HIREMAINDER;
    if (k < (ulong)y[2]) e--;
    else
    {
      l >>= 1; if (k&1) l |= HIGHBIT;
      k >>= 1;
    }
    r = cgetr(3); r[1] = evalsigne(sx) | evalexpo(e);
    hiremainder=k; r[2]=divll(l,y[2]); return r;
  }

  lr = min(lx,ly); r = new_chunk(lr);
  r1 = r-1;
  r1[1] = 0; for (i=2; i<lr; i++) r1[i]=x[i];
  r1[lr] = (lx>ly)? x[lr]: 0;
  y0 = y[2]; y1 = y[3];
  for (i=0; i<lr-1; i++)
  { /* r1 = r + (i-1) */
    ulong k, qp;
    LOCAL_HIREMAINDER;
    LOCAL_OVERFLOW;

    if ((ulong)r1[1] == y0)
    {
      qp = MAXULONG; k = addll(y0,r1[2]);
    }
    else
    {
      if ((ulong)r1[1] > y0) /* can't happen if i=0 */
      {
        GEN y1 = y+1;
        j = lr-i; r1[j] = subll(r1[j],y1[j]);
	for (j--; j>0; j--) r1[j] = subllx(r1[j],y1[j]);
	j=i; do r[--j]++; while (j && !r[j]);
      }
      hiremainder = r1[1]; overflow = 0;
      qp = divll(r1[2],y0); k = hiremainder;
    }
    if (!overflow)
    {
      long k3 = subll(mulll(qp,y1), r1[3]);
      long k4 = subllx(hiremainder,k);
      while (!overflow && k4) { qp--; k3 = subll(k3,y1); k4 = subllx(k4,y0); }
    }
    j = lr-i+1;
    if (j<ly) (void)mulll(qp,y[j]); else { hiremainder = 0 ; j = ly; }
    for (j--; j>1; j--)
    {
      r1[j] = subll(r1[j], addmul(qp,y[j]));
      hiremainder += overflow;
    }
    if ((ulong)r1[1] != hiremainder)
    {
      if ((ulong)r1[1] < hiremainder)
      {
        qp--;
        j = lr-i-(lr-i>=ly); r1[j] = addll(r1[j], y[j]);
        for (j--; j>1; j--) r1[j] = addllx(r1[j], y[j]);
      }
      else
      {
	r1[1] -= hiremainder;
	while (r1[1])
	{
	  qp++; if (!qp) { j=i; do r[--j]++; while (j && !r[j]); }
          j = lr-i-(lr-i>=ly); r1[j] = subll(r1[j],y[j]);
          for (j--; j>1; j--) r1[j] = subllx(r1[j],y[j]);
	  r1[1] -= overflow;
	}
      }
    }
    *++r1 = qp;
  }
  /* i = lr-1 */
  /* round correctly */
  if ((ulong)r1[1] > (y0>>1))
  {
    j=i; do r[--j]++; while (j && !r[j]);
  }
  r1 = r-1; for (j=i; j>=2; j--) r[j]=r1[j];
  if (r[0] == 0) e--;
  else if (r[0] == 1) { shift_right(r,r, 2,lr, 1,1); }
  else { /* possible only when rounding up to 0x2 0x0 ... */
    r[2] = HIGHBIT; e++;
  }
  r[0] = evaltyp(t_REAL)|evallg(lr);
  r[1] = evalsigne(sx) | evalexpo(e);
  return r;
}

GEN
divri(GEN x, GEN y)
{
  long lx, s = signe(y);
  pari_sp av;
  GEN z;

  if (!s) pari_err(gdiver);
  if (!signe(x)) return real_0_bit(expo(x) - expi(y));
  if (!is_bigint(y)) return divrs(x, s>0? y[2]: -y[2]);

  lx = lg(x); z = cgetr(lx); av = avma;
  affrr(divrr(x, itor(y, lx+1)), z);
  avma = av; return z;
}

/* Integer division x / y: such that sign(r) = sign(x)
 *   if z = ONLY_REM return remainder, otherwise return quotient
 *   if z != NULL set *z to remainder
 *   *z is the last object on stack (and thus can be disposed of with cgiv
 *   instead of gerepile)
 * If *z is zero, we put gen_0 here and no copy.
 * space needed: lx + ly */
GEN
dvmdii(GEN x, GEN y, GEN *z)
{
  long sx=signe(x),sy=signe(y);
  long lx, ly, lz, i, j, sh, lq, lr;
  pari_sp av;
  ulong y0,y1, *xd,*rd,*qd;
  GEN q, r, r1;

  if (!sy) { if (z == ONLY_REM && !sx) return gen_0; pari_err(gdiver); }
  if (!sx)
  {
    if (!z || z == ONLY_REM) return gen_0;
    *z=gen_0; return gen_0;
  }
  lx=lgefint(x);
  ly=lgefint(y); lz=lx-ly;
  if (lz <= 0)
  {
    if (lz == 0)
    {
      for (i=2; i<lx; i++)
        if (x[i] != y[i])
        {
          if ((ulong)x[i] > (ulong)y[i]) goto DIVIDE;
          goto TRIVIAL;
        }
      if (z == ONLY_REM) return gen_0;
      if (z) *z = gen_0;
      if (sx < 0) sy = -sy;
      return stoi(sy);
    }
TRIVIAL:
    if (z == ONLY_REM) return icopy(x);
    if (z) *z = icopy(x);
    return gen_0;
  }
DIVIDE: /* quotient is non-zero */
  av=avma; if (sx<0) sy = -sy;
  if (ly==3)
  {
    LOCAL_HIREMAINDER;
    y0 = y[2];
    if (y0 <= (ulong)x[2]) hiremainder=0;
    else
    {
      hiremainder = x[2]; lx--; x++;
    }
    q = new_chunk(lx); for (i=2; i<lx; i++) q[i]=divll(x[i],y0);
    if (z == ONLY_REM)
    {
      avma=av; if (!hiremainder) return gen_0;
      r=cgeti(3);
      r[1] = evalsigne(sx) | evallgefint(3);
      r[2]=hiremainder; return r;
    }
    q[1] = evalsigne(sy) | evallgefint(lx);
    q[0] = evaltyp(t_INT) | evallg(lx);
    if (!z) return q;
    if (!hiremainder) { *z=gen_0; return q; }
    r=cgeti(3);
    r[1] = evalsigne(sx) | evallgefint(3);
    r[2] = hiremainder; *z=r; return q;
  }

  r1 = new_chunk(lx); sh = bfffo(y[2]);
  if (sh)
  { /* normalize so that highbit(y) = 1 (shift left x and y by sh bits)*/
    register const ulong m = BITS_IN_LONG - sh;
    r = new_chunk(ly);
    shift_left2(r, y,2,ly-1, 0,sh,m); y = r;
    shift_left2(r1,x,2,lx-1, 0,sh,m);
    r1[1] = ((ulong)x[2]) >> m;
  }
  else
  {
    r1[1] = 0; for (j=2; j<lx; j++) r1[j] = x[j];
  }
  x = r1;
  y0 = y[2]; y1 = y[3];
  for (i=0; i<=lz; i++)
  { /* r1 = x + i */
    ulong k, qp;
    LOCAL_HIREMAINDER;
    LOCAL_OVERFLOW;

    if ((ulong)r1[1] == y0)
    {
      qp = MAXULONG; k = addll(y0,r1[2]);
    }
    else
    {
      hiremainder = r1[1]; overflow = 0;
      qp = divll(r1[2],y0); k = hiremainder;
    }
    if (!overflow)
    {
      long k3 = subll(mulll(qp,y1), r1[3]);
      long k4 = subllx(hiremainder,k);
      while (!overflow && k4) { qp--; k3 = subll(k3,y1); k4 = subllx(k4,y0); }
    }
    hiremainder = 0; j = ly;
    for (j--; j>1; j--)
    {
      r1[j] = subll(r1[j], addmul(qp,y[j]));
      hiremainder += overflow;
    }
    if ((ulong)r1[1] < hiremainder)
    {
      qp--;
      j = ly-1; r1[j] = addll(r1[j],y[j]);
      for (j--; j>1; j--) r1[j] = addllx(r1[j],y[j]);
    }
    *++r1 = qp;
  }

  lq = lz+2;
  if (!z)
  {
    qd = (ulong*)av;
    xd = (ulong*)(x + lq);
    if (x[1]) { lz++; lq++; }
    while (lz--) *--qd = *--xd;
    *--qd = evalsigne(sy) | evallgefint(lq);
    *--qd = evaltyp(t_INT) | evallg(lq);
    avma = (pari_sp)qd; return (GEN)qd;
  }

  j=lq; while (j<lx && !x[j]) j++;
  lz = lx-j;
  if (z == ONLY_REM)
  {
    if (lz==0) { avma = av; return gen_0; }
    rd = (ulong*)av; lr = lz+2;
    xd = (ulong*)(x + lx);
    if (!sh) while (lz--) *--rd = *--xd;
    else
    { /* shift remainder right by sh bits */
      const ulong shl = BITS_IN_LONG - sh;
      ulong l;
      xd--;
      while (--lz) /* fill r[3..] */
      {
        l = *xd >> sh;
        *--rd = l | (*--xd << shl);
      }
      l = *xd >> sh;
      if (l) *--rd = l; else lr--;
    }
    *--rd = evalsigne(sx) | evallgefint(lr);
    *--rd = evaltyp(t_INT) | evallg(lr);
    avma = (pari_sp)rd; return (GEN)rd;
  }

  lr = lz+2;
  rd = NULL; /* gcc -Wall */
  if (lz)
  { /* non zero remainder: initialize rd */
    xd = (ulong*)(x + lx);
    if (!sh)
    {
      rd = (ulong*)avma; (void)new_chunk(lr);
      while (lz--) *--rd = *--xd;
    }
    else
    { /* shift remainder right by sh bits */
      const ulong shl = BITS_IN_LONG - sh;
      ulong l;
      rd = (ulong*)x; /* overwrite shifted y */
      xd--;
      while (--lz)
      {
        l = *xd >> sh;
        *--rd = l | (*--xd << shl);
      }
      l = *xd >> sh;
      if (l) *--rd = l; else lr--;
    }
    *--rd = evalsigne(sx) | evallgefint(lr);
    *--rd = evaltyp(t_INT) | evallg(lr);
    rd += lr;
  }
  qd = (ulong*)av;
  xd = (ulong*)(x + lq);
  if (x[1]) lq++;
  j = lq-2; while (j--) *--qd = *--xd;
  *--qd = evalsigne(sy) | evallgefint(lq);
  *--qd = evaltyp(t_INT) | evallg(lq);
  q = (GEN)qd;
  if (lr==2) *z = gen_0;
  else
  { /* rd has been properly initialized: we had lz > 0 */
    while (lr--) *--qd = *--rd;
    *z = (GEN)qd;
  }
  avma = (pari_sp)qd; return q;
}

/* Montgomery reduction.
 * N has k words, assume T >= 0 has less than 2k.
 * Return res := T / B^k mod N, where B = 2^BIL
 * such that 0 <= res < T/B^k + N  and  res has less than k words */
GEN
red_montgomery(GEN T, GEN N, ulong inv)
{
  pari_sp av;
  GEN Te, Td, Ne, Nd, scratch;
  ulong i, j, m, t, d, k = lgefint(N)-2;
  int carry;
  LOCAL_HIREMAINDER;
  LOCAL_OVERFLOW;

  if (k == 0) return gen_0;
  d = lgefint(T)-2; /* <= 2*k */
#ifdef DEBUG
  if (d > 2*k) pari_err(bugparier,"red_montgomery");
#endif
  if (k == 1)
  { /* as below, special cased for efficiency */
    ulong n = (ulong)N[2];
    t = (ulong)T[d+1];
    m = t * inv;
    (void)addll(mulll(m, n), t); /* = 0 */
    t = hiremainder + overflow;
    if (d == 2)
    {
      t = addll((ulong)T[2], t);
      if (overflow) t -= n; /* t > n doesn't fit in 1 word */
    }
    return utoi(t);
  }
  /* assume k >= 2 */
  av = avma; scratch = new_chunk(k<<1); /* >= k + 2: result fits */

  /* copy T to scratch space (pad with zeroes to 2k words) */
  Td = (GEN)av;
  Te = T + (d+2);
  for (i=0; i < d     ; i++) *--Td = *--Te;
  for (   ; i < (k<<1); i++) *--Td = 0;

  Te = (GEN)av; /* 1 beyond end of T mantissa */
  Ne = N + k+2; /* 1 beyond end of N mantissa */

  carry = 0;
  for (i=0; i<k; i++) /* set T := T/B nod N, k times */
  {
    Td = Te; /* one beyond end of (new) T mantissa */
    Nd = Ne;
    m = *--Td * inv; /* solve T + m N = O(B) */

    /* set T := (T + mN) / B */
    Te = Td;
    (void)addll(mulll(m, *--Nd), *Td); /* = 0 */
    for (j=1; j<k; j++)
    {
      hiremainder += overflow;
      t = addll(addmul(m, *--Nd), *--Td); *Td = t;
    }
    overflow += hiremainder;
    t = addll(overflow, *--Td); *Td = t + carry;
    carry = (overflow || (carry && *Td == 0));
  }
  if (carry)
  { /* Td > N overflows (k+1 words), set Td := Td - N */
    Td = Te;
    Nd = Ne;
    t = subll(*--Td, *--Nd); *Td = t;
    while (Td > scratch) { t = subllx(*--Td, *--Nd); *Td = t; }
  }

  /* copy result */
  Td = (GEN)av;
  while (! *scratch && Te > scratch) scratch++; /* strip leading 0s */
  while (Te > scratch) *--Td = *--Te;
  k = (GEN)av - Td; if (!k) return gen_0;
  k += 2;
  *--Td = evalsigne(1) | evallgefint(k);
  *--Td = evaltyp(t_INT) | evallg(k);
#ifdef DEBUG
{
  long l = lgefint(N)-2, s = BITS_IN_LONG*l;
  GEN R = int2n(s);
  GEN res = remii(mulii(T, Fp_inv(R, N)), N);
  if (k > lgefint(N)
    || !equalii(remii(Td,N),res)
    || cmpii(Td, addii(shifti(T, -s), N)) >= 0) pari_err(bugparier,"red_montgomery");
}
#endif
  avma = (pari_sp)Td; return Td;
}

/* EXACT INTEGER DIVISION */

/* assume xy>0, the division is exact and y is odd. Destroy x */
static GEN
diviuexact_i(GEN x, ulong y)
{
  long i, lz, lx;
  ulong q, yinv;
  GEN z, z0, x0, x0min;

  if (y == 1) return icopy(x);
  lx = lgefint(x);
  if (lx == 3) return utoipos((ulong)x[2] / y);
  yinv = invrev(y);
  lz = (y <= (ulong)x[2]) ? lx : lx-1;
  z = new_chunk(lz);
  z0 = z + lz;
  x0 = x + lx; x0min = x + lx-lz+2;

  while (x0 > x0min)
  {
    *--z0 = q = yinv*((ulong)*--x0); /* i-th quotient */
    if (!q) continue;
    /* x := x - q * y */
    { /* update neither lowest word (could set it to 0) nor highest ones */
      register GEN x1 = x0 - 1;
      LOCAL_HIREMAINDER;
      (void)mulll(q,y);
      if (hiremainder)
      {
        if ((ulong)*x1 < hiremainder)
        {
          *x1 -= hiremainder;
          do (*--x1)--; while ((ulong)*x1 == MAXULONG);
        }
        else
          *x1 -= hiremainder;
      }
    }
  }
  i=2; while(!z[i]) i++;
  z += i-2; lz -= i-2;
  z[0] = evaltyp(t_INT)|evallg(lz);
  z[1] = evalsigne(1)|evallg(lz);
  avma = (pari_sp)z; return z;
}

/* assume y != 0 and the division is exact */
GEN
diviuexact(GEN x, ulong y)
{
  pari_sp av;
  long lx, vy, s = signe(x);
  GEN z;
  
  if (!s) return gen_0;
  if (y == 1) return icopy(x);
  lx = lgefint(x);
  if (lx == 3) {
    ulong q = (ulong)x[2] / y;
    return (s > 0)? utoipos(q): utoineg(q);
  }
  av = avma; (void)new_chunk(lx); vy = vals(y);
  if (vy) { 
    y >>= vy;
    if (y == 1) { avma = av; return shifti(x, -vy); }
    x = shifti(x, -vy);
    if (lx == 3) {
      ulong q = (ulong)x[2] / y;
      avma = av;
      return (s > 0)? utoipos(q): utoineg(q);
    }
  } else x = icopy(x);
  avma = av;
  z = diviuexact_i(x, y);
  setsigne(z, s); return z;
}

/* Find z such that x=y*z, knowing that y | x (unchecked)
 * Method: y0 z0 = x0 mod B = 2^BITS_IN_LONG ==> z0 = 1/y0 mod B.
 *    Set x := (x - z0 y) / B, updating only relevant words, and repeat */
GEN
diviiexact(GEN x, GEN y)
{
  long lx, ly, lz, vy, i, ii, sx = signe(x), sy = signe(y);
  pari_sp av;
  ulong y0inv,q;
  GEN z;

  if (!sy) pari_err(gdiver);
  if (!sx) return gen_0;
  lx = lgefint(x);
  if (lx == 3) {
    q = (ulong)x[2] / (ulong)y[2];
    return (sx+sy) ? utoipos(q): utoineg(q);
  }
  vy = vali(y); av = avma;
  (void)new_chunk(lx); /* enough room for z */
  if (vy)
  { /* make y odd */
    y = shifti(y,-vy);
    x = shifti(x,-vy); lx = lgefint(x);
  }
  else x = icopy(x); /* necessary because we destroy x */
  avma = av; /* will erase our x,y when exiting */
  /* now y is odd */
  ly = lgefint(y);
  if (ly == 3)
  {
    x = diviuexact_i(x,(ulong)y[2]); /* x != 0 */
    setsigne(x, (sx+sy)? 1: -1); return x;
  }
  y0inv = invrev(y[ly-1]);
  i=2; while (i<ly && y[i]==x[i]) i++;
  lz = (i==ly || (ulong)y[i] < (ulong)x[i]) ? lx-ly+3 : lx-ly+2;
  z = new_chunk(lz);

  y += ly - 1; /* now y[-i] = i-th word of y */
  for (ii=lx-1,i=lz-1; i>=2; i--,ii--)
  {
    long limj;
    LOCAL_HIREMAINDER;
    LOCAL_OVERFLOW;

    z[i] = q = y0inv*((ulong)x[ii]); /* i-th quotient */
    if (!q) continue;

    /* x := x - q * y */
    (void)mulll(q,y[0]); limj = max(lx - lz, ii+3-ly);
    { /* update neither lowest word (could set it to 0) nor highest ones */
      register GEN x0 = x + (ii - 1), y0 = y - 1, xlim = x + limj;
      for (; x0 >= xlim; x0--, y0--)
      {
        *x0 = subll(*x0, addmul(q,*y0));
        hiremainder += overflow;
      }
      if (hiremainder && limj != lx - lz)
      {
        if ((ulong)*x0 < hiremainder)
        {
          *x0 -= hiremainder;
          do (*--x0)--; while ((ulong)*x0 == MAXULONG);
        }
        else
          *x0 -= hiremainder;
      }
    }
  }
  i=2; while(!z[i]) i++;
  z += i-2; lz -= (i-2);
  z[0] = evaltyp(t_INT)|evallg(lz);
  z[1] = evalsigne((sx+sy)? 1: -1) | evallg(lz);
  avma = (pari_sp)z; return z;
}


/********************************************************************/
/**                                                                **/
/**               INTEGER MULTIPLICATION (KARATSUBA)               **/
/**                                                                **/
/********************************************************************/
/* nx >= ny = num. of digits of x, y (not GEN, see mulii) */
INLINE GEN
muliispec_basecase(GEN x, GEN y, long nx, long ny)
{
  GEN z2e,z2d,yd,xd,ye,zd;
  long p1,lz;
  LOCAL_HIREMAINDER;

  if (!ny) return gen_0;
  zd = (GEN)avma; lz = nx+ny+2;
  (void)new_chunk(lz);
  xd = x + nx;
  yd = y + ny;
  ye = yd; p1 = *--xd;

  *--zd = mulll(p1, *--yd); z2e = zd;
  while (yd > y) *--zd = addmul(p1, *--yd);
  *--zd = hiremainder;

  while (xd > x)
  {
    LOCAL_OVERFLOW;
    yd = ye; p1 = *--xd;

    z2d = --z2e;
    *z2d = addll(mulll(p1, *--yd), *z2d); z2d--;
    while (yd > y)
    {
      hiremainder += overflow;
      *z2d = addll(addmul(p1, *--yd), *z2d); z2d--;
    }
    *--zd = hiremainder + overflow;
  }
  if (*zd == 0) { zd++; lz--; } /* normalize */
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

INLINE GEN
sqrispec_basecase(GEN x, long nx)
{
  GEN z2e,z2d,yd,xd,zd,x0,z0;
  long p1,lz;
  LOCAL_HIREMAINDER;
  LOCAL_OVERFLOW;

  if (!nx) return gen_0;
  zd = (GEN)avma; lz = (nx+1) << 1;
  z0 = new_chunk(lz);
  if (nx == 1)
  {
    *--zd = mulll(*x, *x);
    *--zd = hiremainder; goto END;
  }
  xd = x + nx;

  /* compute double products --> zd */
  p1 = *--xd; yd = xd; --zd;
  *--zd = mulll(p1, *--yd); z2e = zd;
  while (yd > x) *--zd = addmul(p1, *--yd);
  *--zd = hiremainder;

  x0 = x+1;
  while (xd > x0)
  {
    LOCAL_OVERFLOW;
    p1 = *--xd; yd = xd;

    z2e -= 2; z2d = z2e;
    *z2d = addll(mulll(p1, *--yd), *z2d); z2d--;
    while (yd > x)
    {
      hiremainder += overflow;
      *z2d = addll(addmul(p1, *--yd), *z2d); z2d--;
    }
    *--zd = hiremainder + overflow;
  }
  /* multiply zd by 2 (put result in zd - 1) */
  zd[-1] = ((*zd & HIGHBIT) != 0);
  shift_left(zd, zd, 0, (nx<<1)-3, 0, 1);

  /* add the squares */
  xd = x + nx; zd = z0 + lz;
  p1 = *--xd;
  zd--; *zd = mulll(p1,p1);
  zd--; *zd = addll(hiremainder, *zd);
  while (xd > x)
  {
    p1 = *--xd;
    zd--; *zd = addll(mulll(p1,p1)+ overflow, *zd);
    zd--; *zd = addll(hiremainder + overflow, *zd);
  }

END:
  if (*zd == 0) { zd++; lz--; } /* normalize */
  *--zd = evalsigne(1) | evallgefint(lz);
  *--zd = evaltyp(t_INT) | evallg(lz);
  avma=(pari_sp)zd; return zd;
}

/* return (x shifted left d words) + y. Assume d > 0, x > 0 and y >= 0 */
static GEN
addshiftw(GEN x, GEN y, long d)
{
  GEN z,z0,y0,yd, zd = (GEN)avma;
  long a,lz,ly = lgefint(y);

  z0 = new_chunk(d);
  a = ly-2; yd = y+ly;
  if (a >= d)
  {
    y0 = yd-d; while (yd > y0) *--zd = *--yd; /* copy last d words of y */
    a -= d;
    if (a)
      z = addiispec(x+2, y+2, lgefint(x)-2, a);
    else
      z = icopy(x);
  }
  else
  {
    y0 = yd-a; while (yd > y0) *--zd = *--yd; /* copy last a words of y */
    while (zd >= z0) *--zd = 0;    /* complete with 0s */
    z = icopy(x);
  }
  lz = lgefint(z)+d;
  z[1] = evalsigne(1) | evallgefint(lz);
  z[0] = evaltyp(t_INT) | evallg(lz); return z;
}

/* Fast product (Karatsuba) of integers. a and b are "special" GENs
 * c,c0,c1,c2 are genuine GENs.
 */
GEN
muliispec(GEN a, GEN b, long na, long nb)
{
  GEN a0,c,c0;
  long n0, n0a, i;
  pari_sp av;

  if (na < nb) swapspec(a,b, na,nb);
  if (nb == 1) return muluispec((ulong)*b, a, na);
  if (nb == 0) return gen_0;
  if (nb < KARATSUBA_MULI_LIMIT) return muliispec_basecase(a,b,na,nb);
  i=(na>>1); n0=na-i; na=i;
  av=avma; a0=a+na; n0a=n0;
  while (!*a0 && n0a) { a0++; n0a--; }

  if (n0a && nb > n0)
  { /* nb <= na <= n0 */
    GEN b0,c1,c2;
    long n0b;

    nb -= n0;
    c = muliispec(a,b,na,nb);
    b0 = b+nb; n0b = n0;
    while (!*b0 && n0b) { b0++; n0b--; }
    if (n0b)
    {
      c0 = muliispec(a0,b0, n0a,n0b);

      c2 = addiispec(a0,a, n0a,na);
      c1 = addiispec(b0,b, n0b,nb);
      c1 = muliispec(c1+2,c2+2, lgefint(c1)-2,lgefint(c2)-2);
      c2 = addiispec(c0+2, c+2, lgefint(c0)-2,lgefint(c) -2);

      c1 = subiispec(c1+2,c2+2, lgefint(c1)-2,lgefint(c2)-2);
    }
    else
    {
      c0 = gen_0;
      c1 = muliispec(a0,b, n0a,nb);
    }
    c = addshiftw(c,c1, n0);
  }
  else
  {
    c = muliispec(a,b,na,nb);
    c0 = muliispec(a0,b,n0a,nb);
  }
  return gerepileuptoint(av, addshiftw(c,c0, n0));
}

/* x % (2^n), assuming x, n >= 0 */
GEN
resmod2n(GEN x, long n)
{
  long hi,l,k,lx,ly;
  GEN z, xd, zd;

  if (!signe(x) || !n) return gen_0;

  l = n & (BITS_IN_LONG-1);    /* n % BITS_IN_LONG */
  k = n >> TWOPOTBITS_IN_LONG; /* n / BITS_IN_LONG */
  lx = lgefint(x);
  if (lx < k+3) return icopy(x);

  xd = x + (lx-k-1);
  /* x = |_|...|#|1|...|k| : copy the last l bits of # and the last k words
   *            ^--- initial xd  */
  hi = ((ulong)*xd) & ((1UL<<l)-1); /* last l bits of # = top bits of result */
  if (!hi)
  { /* strip leading zeroes from result */
    xd++; while (k && !*xd) { k--; xd++; }
    if (!k) return gen_0;
    ly = k+2; xd--;
  }
  else
    ly = k+3;

  zd = z = cgeti(ly);
  *++zd = evalsigne(1) | evallgefint(ly);
  if (hi) *++zd = hi;
  for ( ;k; k--) *++zd = *++xd;
  return z;
}

GEN
sqrispec(GEN a, long na)
{
  GEN a0,c;
  long n0, n0a, i;
  pari_sp av;

  if (na < KARATSUBA_SQRI_LIMIT) return sqrispec_basecase(a,na);
  i=(na>>1); n0=na-i; na=i;
  av=avma; a0=a+na; n0a=n0;
  while (!*a0 && n0a) { a0++; n0a--; }
  c = sqrispec(a,na);
  if (n0a)
  {
    GEN t, c1, c0 = sqrispec(a0,n0a);
#if 0
    c1 = shifti(muliispec(a0,a, n0a,na),1);
#else /* faster */
    t = addiispec(a0,a,n0a,na);
    t = sqrispec(t+2,lgefint(t)-2);
    c1= addiispec(c0+2,c+2, lgefint(c0)-2, lgefint(c)-2);
    c1= subiispec(t+2, c1+2, lgefint(t)-2, lgefint(c1)-2);
#endif
    c = addshiftw(c,c1, n0);
    c = addshiftw(c,c0, n0);
  }
  else
    c = addshiftw(c,gen_0,n0<<1);
  return gerepileuptoint(av, c);
}

/********************************************************************/
/**                                                                **/
/**                    KARATSUBA SQUARE ROOT                       **/
/**      adapted from Paul Zimmermann's implementation of          **/
/**      his algorithm in GMP (mpn_sqrtrem)                        **/
/**                                                                **/
/********************************************************************/

/* Square roots table */
static const unsigned char approx_tab[192] = {
  128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,
  143,144,144,145,146,147,148,149,150,150,151,152,153,154,155,155,
  156,157,158,159,160,160,161,162,163,163,164,165,166,167,167,168,
  169,170,170,171,172,173,173,174,175,176,176,177,178,178,179,180,
  181,181,182,183,183,184,185,185,186,187,187,188,189,189,190,191,
  192,192,193,193,194,195,195,196,197,197,198,199,199,200,201,201,
  202,203,203,204,204,205,206,206,207,208,208,209,209,210,211,211,
  212,212,213,214,214,215,215,216,217,217,218,218,219,219,220,221,
  221,222,222,223,224,224,225,225,226,226,227,227,228,229,229,230,
  230,231,231,232,232,233,234,234,235,235,236,236,237,237,238,238,
  239,240,240,241,241,242,242,243,243,244,244,245,245,246,246,247,
  247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255
};

/* N[0], assume N[0] >= 2^(BIL-2).
 * Return r,s such that s^2 + r = N, 0 <= r <= 2s */
static void
p_sqrtu1(ulong *N, ulong *ps, ulong *pr)
{
  ulong prec, r, s, q, u, n0 = N[0];

  q = n0 >> (BITS_IN_LONG - 8);
  /* 2^6 = 64 <= q < 256 = 2^8 */
  s = approx_tab[q - 64];				/* 128 <= s < 255 */
  r = (n0 >> (BITS_IN_LONG - 16)) - s * s;		/* r <= 2*s */
  if (r > (s << 1)) { r -= (s << 1) | 1; s++; }

  /* 8-bit approximation from the high 8-bits of N[0] */
  prec = 8;
  n0 <<= 2 * prec;
  while (2 * prec < BITS_IN_LONG)
  { /* invariant: s has prec bits, and r <= 2*s */
    r = (r << prec) + (n0 >> (BITS_IN_LONG - prec));
    n0 <<= prec;
    u = 2 * s;
    q = r / u; u = r - q * u;
    s = (s << prec) + q;
    u = (u << prec) + (n0 >> (BITS_IN_LONG - prec));
    q = q * q;
    r = u - q;
    if (u < q) { s--; r += (s << 1) | 1; }
    n0 <<= prec;
    prec = 2 * prec;
  }
  *ps = s;
  *pr = r;
}

/* N[0..1], assume N[0] >= 2^(BIL-2).
 * Return 1 if remainder overflows, 0 otherwise */
static int
p_sqrtu2(ulong *N, ulong *ps, ulong *pr)
{
  ulong cc, qhl, r, s, q, u, n1 = N[1];
  LOCAL_OVERFLOW;

  p_sqrtu1(N, &s, &r); /* r <= 2s */
  qhl = 0; while (r >= s) { qhl++; r -= s; }
  /* now r < s < 2^(BIL/2) */
  r = (r << BITS_IN_HALFULONG) | (n1 >> BITS_IN_HALFULONG);
  u = s << 1;
  q = r / u; u = r - q * u;
  q += (qhl & 1) << (BITS_IN_HALFULONG - 1);
  qhl >>= 1;
  /* (initial r)<<(BIL/2) + n1>>(BIL/2) = (qhl<<(BIL/2) + q) * 2s + u */
  s = ((s + qhl) << BITS_IN_HALFULONG) + q;
  cc = u >> BITS_IN_HALFULONG;
  r = (u << BITS_IN_HALFULONG) | (n1 & LOWMASK);
  r = subll(r, q * q);
  cc -= overflow + qhl;
  /* now subtract 2*q*2^(BIL/2) + 2^BIL if qhl is set */
  if ((long)cc < 0)
  {
    if (s) {
      r = addll(r, s);
      cc += overflow;
      s--;
    } else {
      cc++;
      s = ~0UL;
    }
    r = addll(r, s);
    cc += overflow;
  }
  *ps = s;
  *pr = r; return cc;
}

static void
xmpn_zero(GEN x, long n)
{
  while (--n >= 0) x[n]=0;
}
static void
xmpn_copy(GEN z, GEN x, long n)
{
  long k = n;
  while (--k >= 0) z[k] = x[k];
}
static GEN
cat1u(ulong d)
{
  GEN R = cgeti(4);
  R[1] = evalsigne(1)|evallgefint(4);
  R[2] = 1;
  R[3] = d; return R;
}
/* a[0..la-1] * 2^(lb BIL) | b[0..lb-1] */
static GEN
catii(GEN a, long la, GEN b, long lb)
{
  long l = la + lb + 2;
  GEN z = cgeti(l);
  z[1] = evalsigne(1) | evallgefint(l);
  xmpn_copy(z + 2, a, la);
  xmpn_copy(z + 2 + la, b, lb);
  return int_normalize(z, 0);
}

/* sqrt n[0..1], assume n normalized */
static GEN
sqrtispec2(GEN n, GEN *pr)
{
  ulong s, r;
  int hi = p_sqrtu2((ulong*)n, &s, &r);
  GEN S = utoi(s);
  *pr = hi? cat1u(r): utoi(r);
  return S;
}

/* sqrt n[0], _dont_ assume n normalized */
static GEN
sqrtispec1_sh(GEN n, GEN *pr)
{
  GEN S;
  ulong r, s, u0 = (ulong)n[0];
  int sh = bfffo(u0) & ~1UL;
  if (sh) u0 <<= sh;
  p_sqrtu1(&u0, &s, &r);
  /* s^2 + r = u0, s < 2^(BIL/2). Rescale back:
   * 2^(2k) n = S^2 + R
   * so 2^(2k) n = (S - s0)^2 + (2*S*s0 - s0^2 + R), s0 = S mod 2^k. */
  if (sh) {
    int k = sh >> 1;
    ulong s0 = s & ((1<<k) - 1);
    r += s * (s0<<1);
    s >>= k;
    r >>= sh;
  }
  S = utoi(s);
  if (pr) *pr = utoi(r);
  return S;
}

/* sqrt n[0..1], _dont_ assume n normalized */
static GEN
sqrtispec2_sh(GEN n, GEN *pr)
{
  GEN S;
  ulong U[2], r, s, u0 = (ulong)n[0], u1 = (ulong)n[1];
  int hi, sh = bfffo(u0) & ~1UL;
  if (sh) {
    u0 = (u0 << sh) | (u1 >> (BITS_IN_LONG-sh));
    u1 <<= sh;
  }
  U[0] = u0;
  U[1] = u1; hi = p_sqrtu2(U, &s, &r);
  /* s^2 + R = u0|u1. Rescale back:
   * 2^(2k) n = S^2 + R
   * so 2^(2k) n = (S - s0)^2 + (2*S*s0 - s0^2 + R), s0 = S mod 2^k. */
  if (sh) {
    int k = sh >> 1;
    ulong s0 = s & ((1<<k) - 1);
    LOCAL_HIREMAINDER;
    LOCAL_OVERFLOW;
    r = addll(r, mulll(s, (s0<<1)));
    if (overflow) hiremainder++;
    hiremainder += hi; /* + 0 or 1 */
    s >>= k;
    r = (r>>sh) | (hiremainder << (BITS_IN_LONG-sh));
    hi = (hiremainder & (1<<sh));
  }
  S = utoi(s);
  if (pr) *pr = hi? cat1u(r): utoi(r);
  return S;
}

/* Let N = N[0..2n-1]. Return S (and set R) s.t S^2 + R = N, 0 <= R <= 2S
 * Assume N normalized */
static GEN
sqrtispec(GEN N, long n, GEN *r)
{
  GEN S, R, q, z, u;
  long l, h;

  if (n == 1) return sqrtispec2(N, r);
  l = n >> 1;
  h = n - l; /* N = a3(h) | a2(h) | a1(l) | a0(l words) */
  S = sqrtispec(N, h, &R); /* S^2 + R = a3|a2 */

  z = catii(R+2, lgefint(R)-2, N + 2*h, l); /* = R | a1(l) */
  q = dvmdii(z, shifti(S,1), &u);
  z = catii(u+2, lgefint(u)-2, N + n + h, l); /* = u | a0(l) */

  S = addshiftw(S, q, l);
  R = subii(z, sqri(q));
  if (signe(R) < 0)
  {
    GEN S2 = shifti(S,1);
    R = addis(subiispec(S2+2, R+2, lgefint(S2)-2,lgefint(R)-2), -1);
    S = addis(S, -1);
  }
  *r = R; return S;
}

/* Return S (and set R) s.t S^2 + R = N, 0 <= R <= 2S.
 * As for dvmdii, R is last on stack and guaranteed to be gen_0 in case the
 * remainder is 0. R = NULL is allowed. */
GEN
sqrtremi(GEN N, GEN *r)
{
  pari_sp av;
  GEN S, R, n = N+2;
  long k, l2, ln = lgefint(N) - 2;
  int sh;

  if (ln <= 2)
  {
    if (ln == 2) return sqrtispec2_sh(n, r);
    if (ln == 1) return sqrtispec1_sh(n, r);
    if (r) *r = gen_0;
    return gen_0;
  }
  av = avma;
  sh = bfffo(n[0]) >> 1;
  l2 = (ln + 1) >> 1;
  if (sh || (ln & 1)) { /* normalize n, so that n[0] >= 2^BIL / 4 */
    GEN s0, t = new_chunk(ln + 1);
    t[ln] = 0;
    if (sh)
    { shift_left(t, n, 0,ln-1, 0, (sh << 1)); }
    else
      xmpn_copy(t, n, ln);
    S = sqrtispec(t, l2, &R); /* t normalized, 2 * l2 words */
    /* Rescale back:
     * 2^(2k) n = S^2 + R, k = sh + (ln & 1)*BIL/2
     * so 2^(2k) n = (S - s0)^2 + (2*S*s0 - s0^2 + R), s0 = S mod 2^k. */
    k = sh + (ln & 1) * (BITS_IN_LONG/2);
    s0 = resmod2n(S, k);
    R = addii(shifti(R,-1), mulii(s0, S));
    R = shifti(R, 1 - (k<<1));
    S = shifti(S, -k);
  }
  else
    S = sqrtispec(n, l2, &R);

  if (!r) { avma = (pari_sp)S; return gerepileuptoint(av, S); }
  gerepileall(av, 2, &S, &R); *r = R; return S;
}

/* compute sqrt(|a|), assuming a != 0 */

#if 1
GEN
sqrtr_abs(GEN x)
{
  long l = lg(x) - 2, e = expo(x), er = e>>1;
  GEN b, c, res = cgetr(2 + l);
  res[1] = evalsigne(1) | evalexpo(er);
  if (e&1) {
    b = new_chunk(l << 1);
    xmpn_copy(b, x+2, l);
    xmpn_zero(b + l,l);
    b = sqrtispec(b, l, &c);
    xmpn_copy(res+2, b+2, l);
    if (cmpii(c, b) > 0) roundr_up_ip(res, l+2);
  } else {
    ulong u;
    b = new_chunk(2 + (l << 1));
    shift_left(b+1, x+2, 0,l-1, 0, BITS_IN_LONG-1);
    b[0] = ((ulong)x[2])>>1;
    xmpn_zero(b + l+1,l+1);
    b = sqrtispec(b, l+1, &c);
    xmpn_copy(res+2, b+2, l);
    u = (ulong)b[l+2];
    if ( u&HIGHBIT || (u == ~HIGHBIT && cmpii(c,b) > 0))
      roundr_up_ip(res, l+2);
  }
  avma = (pari_sp)res; return res;
}

#else /* use t_REAL: currently much slower (quadratic division) */

#ifdef LONG_IS_64BIT
/* 64 bits of b = sqrt(a[0] * 2^64 + a[1])  [ up to 1ulp ] */
static ulong
sqrtu2(ulong *a)
{
  ulong c, b = dblmantissa( sqrt((double)a[0]) );
  LOCAL_HIREMAINDER;
  LOCAL_OVERFLOW;

  /* > 32 correct bits, 1 Newton iteration to reach 64 */
  if (b <= a[0]) return HIGHBIT | (a[0] >> 1);
  hiremainder = a[0]; c = divll(a[1], b);
  return (addll(c, b) >> 1) | HIGHBIT;
}
/* 64 bits of sqrt(a[0] * 2^63) */
static ulong
sqrtu2_1(ulong *a)
{
  ulong t[2];
  t[0] = (a[0] >> 1);
  t[1] = (a[0] << (BITS_IN_LONG-1)) | (a[1] >> 1);
  return sqrtu2(t);
}
#else
/* 32 bits of sqrt(a[0] * 2^32) */
static ulong
sqrtu2(ulong *a)   { return dblmantissa( sqrt((double)a[0]) ); }
/* 32 bits of sqrt(a[0] * 2^31) */
static ulong
sqrtu2_1(ulong *a) { return dblmantissa( sqrt(2. * a[0]) ); }
#endif

GEN
sqrtr_abs(GEN x)
{
  long l1, i, l = lg(x), ex = expo(x);
  GEN a, t, y = cgetr(l);
  pari_sp av, av0 = avma;

  a = cgetr(l+1); affrr(x,a);
  t = cgetr(l+1);
  if (ex & 1) { /* odd exponent */
    a[1] = evalsigne(1) | evalexpo(1);
    t[2] = (long)sqrtu2((ulong*)a + 2);
  } else { /* even exponent */
    a[1] = evalsigne(1) | evalexpo(0);
    t[2] = (long)sqrtu2_1((ulong*)a + 2);
  }
  t[1] = evalsigne(1) | evalexpo(0);
  for (i = 3; i <= l; i++) t[i] = 0;

  /* |x| = 2^(ex/2) a, t ~ sqrt(a) */
  l--; l1 = 1; av = avma;
  while (l1 < l) { /* let t := (t + a/t)/2 */
    l1 <<= 1; if (l1 > l) l1 = l;
    setlg(a, l1 + 2);
    setlg(t, l1 + 2);
    affrr(addrr(t, divrr(a,t)), t); setexpo(t, expo(t)-1);
    avma = av;
  }
  affrr(t,y); setexpo(y, expo(y) + (ex>>1));
  avma = av0; return y;
}

#endif
#line 2 "../src/kernel/none/cmp.c"
/* $Id: cmp.c 6490 2005-01-13 21:50:15Z kb $

Copyright (C) 2002-2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */


/********************************************************************/
/**                                                                **/
/**                      Comparison routines                       **/
/**                                                                **/
/********************************************************************/

/*They depend on absi_cmp_lg and absi_equal_lg in mp.c*/

#define MASK(x) (((ulong)(x)) & (LGBITS | SIGNBITS))
int
equalii(GEN x, GEN y)
{
  if (MASK(x[1]) != MASK(y[1])) return 0;
  return absi_equal_lg(x, y, lgefint(x));
}
#undef MASK

/* x == y ? */
int
equalsi(long x, GEN y)
{
  if (!x) return !signe(y);
  if (x > 0)
  {
    if (signe(y) <= 0 || lgefint(y) != 3) return 0;
    return ((ulong)y[2] == (ulong)x);
  }
  if (signe(y) >= 0 || lgefint(y) != 3) return 0;
  return ((ulong)y[2] == (ulong)-x);
}
/* x == |y| ? */
int
equalui(ulong x, GEN y)
{
  if (!x) return !signe(y);
  return (lgefint(y) == 3 && (ulong)y[2] == x);
}

int
cmpsi(long x, GEN y)
{
  ulong p;

  if (!x) return -signe(y);

  if (x>0)
  {
    if (signe(y)<=0) return 1;
    if (lgefint(y)>3) return -1;
    p=y[2]; if (p == (ulong)x) return 0;
    return p < (ulong)x ? 1 : -1;
  }

  if (signe(y)>=0) return -1;
  if (lgefint(y)>3) return 1;
  p=y[2]; if (p == (ulong)-x) return 0;
  return p < (ulong)(-x) ? -1 : 1;
}

/* compare x and |y| */
int
cmpui(ulong x, GEN y)
{
  long l = lgefint(y);
  ulong p;

  if (!x) return (l > 2)? -1: 0;
  if (l == 2) return 1;
  if (l > 3) return -1;
  p = y[2]; if (p == x) return 0;
  return p < x ? 1 : -1;
}

int
cmpii(GEN x, GEN y)
{
  const long sx = signe(x), sy = signe(y);
  long lx,ly;

  if (sx<sy) return -1;
  if (sx>sy) return 1;
  if (!sx) return 0;

  lx=lgefint(x); ly=lgefint(y);
  if (lx>ly) return sx;
  if (lx<ly) return -sx;
  if (sx>0)
    return absi_cmp_lg(x, y, lx);
  else
    return -absi_cmp_lg(x, y, lx);
}

int
cmprr(GEN x, GEN y)
{
  const long sx = signe(x), sy = signe(y);
  long ex,ey,lx,ly,lz,i;

  if (sx<sy) return -1;
  if (sx>sy) return 1;
  if (!sx) return 0;

  ex=expo(x); ey=expo(y);
  if (ex>ey) return sx;
  if (ex<ey) return -sx;

  lx=lg(x); ly=lg(y); lz = (lx<ly)?lx:ly;
  i=2; while (i<lz && x[i]==y[i]) i++;
  if (i<lz) return ((ulong)x[i] > (ulong)y[i]) ? sx : -sx;
  if (lx>=ly)
  {
    while (i<lx && !x[i]) i++;
    return (i==lx) ? 0 : sx;
  }
  while (i<ly && !y[i]) i++;
  return (i==ly) ? 0 : -sx;
}

/* x and y are integers. Return 1 if |x| == |y|, 0 otherwise */
int
absi_equal(GEN x, GEN y)
{
  long lx;

  if (!signe(x)) return !signe(y);
  if (!signe(y)) return 0;

  lx=lgefint(x); if (lx != lgefint(y)) return 0;
  return absi_equal_lg(x, y, lx);
}

/* x and y are integers. Return sign(|x| - |y|) */
int
absi_cmp(GEN x, GEN y)
{
  long lx,ly;

  if (!signe(x)) return signe(y)? -1: 0;
  if (!signe(y)) return 1;

  lx=lgefint(x); ly=lgefint(y);
  if (lx>ly) return 1;
  if (lx<ly) return -1;
  return absi_cmp_lg(x, y, lx);
}

/* x and y are reals. Return sign(|x| - |y|) */
int
absr_cmp(GEN x, GEN y)
{
  long ex,ey,lx,ly,lz,i;

  if (!signe(x)) return signe(y)? -1: 0;
  if (!signe(y)) return 1;

  ex=expo(x); ey=expo(y);
  if (ex>ey) return  1;
  if (ex<ey) return -1;

  lx=lg(x); ly=lg(y); lz = (lx<ly)?lx:ly;
  i=2; while (i<lz && x[i]==y[i]) i++;
  if (i<lz) return ((ulong)x[i] > (ulong)y[i])? 1: -1;
  if (lx>=ly)
  {
    while (i<lx && !x[i]) i++;
    return (i==lx)? 0: 1;
  }
  while (i<ly && !y[i]) i++;
  return (i==ly)? 0: -1;
}

#line 2 "../src/kernel/none/gcdll.c"
/* $Id: gcdll.c 7835 2006-04-08 11:56:51Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/***********************************************************************/
/**								      **/
/**		          GCD                            	      **/
/**                                                                   **/
/***********************************************************************/

/* Ultra-fast private ulong gcd for trial divisions.  Called with y odd;
   x can be arbitrary (but will most of the time be smaller than y).
   Will also be used from inside ifactor2.c, so it's `semi-private' really.
   --GN */

/* Gotos are Harmful, and Programming is a Science.  E.W.Dijkstra. */
ulong
ugcd(ulong x, ulong y)         /* assume y&1==1, y > 1 */
{
  if (!x) return y;
  /* fix up x */
  while (!(x&1)) x>>=1;
  if (x==1) return 1;
  if (x==y) return y;
  else if (x>y) goto xislarger;/* will be rare, given how we'll use this */
  /* loop invariants: x,y odd and distinct. */
 yislarger:
  if ((x^y)&2)                 /* ...01, ...11 or vice versa */
    y=(x>>2)+(y>>2)+1;         /* ==(x+y)>>2 except it can't overflow */
  else                         /* ...01,...01 or ...11,...11 */
    y=(y-x)>>2;                /* now y!=0 in either case */
  while (!(y&1)) y>>=1;        /* kill any windfall-gained powers of 2 */
  if (y==1) return 1;          /* comparand == return value... */
  if (x==y) return y;          /* this and the next is just one comparison */
  else if (x<y) goto yislarger;/* else fall through to xislarger */

 xislarger:                    /* same as above, seen through a mirror */
  if ((x^y)&2)
    x=(x>>2)+(y>>2)+1;
  else
    x=(x-y)>>2;                /* x!=0 */
  while (!(x&1)) x>>=1;
  if (x==1) return 1;
  if (x==y) return y;
  else if (x>y) goto xislarger;

  goto yislarger;
}
/* Gotos are useful, and Programming is an Art.  D.E.Knuth. */
/* PS: Of course written with Dijkstra's lessons firmly in mind... --GN */

/* modified right shift binary algorithm with at most one division */
long
cgcd(long a,long b)
{
  long v;
  a=labs(a); if (!b) return a;
  b=labs(b); if (!a) return b;
  if (a>b) { a %= b; if (!a) return b; }
  else { b %= a; if (!b) return a; }
  v=vals(a|b); a>>=v; b>>=v;
  if (a==1 || b==1) return 1L<<v;
  if (b&1)
    return ((long)ugcd((ulong)a, (ulong)b)) << v;
  else
    return ((long)ugcd((ulong)b, (ulong)a)) << v;
}

/*Warning: will overflow silently if lcm does not fit*/
long
clcm(long a,long b)
{
  long d;
  if(!a) return 0;
  d=cgcd(a,b);
  if(d!=1) return a*(b/d);
  return a*b;
}

/* assume a>b>0, return gcd(a,b) as a GEN (for gcdii) */
static GEN
gcduu(ulong a, ulong b)
{
  GEN r = cgeti(3);
  long v;

  r[1] = evalsigne(1)|evallgefint(3);
  a %= b; if (!a) { r[2]=(long)b; return r; }
  v=vals(a|b); a>>=v; b>>=v;
  if (a==1 || b==1) { r[2]=(long)(1UL<<v); return r; }
  if (b&1)
    r[2] = (long)(ugcd((ulong)a, (ulong)b) << v);
  else
    r[2] = (long)(ugcd((ulong)b, (ulong)a) << v);
  return r;
}

/********************************************************************/
/**                                                                **/
/**               INTEGER EXTENDED GCD  (AND INVMOD)               **/
/**                                                                **/
/********************************************************************/

/* GN 1998Oct25, originally developed in January 1998 under 2.0.4.alpha,
 * in the context of trying to improve elliptic curve cryptosystem attacking
 * algorithms.  2001Jan02 -- added bezout() functionality.
 *
 * Two basic ideas - (1) avoid many integer divisions, especially when the
 * quotient is 1 (which happens more than 40% of the time).  (2) Use Lehmer's
 * trick as modified by Jebelean of extracting a couple of words' worth of
 * leading bits from both operands, and compute partial quotients from them
 * as long as we can be sure of their values.  The Jebelean modifications
 * consist in reliable inequalities from which we can decide fast whether
 * to carry on or to return to the outer loop, and in re-shifting after the
 * first word's worth of bits has been used up.  All of this is described
 * in R. Lercier's these [pp148-153 & 163f.], except his outer loop isn't
 * quite right  (the catch-up divisions needed when one partial quotient is
 * larger than a word are missing).
 *
 * The API consists of invmod() and bezout() below;  the single-word routines
 * xgcduu and xxgcduu may be called directly if desired;  lgcdii() probably
 * doesn't make much sense out of context.
 *
 * The whole lot is a factor 6 .. 8 faster on word-sized operands, and asym-
 * ptotically about a factor 2.5 .. 3, depending on processor architecture,
 * than the naive continued-division code.  Unfortunately, thanks to the
 * unrolled loops and all, the code is a bit lengthy.
 */

/*==================================
 * xgcduu(d,d1,f,v,v1,s)
 * xxgcduu(d,d1,f,u,u1,v,v1,s)
 * rgcduu(d,d1,vmax,u,u1,v,v1,s)
 *==================================*/
/*
 * Fast `final' extended gcd algorithm, acting on two ulongs.  Ideally this
 * should be replaced with assembler versions wherever possible.  The present
 * code essentially does `subtract, compare, and possibly divide' at each step,
 * which is reasonable when hardware division (a) exists, (b) is a bit slowish
 * and (c) does not depend a lot on the operand values (as on i486).  When
 * wordsize division is in fact an assembler routine based on subtraction,
 * this strategy may not be the most efficient one.
 *
 * xxgcduu() should be called with  d > d1 > 0, returns gcd(d,d1), and assigns
 * the usual signless cont.frac. recurrence matrix to [u, u1; v, v1]  (i.e.,
 * the product of all the [0, 1; 1 q_j] where the leftmost factor arises from
 * the quotient of the first division step),  and the information about the
 * implied signs to s  (-1 when an odd number of divisions has been done,
 * 1 otherwise).  xgcduu() is exactly the same except that u,u1 are not com-
 * puted (and not returned, of course).
 *
 * The input flag f should be set to 1 if we know in advance that gcd(d,d1)==1
 * (so we can stop the chain division one step early:  as soon as the remainder
 * equals 1).  Use this when you intend to use only what would be v, or only
 * what would be u and v, after that final division step, but not u1 and v1.
 * With the flag in force and thus without that final step, the interesting
 * quantity/ies will still sit in [u1 and] v1, of course.
 *
 * For computing the inverse of a single-word INTMOD known to exist, pass f=1
 * to xgcduu(), and obtain the result from s and v1.  (The routine does the
 * right thing when d1==1 already.)  For finishing a multiword modinv known
 * to exist, pass f=1 to xxgcduu(), and multiply the returned matrix  (with
 * properly adjusted signs)  onto the values v' and v1' previously obtained
 * from the multiword division steps.  Actually, just take the scalar product
 * of [v',v1'] with [u1,-v1], and change the sign if s==-1.  (If the final
 * step had been carried out, it would be [-u,v], and s would also change.)
 * For reducing a rational number to lowest terms, pass f=0 to xgcduu().
 * Finally, f=0 with xxgcduu() is useful for Bezout computations.
 * [Harrumph.  In the above prescription, the sign turns out to be precisely
 * wrong.]
 * (It is safe for invmod() to call xgcduu() with f=1, because f&1 doesn't
 * make a difference when gcd(d,d1)>1.  The speedup is negligible.)
 *
 * In principle, when gcd(d,d1) is known to be 1, it is straightforward to
 * recover the final u,u1 given only v,v1 and s.  However, it probably isn't
 * worthwhile, as it trades a few multiplications for a division.
 *
 * Note that these routines do not know and do not need to know about the
 * PARI stack.
 *
 * Added 2001Jan15:
 * rgcduu() is a variant of xxgcduu() which does not have f  (the effect is
 * that of f=0),  but instead has a ulong vmax parameter, for use in rational
 * reconstruction below.  It returns when v1 exceeds vmax;  v will never
 * exceed vmax.  (vmax=0 is taken as a synonym of MAXULONG i.e. unlimited,
 * in which case rgcduu behaves exactly like xxgcduu with f=0.)  The return
 * value of rgcduu() is typically meaningless;  the interesting part is the
 * matrix.
 */

ulong
xgcduu(ulong d, ulong d1, int f, ulong* v, ulong* v1, long *s)
{
  ulong xv,xv1, xs, q,res;
  LOCAL_HIREMAINDER;

  /* The above blurb contained a lie.  The main loop always stops when d1
   * has become equal to 1.  If (d1 == 1 && !(f&1)) after the loop, we do
   * the final `division' of d by 1 `by hand' as it were.
   *
   * The loop has already been unrolled once.  Aggressive optimization could
   * well lead to a totally unrolled assembler version...
   *
   * On modern x86 architectures, this loop is a pig anyway.  The division
   * instruction always puts its result into the same pair of registers, and
   * we always want to use one of them straight away, so pipeline performance
   * will suck big time.  An assembler version should probably do a first loop
   * computing and storing all the quotients -- their number is bounded in
   * advance -- and then assembling the matrix in a second pass.  On other
   * architectures where we can cycle through four or so groups of registers
   * and exploit a fast ALU result-to-operand feedback path, this is much less
   * of an issue.  (Intel sucks.  See http://www.x86.org/ ...)
   */
  xs = res = 0;
  xv = 0UL; xv1 = 1UL;
  while (d1 > 1UL)
  {
    d -= d1;			/* no need to use subll */
    if (d >= d1)
    {
      hiremainder = 0; q = 1 + divll(d,d1); d = hiremainder;
      xv += q * xv1;
    }
    else
      xv += xv1;
				/* possible loop exit */
    if (d <= 1UL) { xs=1; break; }
				/* repeat with inverted roles */
    d1 -= d;
    if (d1 >= d)
    {
      hiremainder = 0; q = 1 + divll(d1,d); d1 = hiremainder;
      xv1 += q * xv;
    }
    else
      xv1 += xv;
  } /* while */

  if (!(f&1))			/* division by 1 postprocessing if needed */
  {
    if (xs && d==1)
    { xv1 += d1 * xv; xs = 0; res = 1UL; }
    else if (!xs && d1==1)
    { xv += d * xv1; xs = 1; res = 1UL; }
  }

  if (xs)
  {
    *s = -1; *v = xv1; *v1 = xv;
    return (res ? res : (d==1 ? 1UL : d1));
  }
  else
  {
    *s = 1; *v = xv; *v1 = xv1;
    return (res ? res : (d1==1 ? 1UL : d));
  }
}


ulong
xxgcduu(ulong d, ulong d1, int f,
	ulong* u, ulong* u1, ulong* v, ulong* v1, long *s)
{
  ulong xu,xu1, xv,xv1, xs, q,res;
  LOCAL_HIREMAINDER;

  xs = res = 0;
  xu = xv1 = 1UL;
  xu1 = xv = 0UL;
  while (d1 > 1UL)
  {
    d -= d1;			/* no need to use subll */
    if (d >= d1)
    {
      hiremainder = 0; q = 1 + divll(d,d1); d = hiremainder;
      xv += q * xv1;
      xu += q * xu1;
    }
    else
    { xv += xv1; xu += xu1; }
				/* possible loop exit */
    if (d <= 1UL) { xs=1; break; }
				/* repeat with inverted roles */
    d1 -= d;
    if (d1 >= d)
    {
      hiremainder = 0; q = 1 + divll(d1,d); d1 = hiremainder;
      xv1 += q * xv;
      xu1 += q * xu;
    }
    else
    { xv1 += xv; xu1 += xu; }
  } /* while */

  if (!(f&1))			/* division by 1 postprocessing if needed */
  {
    if (xs && d==1)
    {
      xv1 += d1 * xv;
      xu1 += d1 * xu;
      xs = 0; res = 1UL;
    }
    else if (!xs && d1==1)
    {
      xv += d * xv1;
      xu += d * xu1;
      xs = 1; res = 1UL;
    }
  }

  if (xs)
  {
    *s = -1; *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
    return (res ? res : (d==1 ? 1UL : d1));
  }
  else
  {
    *s = 1; *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
    return (res ? res : (d1==1 ? 1UL : d));
  }
}

ulong
rgcduu(ulong d, ulong d1, ulong vmax,
       ulong* u, ulong* u1, ulong* v, ulong* v1, long *s)
{
  ulong xu,xu1, xv,xv1, xs, q, res=0;
  int f = 0;
  LOCAL_HIREMAINDER;

  if (vmax == 0) vmax = MAXULONG;
  xs = res = 0;
  xu = xv1 = 1UL;
  xu1 = xv = 0UL;
  while (d1 > 1UL)
  {
    d -= d1;			/* no need to use subll */
    if (d >= d1)
    {
      hiremainder = 0; q = 1 + divll(d,d1); d = hiremainder;
      xv += q * xv1;
      xu += q * xu1;
    }
    else
    { xv += xv1; xu += xu1; }
				/* possible loop exit */
    if (xv > vmax) { f=xs=1; break; }
    if (d <= 1UL) { xs=1; break; }
				/* repeat with inverted roles */
    d1 -= d;
    if (d1 >= d)
    {
      hiremainder = 0; q = 1 + divll(d1,d); d1 = hiremainder;
      xv1 += q * xv;
      xu1 += q * xu;
    }
    else
    { xv1 += xv; xu1 += xu; }
				/* possible loop exit */
    if (xv1 > vmax) { f=1; break; }
  } /* while */

  if (!(f&1))			/* division by 1 postprocessing if needed */
  {
    if (xs && d==1)
    {
      xv1 += d1 * xv;
      xu1 += d1 * xu;
      xs = 0; res = 1UL;
    }
    else if (!xs && d1==1)
    {
      xv += d * xv1;
      xu += d * xu1;
      xs = 1; res = 1UL;
    }
  }

  if (xs)
  {
    *s = -1; *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
    return (res ? res : (d==1 ? 1UL : d1));
  }
  else
  {
    *s = 1; *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
    return (res ? res : (d1==1 ? 1UL : d));
  }
}

/*==================================
 * cbezout(a,b,uu,vv)
 *==================================
 * Same as bezout() but for C longs.
 *    Return g = gcd(a,b) >= 0, and assign longs u,v through pointers uu,vv
 *    such that g = u*a + v*b.
 * Special cases:
 *    a == b == 0 ==> pick u=1, v=0 (and return 1, surprisingly)
 *    a != 0 == b ==> keep v=0
 *    a == 0 != b ==> keep u=0
 *    |a| == |b| != 0 ==> keep u=0, set v=+-1
 * Assignments through uu,vv happen unconditionally;  non-NULL pointers
 * _must_ be used.
 */
long
cbezout(long a,long b,long *uu,long *vv)
{
  long s,*t;
  ulong d = labs(a), d1 = labs(b);
  ulong r,u,u1,v,v1;

#ifdef DEBUG_CBEZOUT
  fprintferr("> cbezout(%ld,%ld,%p,%p)\n", a, b, (void *)uu, (void *)vv);
#endif
  if (!b)
  {
    *vv=0L;
    if (!a)
    {
      *uu=1L;
#ifdef DEBUG_CBEZOUT
      fprintferr("< %ld (%ld, %ld)\n", 1L, *uu, *vv);
#endif
      return 0L;
    }
    *uu = a < 0 ? -1L : 1L;
#ifdef DEBUG_CBEZOUT
    fprintferr("< %ld (%ld, %ld)\n", (long)d, *uu, *vv);
#endif
    return (long)d;
  }
  else if (!a || (d == d1))
  {
    *uu = 0L; *vv = b < 0 ? -1L : 1L;
#ifdef DEBUG_CBEZOUT
    fprintferr("< %ld (%ld, %ld)\n", (long)d1, *uu, *vv);
#endif
    return (long)d1;
  }
  else if (d == 1)		/* frequently used by nfinit */
  {
    *uu = a; *vv = 0L;
#ifdef DEBUG_CBEZOUT
    fprintferr("< %ld (%ld, %ld)\n", 1L, *uu, *vv);
#endif
    return 1L;
  }
  else if (d < d1)
  {
/* bug in gcc-2.95.3:
 * s = a; a = b; b = s; produces wrong result a = b. This is OK:  */
    { long _x = a; a = b; b = _x; }	/* in order to keep the right signs */
    r = d; d = d1; d1 = r;
    t = uu; uu = vv; vv = t;
#ifdef DEBUG_CBEZOUT
    fprintferr("  swapping\n");
#endif
  }
  /* d > d1 > 0 */
  r = xxgcduu(d, d1, 0, &u, &u1, &v, &v1, &s);
  if (s < 0)
  {
    *uu = a < 0 ? (long)u : -(long)u;
    *vv = b < 0 ? -(long)v : (long)v;
  }
  else
  {
    *uu = a < 0 ? -(long)u : (long)u;
    *vv = b < 0 ? (long)v : -(long)v;
  }
#ifdef DEBUG_CBEZOUT
  fprintferr("< %ld (%ld, %ld)\n", (long)r, *uu, *vv);
#endif
  return (long)r;
}

/*==================================
 * lgcdii(d,d1,u,u1,v,v1,vmax)
 *==================================*/
/* Lehmer's partial extended gcd algorithm, acting on two t_INT GENs.
 *
 * Tries to determine, using the leading 2*BITS_IN_LONG significant bits of d
 * and a quantity of bits from d1 obtained by a shift of the same displacement,
 * as many partial quotients of d/d1 as possible, and assigns to [u,u1;v,v1]
 * the product of all the [0, 1; 1, q_j] thus obtained, where the leftmost
 * factor arises from the quotient of the first division step.
 *
 * For use in rational reconstruction, input param vmax can be given a
 * nonzero value.  In this case, we will return early as soon as v1 > vmax
 * (i.e. it is guaranteed that v <= vmax). --2001Jan15
 *
 * MUST be called with  d > d1 > 0, and with  d  occupying more than one
 * significant word  (if it doesn't, the caller has no business with us;
 * he/she/it should use xgcduu() instead).  Returns the number of reduction/
 * swap steps carried out, possibly zero, or under certain conditions minus
 * that number.  When the return value is nonzero, the caller should use the
 * returned recurrence matrix to update its own copies of d,d1.  When the
 * return value is non-positive, and the latest remainder after updating
 * turns out to be nonzero, the caller should at once attempt a full division,
 * rather than first trying lgcdii() again -- this typically happens when we
 * are about to encounter a quotient larger than half a word.  (This is not
 * detected infallibly -- after a positive return value, it is perfectly
 * possible that the next stage will end up needing a full division.  After
 * a negative return value, however, this is certain, and should be acted
 * upon.)
 *
 * (The sign information, for which xgcduu() has its return argument s, is now
 * implicit in the LSB of our return value, and the caller may take advantage
 * of the fact that a return value of +-1 implies u==0,u1==v==1  [only v1 pro-
 * vides interesting information in this case].  One might also use the fact
 * that if the return value is +-2, then u==1, but this is rather marginal.)
 *
 * If it was not possible to determine even the first quotient, either because
 * we're too close to an integer quotient or because the quotient would be
 * larger than one word  (if the `leading digit' of d1 after shifting is all
 * zeros),  we return 0 and do not bother to assign anything to the last four
 * args.
 *
 * The division chain might (sometimes) even run to completion.  It will be
 * up to the caller to detect this case.
 *
 * This routine does _not_ change d or d1;  this will also be up to the caller.
 *
 * Note that this routine does not know and does not need to know about the
 * PARI stack.
 */
/*#define DEBUG_LEHMER 1 */
int
lgcdii(ulong* d, ulong* d1,
       ulong* u, ulong* u1, ulong* v, ulong* v1,
       ulong vmax)
{
  /* Strategy:  (1) Extract/shift most significant bits.  We assume that d
   * has at least two significant words, but we can cope with a one-word d1.
   * Let dd,dd1 be the most significant dividend word and matching part of the
   * divisor.
   * (2) Check for overflow on the first division.  For our purposes, this
   * happens when the upper half of dd1 is zero.  (Actually this is detected
   * during extraction.)
   * (3) Get a fix on the first quotient.  We compute q = floor(dd/dd1), which
   * is an upper bound for floor(d/d1), and which gives the true value of the
   * latter if (and-almost-only-if) the remainder dd' = dd-q*dd1 is >= q.
   * (If it isn't, we give up.  This is annoying because the subsequent full
   * division will repeat some work already done, but it happens very infre-
   * quently.  Doing the extra-bit-fetch in this case would be awkward.)
   * (4) Finish initializations.
   *
   * The remainder of the action is comparatively boring... The main loop has
   * been unrolled once  (so we don't swap things and we can apply Jebelean's
   * termination conditions which alternatingly take two different forms during
   * successive iterations).  When we first run out of sufficient bits to form
   * a quotient, and have an extra word of each operand, we pull out two whole
   * word's worth of dividend bits, and divisor bits of matching significance;
   * to these we apply our partial matrix (disregarding overflow because the
   * result mod 2^(2*BITS_IN_LONG) will in fact give the correct values), and
   * re-extract one word's worth of the current dividend and a matching amount
   * of divisor bits.  The affair will normally terminate with matrix entries
   * just short of a whole word.  (We terminate the inner loop before these can
   * possibly overflow.)
   */
  ulong dd,dd1,ddlo,dd1lo, sh,shc;        /* `digits', shift count */
  ulong xu,xu1, xv,xv1, q,res;	/* recurrences, partial quotient, count */
  ulong tmp0,tmp1,tmp2,tmpd,tmpu,tmpv; /* temps */
  ulong dm1,dm2,d1m1,d1m2;
  long ld, ld1, lz;		/* t_INT effective lengths */
  int skip = 0;			/* a boolean flag */
  LOCAL_OVERFLOW;
  LOCAL_HIREMAINDER;

#ifdef DEBUG_LEHMER
  voir(d, -1); voir(d1, -1);
#endif
  /* following is just for convenience: vmax==0 means no bound */
  if (vmax == 0) vmax = MAXULONG;
  ld = lgefint(d); ld1 = lgefint(d1); lz = ld - ld1; /* >= 0 */
  if (lz > 1) return 0;		/* rare, quick and desperate exit */

  d = int_MSW(d); d1 = int_MSW(d1);		/* point at the leading `digits' */
  dm1 = *int_precW(d); d1m1 = *int_precW(d1);	
  dm2 = *int_precW(int_precW(d)); d1m2 = *int_precW(int_precW(d1));
  dd1lo = 0;		        /* unless we find something better */
  sh = bfffo(*d);		/* obtain dividend left shift count */

  if (sh)
  {				/* do the shifting */
    shc = BITS_IN_LONG - sh;
    if (lz)
    {				/* dividend longer than divisor */
      dd1 = (*d1 >> shc);
      if (!(HIGHMASK & dd1)) return 0;  /* overflow detected */
      if (ld1 > 3)
        dd1lo = (*d1 << sh) + (d1m1 >> shc);
      else
        dd1lo = (*d1 << sh);
    }
    else
    {				/* dividend and divisor have the same length */
      dd1 = (*d1 << sh);
      if (!(HIGHMASK & dd1)) return 0;
      if (ld1 > 3)
      {
        dd1 += (d1m1 >> shc);
        if (ld1 > 4)
          dd1lo = (d1m1 << sh) + (d1m2 >> shc);
        else
          dd1lo = (d1m1 << sh);
      }
    }
    /* following lines assume d to have 2 or more significant words */
    dd = (*d << sh) + (dm1 >> shc);
    if (ld > 4)
      ddlo = (dm1 << sh) + (dm2 >> shc);
    else
      ddlo = (dm1 << sh);
  }
  else
  {				/* no shift needed */
    if (lz) return 0;		/* div'd longer than div'r: o'flow automatic */
    dd1 = *d1;
    if (!(HIGHMASK & dd1)) return 0;
    if(ld1 > 3) dd1lo = d1m1;
    /* assume again that d has another significant word */
    dd = *d; ddlo = dm1;
  }
#ifdef DEBUG_LEHMER
  fprintferr("  %lx:%lx, %lx:%lx\n", dd, ddlo, dd1, dd1lo);
#endif

  /* First subtraction/division stage.  (If a subtraction initially suffices,
   * we don't divide at all.)  If a Jebelean condition is violated, and we
   * can't fix it even by looking at the low-order bits in ddlo,dd1lo, we
   * give up and ask for a full division.  Otherwise we commit the result,
   * possibly deciding to re-shift immediately afterwards.
   */
  dd -= dd1;
  if (dd < dd1)
  {				/* first quotient known to be == 1 */
    xv1 = 1UL;
    if (!dd)			/* !(Jebelean condition), extraspecial case */
    {				/* note this can actually happen...  Now
    				 * q==1 is known, but we underflow already.
				 * OTOH we've just shortened d by a whole word.
				 * Thus we feel pleased with ourselves and
				 * return.  (The re-shift code below would
				 * do so anyway.) */
      *u = 0; *v = *u1 = *v1 = 1UL;
      return -1;		/* Next step will be a full division. */
    } /* if !(Jebelean) then */
  }
  else
  {				/* division indicated */
    hiremainder = 0;
    xv1 = 1 + divll(dd, dd1);	/* xv1: alternative spelling of `q', here ;) */
    dd = hiremainder;
    if (dd < xv1)		/* !(Jebelean cond'), non-extra special case */
    {				/* Attempt to complete the division using the
				 * less significant bits, before skipping right
				 * past the 1st loop to the reshift stage. */
      ddlo = subll(ddlo, mulll(xv1, dd1lo));
      dd = subllx(dd, hiremainder);

      /* If we now have an overflow, q was _certainly_ too large.  Thanks to
       * our decision not to get here unless the original dd1 had bits set in
       * the upper half of the word, however, we now do know that the correct
       * quotient is in fact q-1.  Adjust our data accordingly. */
      if (overflow)
      {
	xv1--;
	ddlo = addll(ddlo,dd1lo);
	dd = addllx(dd,dd1);	/* overflows again which cancels the borrow */
	/* ...and fall through to skip=1 below */
      }
      else
      /* Test Jebelean condition anew, at this point using _all_ the extracted
       * bits we have.  This is clutching at straws; we have a more or less
       * even chance of succeeding this time.  Note that if we fail, we really
       * do not know whether the correct quotient would have been q or some
       * smaller value. */
	if (!dd && ddlo < xv1) return 0;

      /* Otherwise, we now know that q is correct, but we cannot go into the
       * 1st loop.  Raise a flag so we'll remember to skip past the loop.
       * Get here also after the q-1 adjustment case. */
      skip = 1;
    } /* if !(Jebelean) then */
  }
  res = 1;
#ifdef DEBUG_LEHMER
  fprintferr("  q = %ld, %lx, %lx\n", xv1, dd1, dd);
#endif
  if (xv1 > vmax)
  {				/* gone past the bound already */
    *u = 0UL; *u1 = 1UL; *v = 1UL; *v1 = xv1;
    return res;
  }
  xu = 0UL; xv = xu1 = 1UL;

  /* Some invariants from here across the first loop:
   *
   * At this point, and again after we are finished with the first loop and
   * subsequent conditional, a division and the associated update of the
   * recurrence matrix have just been carried out completely.  The matrix
   * xu,xu1;xv,xv1 has been initialized (or updated, possibly with permuted
   * columns), and the current remainder == next divisor (dd at the moment)
   * is nonzero (it might be zero here, but then skip will have been set).
   *
   * After the first loop, or when skip is set already, it will also be the
   * case that there aren't sufficiently many bits to continue without re-
   * shifting.  If the divisor after reshifting is zero, or indeed if it
   * doesn't have more than half a word's worth of bits, we will have to
   * return at that point.  Otherwise, we proceed into the second loop.
   *
   * Furthermore, when we reach the re-shift stage, dd:ddlo and dd1:dd1lo will
   * already reflect the result of applying the current matrix to the old
   * ddorig:ddlo and dd1orig:dd1lo.  (For the first iteration above, this
   * was easy to achieve, and we didn't even need to peek into the (now
   * no longer existent!) saved words.  After the loop, we'll stop for a
   * moment to merge in the ddlo,dd1lo contributions.)
   *
   * Note that after the first division, even an a priori quotient of 1 cannot
   * be trusted until we've checked Jebelean's condition -- it cannot be too
   * large, of course, but it might be too small.
   */

  if (!skip)
  {
    for(;;)
    {
      /* First half of loop divides dd into dd1, and leaves the recurrence
       * matrix xu,...,xv1 groomed the wrong way round (xu,xv will be the newer
       * entries) when successful. */
      tmpd = dd1 - dd;
      if (tmpd < dd)
      {				/* quotient suspected to be 1 */
#ifdef DEBUG_LEHMER
	q = 1;
#endif
	tmpu = xu + xu1;	/* cannot overflow -- everything bounded by
				 * the original dd during first loop */
	tmpv = xv + xv1;
      }
      else
      {				/* division indicated */
	hiremainder = 0;
	q = 1 + divll(tmpd, dd);
	tmpd = hiremainder;
	tmpu = xu + q*xu1;	/* can't overflow, but may need to be undone */
	tmpv = xv + q*xv1;
      }

      tmp0 = addll(tmpv, xv1);
      if ((tmpd < tmpu) || overflow ||
	  (dd - tmpd < tmp0))	/* !(Jebelean cond.) */
	break;			/* skip ahead to reshift stage */
      else
      {				/* commit dd1, xu, xv */
	res++;
	dd1 = tmpd; xu = tmpu; xv = tmpv;
#ifdef DEBUG_LEHMER
	fprintferr("  q = %ld, %lx, %lx [%lu,%lu;%lu,%lu]\n",
		   q, dd, dd1, xu1, xu, xv1, xv);
#endif
	if (xv > vmax)
	{			/* time to return */
	  *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
	  return res;
	}
      }

      /* Second half of loop divides dd1 into dd, and the matrix returns to its
       * normal arrangement. */
      tmpd = dd - dd1;
      if (tmpd < dd1)
      {				/* quotient suspected to be 1 */
#ifdef DEBUG_LEHMER
	q = 1;
#endif
	tmpu = xu1 + xu;	/* cannot overflow */
	tmpv = xv1 + xv;
      }
      else
      {				/* division indicated */
	hiremainder = 0;
	q = 1 + divll(tmpd, dd1);
	tmpd = hiremainder;
	tmpu = xu1 + q*xu;
	tmpv = xv1 + q*xv;
      }

      tmp0 = addll(tmpu, xu);
      if ((tmpd < tmpv) || overflow ||
	  (dd1 - tmpd < tmp0))	/* !(Jebelean cond.) */
	break;			/* skip ahead to reshift stage */
      else
      {				/* commit dd, xu1, xv1 */
	res++;
	dd = tmpd; xu1 = tmpu; xv1 = tmpv;
#ifdef DEBUG_LEHMER
	fprintferr("  q = %ld, %lx, %lx [%lu,%lu;%lu,%lu]\n",
		q, dd1, dd, xu, xu1, xv, xv1);
#endif
	if (xv1 > vmax)
	{			/* time to return */
	  *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
	  return res;
	}
      }

    } /* end of first loop */

    /* Intermezzo: update dd:ddlo, dd1:dd1lo.  (But not if skip is set.) */

    if (res&1)
    {
      /* after failed division in 1st half of loop:
       * [dd1:dd1lo,dd:ddlo] = [ddorig:ddlo,dd1orig:dd1lo]
       *                       * [ -xu, xu1 ; xv, -xv1 ]
       * (Actually, we only multiply [ddlo,dd1lo] onto the matrix and
       * add the high-order remainders + overflows onto [dd1,dd].)
       */
      tmp1 = mulll(ddlo, xu); tmp0 = hiremainder;
      tmp1 = subll(mulll(dd1lo,xv), tmp1);
      dd1 += subllx(hiremainder, tmp0);
      tmp2 = mulll(ddlo, xu1); tmp0 = hiremainder;
      ddlo = subll(tmp2, mulll(dd1lo,xv1));
      dd += subllx(tmp0, hiremainder);
      dd1lo = tmp1;
    }
    else
    {
      /* after failed division in 2nd half of loop:
       * [dd:ddlo,dd1:dd1lo] = [ddorig:ddlo,dd1orig:dd1lo]
       *                       * [ xu1, -xu ; -xv1, xv ]
       * (Actually, we only multiply [ddlo,dd1lo] onto the matrix and
       * add the high-order remainders + overflows onto [dd,dd1].)
       */
      tmp1 = mulll(ddlo, xu1); tmp0 = hiremainder;
      tmp1 = subll(tmp1, mulll(dd1lo,xv1));
      dd += subllx(tmp0, hiremainder);
      tmp2 = mulll(ddlo, xu); tmp0 = hiremainder;
      dd1lo = subll(mulll(dd1lo,xv), tmp2);
      dd1 += subllx(hiremainder, tmp0);
      ddlo = tmp1;
    }
#ifdef DEBUG_LEHMER
    fprintferr("  %lx:%lx, %lx:%lx\n", dd, ddlo, dd1, dd1lo);
#endif
  } /* end of skip-pable section:  get here also, with res==1, when there
     * was a problem immediately after the very first division. */

  /* Re-shift.  Note:  the shift count _can_ be zero, viz. under the following
   * precise conditions:  The original dd1 had its topmost bit set, so the 1st
   * q was 1, and after subtraction, dd had its topmost bit unset.  If now
   * dd==0, we'd have taken the return exit already, so we couldn't have got
   * here.  If not, then it must have been the second division which has gone
   * amiss  (because dd1 was very close to an exact multiple of the remainder
   * dd value, so this will be very rare).  At this point, we'd have a fairly
   * slim chance of fixing things by re-examining dd1:dd1lo vs. dd:ddlo, but
   * this is not guaranteed to work.  Instead of trying, we return at once.
   * The caller will see to it that the initial subtraction is re-done using
   * _all_ the bits of both operands, which already helps, and the next round
   * will either be a full division  (if dd occupied a halfword or less),  or
   * another llgcdii() first step.  In the latter case, since we try a little
   * harder during our first step, we may actually be able to fix the problem,
   * and get here again with improved low-order bits and with another step
   * under our belt.  Otherwise we'll have given up above and forced a full-
   * blown division.
   *
   * If res is even, the shift count _cannot_ be zero.  (The first step forces
   * a zero into the remainder's MSB, and all subsequent remainders will have
   * inherited it.)
   *
   * The re-shift stage exits if the next divisor has at most half a word's
   * worth of bits.
   *
   * For didactic reasons, the second loop will be arranged in the same way
   * as the first -- beginning with the division of dd into dd1, as if res
   * was odd.  To cater for this, if res is actually even, we swap things
   * around during reshifting.  (During the second loop, the parity of res
   * does not matter;  we know in which half of the loop we are when we decide
   * to return.)
   */
#ifdef DEBUG_LEHMER
  fprintferr("(sh)");
#endif

  if (res&1)
  {				/* after odd number of division(s) */
    if (dd1 && (sh = bfffo(dd1)))
    {
      shc = BITS_IN_LONG - sh;
      dd = (ddlo >> shc) + (dd << sh);
      if (!(HIGHMASK & dd))
      {
	*u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
	return -res;		/* full division asked for */
      }
      dd1 = (dd1lo >> shc) + (dd1 << sh);
    }
    else
    {				/* time to return: <= 1 word left, or sh==0 */
      *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
      return res;
    }
  }
  else
  {				/* after even number of divisions */
    if (dd)
    {
      sh = bfffo(dd);		/* Known to be positive. */
      shc = BITS_IN_LONG - sh;
				/* dd:ddlo will become the new dd1, and v.v. */
      tmpd = (ddlo >> shc) + (dd << sh);
      dd = (dd1lo >> shc) + (dd1 << sh);
      dd1 = tmpd;
      /* This has completed the swap;  now dd is again the current divisor.
       * The following test originally inspected dd1 -- a most subtle and
       * most annoying bug. The Management. */
      if (HIGHMASK & dd)
      {
	/* recurrence matrix is the wrong way round;  swap it. */
	tmp0 = xu; xu = xu1; xu1 = tmp0;
	tmp0 = xv; xv = xv1; xv1 = tmp0;
      }
      else
      {
	/* recurrence matrix is the wrong way round;  fix this. */
	*u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
	return -res;		/* full division asked for */
      }
    }
    else
    {				/* time to return: <= 1 word left */
      *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
      return res;
    }
  } /* end reshift */

#ifdef DEBUG_LEHMER
  fprintferr("  %lx:%lx, %lx:%lx\n", dd, ddlo, dd1, dd1lo);
#endif

  /* The Second Loop.  Rip-off of the first, but we now check for overflow
   * in the recurrences.  Returns instead of breaking when we cannot fix the
   * quotient any longer.
   */

  for(;;)
  {
    /* First half of loop divides dd into dd1, and leaves the recurrence
     * matrix xu,...,xv1 groomed the wrong way round (xu,xv will be the newer
     * entries) when successful. */
    tmpd = dd1 - dd;
    if (tmpd < dd)
    {				/* quotient suspected to be 1 */
#ifdef DEBUG_LEHMER
      q = 1;
#endif
      tmpu = xu + xu1;
      tmpv = addll(xv, xv1);	/* xv,xv1 will overflow first */
      tmp1 = overflow;
    }
    else
    {				/* division indicated */
      hiremainder = 0;
      q = 1 + divll(tmpd, dd);
      tmpd = hiremainder;
      tmpu = xu + q*xu1;
      tmpv = addll(xv, mulll(q,xv1));
      tmp1 = overflow | hiremainder;
    }

    tmp0 = addll(tmpv, xv1);
    if ((tmpd < tmpu) || overflow || tmp1 ||
	(dd - tmpd < tmp0))	/* !(Jebelean cond.) */
    {
      /* The recurrence matrix has not yet been warped... */
      *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
      break;
    }
    /* commit dd1, xu, xv */
    res++;
    dd1 = tmpd; xu = tmpu; xv = tmpv;
#ifdef DEBUG_LEHMER
    fprintferr("  q = %ld, %lx, %lx\n", q, dd, dd1);
#endif
    if (xv > vmax)
    {				/* time to return */
      *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
      return res;
    }

    /* Second half of loop divides dd1 into dd, and the matrix returns to its
     * normal arrangement. */
    tmpd = dd - dd1;
    if (tmpd < dd1)
    {				/* quotient suspected to be 1 */
#ifdef DEBUG_LEHMER
      q = 1;
#endif
      tmpu = xu1 + xu;
      tmpv = addll(xv1, xv);
      tmp1 = overflow;
    }
    else
    {				/* division indicated */
      hiremainder = 0;
      q = 1 + divll(tmpd, dd1);
      tmpd = hiremainder;
      tmpu = xu1 + q*xu;
      tmpv = addll(xv1, mulll(q, xv));
      tmp1 = overflow | hiremainder;
    }

    tmp0 = addll(tmpu, xu);
    if ((tmpd < tmpv) || overflow || tmp1 ||
	(dd1 - tmpd < tmp0))	/* !(Jebelean cond.) */
    {
      /* The recurrence matrix has not yet been unwarped, so it is
       * the wrong way round;  fix this. */
      *u = xu1; *u1 = xu; *v = xv1; *v1 = xv;
      break;
    }

    res++; /* commit dd, xu1, xv1 */
    dd = tmpd; xu1 = tmpu; xv1 = tmpv;
#ifdef DEBUG_LEHMER
    fprintferr("  q = %ld, %lx, %lx\n", q, dd1, dd);
#endif
    if (xv1 > vmax)
    {				/* time to return */
      *u = xu; *u1 = xu1; *v = xv; *v1 = xv1;
      return res;
    }
  } /* end of second loop */

  return res;
}

/* 1 / Mod(x,p). Assume x < p */
ulong
Fl_inv(ulong x, ulong p)
{
  long s;
  ulong xv, xv1, g = xgcduu(p, x, 1, &xv, &xv1, &s);
  if (g != 1UL) pari_err(invmoder, "%Z", mkintmod(utoi(x), utoi(p)));
  xv = xv1 % p; if (s < 0) xv = p - xv;
  return xv;
}

#if 0
/* assume m > 0 */
long
Fl_inv_signed(long a, long m)
{
  if (a >= 0)
  {
    if (a > m) a %= m;
  }
  else
  {
    if (-a > m) a %= m;
    a += m;
  }
  return (long)Fl_inv((ulong)a, (ulong)m);
}
#endif
#line 2 "../src/kernel/none/ratlift.c"
/* $Id: ratlift.c 7534 2005-12-12 08:58:13Z kb $

Copyright (C) 2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/*==========================================================
 * ratlift(GEN x, GEN m, GEN *a, GEN *b, GEN amax, GEN bmax)
 *==========================================================
 * Reconstruct rational number from its residue x mod m
 *    Given t_INT x, m, amax>=0, bmax>0 such that
 * 	0 <= x < m;  2*amax*bmax < m
 *    attempts to find t_INT a, b such that
 * 	(1) a = b*x (mod m)
 * 	(2) |a| <= amax, 0 < b <= bmax
 * 	(3) gcd(m, b) = gcd(a, b)
 *    If unsuccessful, it will return 0 and leave a,b unchanged  (and
 *    caller may deduce no such a,b exist).  If successful, sets a,b
 *    and returns 1.  If there exist a,b satisfying (1), (2), and
 * 	(3') gcd(m, b) = 1
 *    then they are uniquely determined subject to (1),(2) and
 * 	(3'') gcd(a, b) = 1,
 *    and will be returned by the routine.  (The caller may wish to
 *    check gcd(a,b)==1, either directly or based on known prime
 *    divisors of m, depending on the application.)
 * Reference:
 @article {MR97c:11116,
     AUTHOR = {Collins, George E. and Encarnaci{\'o}n, Mark J.},
      TITLE = {Efficient rational number reconstruction},
    JOURNAL = {J. Symbolic Comput.},
     VOLUME = {20},
       YEAR = {1995},
     NUMBER = {3},
      PAGES = {287--297},
 }
 * Preprint available from:
 * ftp://ftp.risc.uni-linz.ac.at/pub/techreports/1994/94-64.ps.gz
 */

/* #define DEBUG_RATLIFT */

int
ratlift(GEN x, GEN m, GEN *a, GEN *b, GEN amax, GEN bmax)
{
  GEN d,d1,v,v1,q,r;
  pari_sp av = avma, av1, lim;
  long lb,lr,lbb,lbr,s,s0;
  ulong vmax;
  ulong xu,xu1,xv,xv1;		/* Lehmer stage recurrence matrix */
  int lhmres;			/* Lehmer stage return value */

  if ((typ(x) | typ(m) | typ(amax) | typ(bmax)) != t_INT) pari_err(arither1);
  if (signe(bmax) <= 0)
    pari_err(talker, "ratlift: bmax must be > 0, found\n\tbmax=%Z\n", bmax);
  if (signe(amax) < 0)
    pari_err(talker, "ratilft: amax must be >= 0, found\n\tamax=%Z\n", amax);
  /* check 2*amax*bmax < m */
  if (cmpii(shifti(mulii(amax, bmax), 1), m) >= 0)
    pari_err(talker, "ratlift: must have 2*amax*bmax < m, found\n\tamax=%Z\n\tbmax=%Z\n\tm=%Z\n", amax,bmax,m);
  /* we _could_ silently replace x with modii(x,m) instead of the following,
   * but let's leave this up to the caller
   */
  avma = av; s = signe(x);
  if (s < 0 || cmpii(x,m) >= 0)
    pari_err(talker, "ratlift: must have 0 <= x < m, found\n\tx=%Z\n\tm=%Z\n", x,m);

  /* special cases x=0 and/or amax=0 */
  if (s == 0)
  {
    if (a != NULL) *a = gen_0;
    if (b != NULL) *b = gen_1;
    return 1;
  }
  else if (signe(amax)==0)
    return 0;
  /* assert: m > x > 0, amax > 0 */

  /* check here whether a=x, b=1 is a solution */
  if (cmpii(x,amax) <= 0)
  {
    if (a != NULL) *a = icopy(x);
    if (b != NULL) *b = gen_1;
    return 1;
  }

  /* There is no special case for single-word numbers since this is
   * mainly meant to be used with large moduli.
   */
  (void)new_chunk(lgefint(bmax) + lgefint(amax)); /* room for a,b */
  d = m; d1 = x;
  v = gen_0; v1 = gen_1;
  /* assert d1 > amax, v1 <= bmax here */
  lb = lgefint(bmax);
  lbb = bfffo(*int_MSW(bmax));
  s = 1;
  av1 = avma; lim = stack_lim(av, 1);

  /* general case: Euclidean division chain starting with m div x, and
   * with bounds on the sequence of convergents' denoms v_j.
   * Just to be different from what invmod and bezout are doing, we work
   * here with the all-nonnegative matrices [u,u1;v,v1]=prod_j([0,1;1,q_j]).
   * Loop invariants:
   * (a) (sign)*[-v,v1]*x = [d,d1] (mod m)  (componentwise)
   * (sign initially +1, changes with each Euclidean step)
   * so [a,b] will be obtained in the form [-+d,v] or [+-d1,v1];
   * this congruence is a consequence of
   * (b) [x,m]~ = [u,u1;v,v1]*[d1,d]~,
   * where u,u1 is the usual numerator sequence starting with 1,0
   * instead of 0,1  (just multiply the eqn on the left by the inverse
   * matrix, which is det*[v1,-u1;-v,u], where "det" is the same as the
   * "(sign)" in (a)).  From m = v*d1 + v1*d and
   * (c) d > d1 >= 0, 0 <= v < v1,
   * we have d >= m/(2*v1), so while v1 remains smaller than m/(2*amax),
   * the pair [-(sign)*d,v] satisfies (1) but violates (2) (d > amax).
   * Conversely, v1 > bmax indicates that no further solutions will be
   * forthcoming;  [-(sign)*d,v] will be the last, and first, candidate.
   * Thus there's at most one point in the chain division where a solution
   * can live:  v < bmax, v1 >= m/(2*amax) > bmax,  and this is acceptable
   * iff in fact d <= amax  (e.g. m=221, x=34 or 35, amax=bmax=10 fail on
   * this count while x=32,33,36,37 succeed).  However, a division may leave
   * a zero residue before we ever reach this point  (consider m=210, x=35,
   * amax=bmax=10),  and our caller may find that gcd(d,v) > 1  (numerous
   * examples -- keep m=210 and consider any of x=29,31,32,33,34,36,37,38,
   * 39,40,41).
   * Furthermore, at the start of the loop body we have in fact
   * (c') 0 <= v < v1 <= bmax, d > d1 > amax >= 0,
   * (and are never done already).
   *
   * Main loop is similar to those of invmod() and bezout(), except for
   * having to determine appropriate vmax bounds, and checking termination
   * conditions.  The signe(d1) condition is only for paranoia
   */
  while (lgefint(d) > 3 && signe(d1))
  {
    /* determine vmax for lgcdii so as to ensure v won't overshoot.
     * If v+v1 > bmax, the next step would take v1 beyond the limit, so
     * since [+-d1,v1] is not a solution, we give up.  Otherwise if v+v1
     * is way shorter than bmax, use vmax=MAXULUNG.  Otherwise, set vmax
     * to a crude lower approximation of bmax/(v+v1), or to 1, which will
     * allow the inner loop to do one step
     */
    r = addii(v,v1);
    lr = lb - lgefint(r);
    lbr = bfffo(*int_MSW(r));
    if (cmpii(r,bmax) > 0)	/* done, not found */
    {
      avma = av;
      return 0;
    }
    else if (lr > 1)		/* still more than a word's worth to go */
    {
      vmax = MAXULONG;
    }
    else			/* take difference of bit lengths */
    {
      lr = (lr << TWOPOTBITS_IN_LONG) - lbb + lbr;
      if ((ulong)lr > BITS_IN_LONG)
	vmax = MAXULONG;
      else if (lr == 0)
	vmax = 1UL;
      else
	vmax = 1UL << (lr-1);
      /* the latter is pessimistic but faster than a division */
    }
    /* do a Lehmer-Jebelean round */
    lhmres = lgcdii((ulong *)d, (ulong *)d1, &xu, &xu1, &xv, &xv1, vmax);
    if (lhmres != 0)		/* check progress */
    {				/* apply matrix */
      if ((lhmres == 1) || (lhmres == -1))
      {
	s = -s;
	if (xv1 == 1)
	{
	  /* re-use v+v1 computed above */
	  v=v1; v1=r;
	  r = subii(d,d1); d=d1; d1=r;
	}
	else
	{
	  r = subii(d, mului(xv1,d1)); d=d1; d1=r;
	  r = addii(v, mului(xv1,v1)); v=v1; v1=r;
	}
      }
      else
      {
	r  = subii(muliu(d,xu),  muliu(d1,xv));
	d1 = subii(muliu(d,xu1), muliu(d1,xv1)); d = r;
	r  = addii(muliu(v,xu),  muliu(v1,xv));
	v1 = addii(muliu(v,xu1), muliu(v1,xv1)); v = r;
        if (lhmres&1)
	{
          setsigne(d,-signe(d));
	  s = -s;
        }
        else if (signe(d1))
	{
          setsigne(d1,-signe(d1));
        }
      }
      /* check whether we're done.  Assert v <= bmax here.  Examine v1:
       * if v1 > bmax, check d and return 0 or 1 depending on the outcome;
       * if v1 <= bmax, check d1 and return 1 if d1 <= amax, otherwise
       * proceed.
       */
      if (cmpii(v1,bmax) > 0) /* certainly done */
      {
	avma = av;
	if (cmpii(d,amax) <= 0) /* done, found */
	{
	  if (a != NULL)
	  {
	    *a = icopy(d);
	    setsigne(*a,-s);	/* sign opposite to s */
	  }
	  if (b != NULL) *b = icopy(v);
	  return 1;
	}
	else			/* done, not found */
	  return 0;
      }
      else if (cmpii(d1,amax) <= 0) /* also done, found */
      {
	avma = av;
	if (a != NULL)
	{
	  if (signe(d1))
	  {
	    *a = icopy(d1);
	    setsigne(*a,s);	/* same sign as s */
	  }
	  else
	    *a = gen_0;
	}
	if (b != NULL) *b = icopy(v1);
	return 1;
      }
    } /* lhmres != 0 */

    if (lhmres <= 0 && signe(d1))
    {
      q = dvmdii(d,d1,&r);
#ifdef DEBUG_LEHMER
      fprintferr("Full division:\n");
      printf("  q = "); output(q); sleep (1);
#endif
      d=d1; d1=r;
      r = addii(v,mulii(q,v1));
      v=v1; v1=r;
      s = -s;
      /* check whether we are done now.  Since we weren't before the div, it
       * suffices to examine v1 and d1 -- the new d (former d1) cannot cut it
       */
      if (cmpii(v1,bmax) > 0) /* done, not found */
      {
	avma = av;
	return 0;
      }
      else if (cmpii(d1,amax) <= 0) /* done, found */
      {
	avma = av;
	if (a != NULL)
	{
	  if (signe(d1))
	  {
	    *a = icopy(d1);
	    setsigne(*a,s);	/* same sign as s */
	  }
	  else
	    *a = gen_0;
	}
	if (b != NULL) *b = icopy(v1);
	return 1;
      }
    }

    if (low_stack(lim, stack_lim(av,1)))
    {
      GEN *gptr[4]; gptr[0]=&d; gptr[1]=&d1; gptr[2]=&v; gptr[3]=&v1;
      if(DEBUGMEM>1) pari_warn(warnmem,"ratlift");
      gerepilemany(av1,gptr,4);
    }
  } /* end while */

  /* Postprocessing - final sprint.  Since we usually underestimate vmax,
   * this function needs a loop here instead of a simple conditional.
   * Note we can only get here when amax fits into one word  (which will
   * typically not be the case!).  The condition is bogus -- d1 is never
   * zero at the start of the loop.  There will be at most a few iterations,
   * so we don't bother collecting garbage
   */
  while (signe(d1))
  {
    /* Assertions: lgefint(d)==lgefint(d1)==3.
     * Moreover, we aren't done already, or we would have returned by now.
     * Recompute vmax...
     */
#ifdef DEBUG_RATLIFT
    fprintferr("rl-fs: d,d1=%Z,%Z\n", d, d1);
    fprintferr("rl-fs: v,v1=%Z,%Z\n", v, v1);
#endif
    r = addii(v,v1);
    lr = lb - lgefint(r);
    lbr = bfffo(*int_MSW(r));
    if (cmpii(r,bmax) > 0)	/* done, not found */
    {
      avma = av;
      return 0;
    }
    else if (lr > 1)		/* still more than a word's worth to go */
    {
      vmax = MAXULONG;		/* (cannot in fact happen) */
    }
    else			/* take difference of bit lengths */
    {
      lr = (lr << TWOPOTBITS_IN_LONG) - lbb + lbr;
      if ((ulong)lr > BITS_IN_LONG)
	vmax = MAXULONG;
      else if (lr == 0)
	vmax = 1UL;
      else
	vmax = 1UL << (lr-1);	/* as above */
    }
#ifdef DEBUG_RATLIFT
    fprintferr("rl-fs: vmax=%lu\n", vmax);
#endif
    /* single-word "Lehmer", discarding the gcd or whatever it returns */
    (void)rgcduu((ulong)*int_MSW(d), (ulong)*int_MSW(d1), vmax, &xu, &xu1, &xv, &xv1, &s0);
#ifdef DEBUG_RATLIFT
    fprintferr("rl-fs: [%lu,%lu; %lu,%lu] %s\n",
	       xu, xu1, xv, xv1,
	       s0 < 0 ? "-" : "+");
#endif
    if (xv1 == 1)		/* avoid multiplications */
    {
      /* re-use v+v1 computed above */
      v=v1; v1=r;
      r = subii(d,d1); d=d1; d1=r;
      s = -s;
    }
    else if (xu == 0)		/* and xv==1, xu1==1, xv1 > 1 */
    {
      r = subii(d, mului(xv1,d1)); d=d1; d1=r;
      r = addii(v, mului(xv1,v1)); v=v1; v1=r;
      s = -s;
    }
    else
    {
      r  = subii(muliu(d,xu),  muliu(d1,xv));
      d1 = subii(muliu(d,xu1), muliu(d1,xv1)); d = r;
      r  = addii(muliu(v,xu),  muliu(v1,xv));
      v1 = addii(muliu(v,xu1), muliu(v1,xv1)); v = r;
      if (s0 < 0)
      {
	setsigne(d,-signe(d));
	s = -s;
      }
      else if (signe(d1))		/* sic: might vanish now */
      {
	setsigne(d1,-signe(d1));
      }
    }
    /* check whether we're done, as above.  Assert v <= bmax.  Examine v1:
     * if v1 > bmax, check d and return 0 or 1 depending on the outcome;
     * if v1 <= bmax, check d1 and return 1 if d1 <= amax, otherwise proceed.
     */
    if (cmpii(v1,bmax) > 0) /* certainly done */
    {
      avma = av;
      if (cmpii(d,amax) <= 0) /* done, found */
      {
	if (a != NULL)
	{
	  *a = icopy(d);
	  setsigne(*a,-s);	/* sign opposite to s */
	}
	if (b != NULL) *b = icopy(v);
	return 1;
      }
      else			/* done, not found */
	return 0;
    }
    else if (cmpii(d1,amax) <= 0) /* also done, found */
    {
      avma = av;
      if (a != NULL)
      {
	if (signe(d1))
	{
	  *a = icopy(d1);
	  setsigne(*a,s);	/* same sign as s */
	}
	else
	  *a = gen_0;
      }
      if (b != NULL) *b = icopy(v1);
      return 1;
    }
  } /* while */

  /* get here when we have run into d1 == 0 before returning... in fact,
   * this cannot happen.
   */
  pari_err(talker, "ratlift failed to catch d1 == 0\n");
  /* NOTREACHED */
  return 0;
}

#line 2 "../src/kernel/none/gcd.c"
/* $Id: gcd.c 7600 2006-01-11 10:40:41Z kb $

Copyright (C) 2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* assume y > x > 0. return y mod x */
static ulong
resiu(GEN y, ulong x)
{
  long i, ly = lgefint(y);
  LOCAL_HIREMAINDER;

  hiremainder = 0;
  for (i=2; i<ly; i++) (void)divll(y[i],x);
  return hiremainder;
}

/* Assume x>y>0, both of them odd. return x-y if x=y mod 4, x+y otherwise */
static void
gcd_plus_minus(GEN x, GEN y, GEN res)
{
  pari_sp av = avma;
  long lx = lgefint(x)-1;
  long ly = lgefint(y)-1, lt,m,i;
  GEN t;

  if ((x[lx]^y[ly]) & 3) /* x != y mod 4*/
    t = addiispec(x+2,y+2,lx-1,ly-1);
  else
    t = subiispec(x+2,y+2,lx-1,ly-1);

  lt = lgefint(t)-1; while (!t[lt]) lt--;
  m = vals(t[lt]); lt++;
  if (m == 0) /* 2^32 | t */
  {
    for (i = 2; i < lt; i++) res[i] = t[i];
  }
  else if (t[2] >> m)
  {
    shift_right(res,t, 2,lt, 0,m);
  }
  else
  {
    lt--; t++;
    shift_right(res,t, 2,lt, t[1],m);
  }
  res[1] = evalsigne(1)|evallgefint(lt);
  avma = av;
}

/* uses modified right-shift binary algorithm now --GN 1998Jul23 */
GEN
gcdii(GEN a, GEN b)
{
  long v, w;
  pari_sp av;
  GEN t, p1;

  switch (absi_cmp(a,b))
  {
    case 0: return absi(a);
    case -1: t=b; b=a; a=t;
  }
  if (!signe(b)) return absi(a);
  /* here |a|>|b|>0. Try single precision first */
  if (lgefint(a)==3)
    return gcduu((ulong)a[2], (ulong)b[2]);
  if (lgefint(b)==3)
  {
    ulong u = resiu(a,(ulong)b[2]);
    if (!u) return absi(b);
    return gcduu((ulong)b[2], u);
  }

  /* larger than gcd: "avma=av" gerepile (erasing t) is valid */
  av = avma; (void)new_chunk(lgefint(b)); /* HACK */
  t = remii(a,b);
  if (!signe(t)) { avma=av; return absi(b); }

  a = b; b = t;
  v = vali(a); a = shifti(a,-v); setsigne(a,1);
  w = vali(b); b = shifti(b,-w); setsigne(b,1);
  if (w < v) v = w;
  switch(absi_cmp(a,b))
  {
    case  0: avma=av; a=shifti(a,v); return a;
    case -1: p1=b; b=a; a=p1;
  }
  if (is_pm1(b)) { avma=av; return int2n(v); }

  /* we have three consecutive memory locations: a,b,t.
   * All computations are done in place */

  /* a and b are odd now, and a>b>1 */
  while (lgefint(a) > 3)
  {
    /* if a=b mod 4 set t=a-b, otherwise t=a+b, then strip powers of 2 */
    /* so that t <= (a+b)/4 < a/2 */
    gcd_plus_minus(a,b, t);
    if (is_pm1(t)) { avma=av; return int2n(v); }
    switch(absi_cmp(t,b))
    {
      case -1: p1 = a; a = b; b = t; t = p1; break;
      case  1: p1 = a; a = t; t = p1; break;
      case  0: avma = av; b=shifti(b,v); return b;
    }
  }
  {
    long r[] = {evaltyp(t_INT)|_evallg(3), evalsigne(1)|evallgefint(3), 0};
    r[2] = (long) ugcd((ulong)b[2], (ulong)a[2]);
    avma = av; return shifti(r,v);
  }
}

/*==================================
 * bezout(a,b,pu,pv)
 *==================================
 *    Return g = gcd(a,b) >= 0, and assign GENs u,v through pointers pu,pv
 *    such that g = u*a + v*b.
 * Special cases:
 *    a == b == 0 ==> pick u=1, v=0
 *    a != 0 == b ==> keep v=0
 *    a == 0 != b ==> keep u=0
 *    |a| == |b| != 0 ==> keep u=0, set v=+-1
 * Assignments through pu,pv will be suppressed when the corresponding
 * pointer is NULL  (but the computations will happen nonetheless).
 */

GEN
bezout(GEN a, GEN b, GEN *pu, GEN *pv)
{
  GEN t,u,u1,v,v1,d,d1,q,r;
  GEN *pt;
  pari_sp av, av1, lim;
  long s, sa, sb;
  ulong g;
  ulong xu,xu1,xv,xv1;		/* Lehmer stage recurrence matrix */
  int lhmres;			/* Lehmer stage return value */

  if (typ(a) != t_INT || typ(b) != t_INT) pari_err(arither1);
  s = absi_cmp(a,b);
  if (s < 0)
  {
    t=b; b=a; a=t;
    pt=pu; pu=pv; pv=pt;
  }
  /* now |a| >= |b| */

  sa = signe(a); sb = signe(b);
  if (!sb)
  {
    if (pv) *pv = gen_0;
    switch(sa)
    {
    case  0: if (pu) *pu = gen_0; return gen_0;
    case  1: if (pu) *pu = gen_1; return icopy(a);
    case -1: if (pu) *pu = gen_m1; return(negi(a));
    }
  }
  if (s == 0)			/* |a| == |b| != 0 */
  {
    if (pu) *pu = gen_0;
    if (sb > 0)
    { if (pv) *pv = gen_1; return icopy(b); }
    else
    { if (pv) *pv = gen_m1; return(negi(b)); }
  }
  /* now |a| > |b| > 0 */

  if (lgefint(a) == 3)		/* single-word affair */
  {
    g = xxgcduu((ulong)a[2], (ulong)b[2], 0, &xu, &xu1, &xv, &xv1, &s);
    sa = s > 0 ? sa : -sa;
    sb = s > 0 ? -sb : sb;
    if (pu)
    {
      if (xu == 0) *pu = gen_0; /* can happen when b divides a */
      else if (xu == 1) *pu = sa < 0 ? gen_m1 : gen_1;
      else if (xu == 2) *pu = sa < 0 ? negi(gen_2) : gen_2;
      else
      {
	*pu = cgeti(3);
	(*pu)[1] = evalsigne(sa)|evallgefint(3);
	(*pu)[2] = xu;
      }
    }
    if (pv)
    {
      if (xv == 1) *pv = sb < 0 ? gen_m1 : gen_1;
      else if (xv == 2) *pv = sb < 0 ? negi(gen_2) : gen_2;
      else
      {
	*pv = cgeti(3);
	(*pv)[1] = evalsigne(sb)|evallgefint(3);
	(*pv)[2] = xv;
      }
    }
    if      (g == 1) return gen_1;
    else if (g == 2) return gen_2;
    else
    {
      r = cgeti(3);
      r[1] = evalsigne(1)|evallgefint(3);
      r[2] = g;
      return r;
    }
  }

  /* general case */
  av = avma;
  (void)new_chunk(lgefint(b) + (lgefint(a)<<1)); /* room for u,v,gcd */
  /* if a is significantly larger than b, calling lgcdii() is not the best
   * way to start -- reduce a mod b first
   */
  if (lgefint(a) > lgefint(b))
  {
    d = absi(b), q = dvmdii(absi(a), d, &d1);
    if (!signe(d1))		/* a == qb */
    {
      avma = av;
      if (pu) *pu = gen_0;
      if (pv) *pv = sb < 0 ? gen_m1 : gen_1;
      return (icopy(d));
    }
    else
    {
      u = gen_0;
      u1 = v = gen_1;
      v1 = negi(q);
    }
    /* if this results in lgefint(d) == 3, will fall past main loop */
  }
  else
  {
    d = absi(a); d1 = absi(b);
    u = v1 = gen_1; u1 = v = gen_0;
  }
  av1 = avma; lim = stack_lim(av, 1);

  /* main loop is almost identical to that of invmod() */
  while (lgefint(d) > 3 && signe(d1))
  {
    lhmres = lgcdii((ulong *)d, (ulong *)d1, &xu, &xu1, &xv, &xv1, MAXULONG);
    if (lhmres != 0)		/* check progress */
    {				/* apply matrix */
      if ((lhmres == 1) || (lhmres == -1))
      {
	if (xv1 == 1)
	{
	  r = subii(d,d1); d=d1; d1=r;
	  a = subii(u,u1); u=u1; u1=a;
	  a = subii(v,v1); v=v1; v1=a;
	}
	else
	{
	  r = subii(d, mului(xv1,d1)); d=d1; d1=r;
	  a = subii(u, mului(xv1,u1)); u=u1; u1=a;
	  a = subii(v, mului(xv1,v1)); v=v1; v1=a;
	}
      }
      else
      {
	r  = subii(muliu(d,xu),  muliu(d1,xv));
	d1 = subii(muliu(d,xu1), muliu(d1,xv1)); d = r;
	a  = subii(muliu(u,xu),  muliu(u1,xv));
	u1 = subii(muliu(u,xu1), muliu(u1,xv1)); u = a;
	a  = subii(muliu(v,xu),  muliu(v1,xv));
	v1 = subii(muliu(v,xu1), muliu(v1,xv1)); v = a;
        if (lhmres&1)
	{
          setsigne(d,-signe(d));
          setsigne(u,-signe(u));
          setsigne(v,-signe(v));
        }
        else
	{
          if (signe(d1)) { setsigne(d1,-signe(d1)); }
          setsigne(u1,-signe(u1));
          setsigne(v1,-signe(v1));
        }
      }
    }
    if (lhmres <= 0 && signe(d1))
    {
      q = dvmdii(d,d1,&r);
#ifdef DEBUG_LEHMER
      fprintferr("Full division:\n");
      printf("  q = "); output(q); sleep (1);
#endif
      a = subii(u,mulii(q,u1));
      u=u1; u1=a;
      a = subii(v,mulii(q,v1));
      v=v1; v1=a;
      d=d1; d1=r;
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"bezout");
      gerepileall(av1,6, &d,&d1,&u,&u1,&v,&v1);
    }
  } /* end while */

  /* Postprocessing - final sprint */
  if (signe(d1))
  {
    /* Assertions: lgefint(d)==lgefint(d1)==3, and
     * gcd(d,d1) is nonzero and fits into one word
     */
    g = xxgcduu((ulong)(d[2]), (ulong)(d1[2]), 0, &xu, &xu1, &xv, &xv1, &s);
    u = subii(muliu(u,xu), muliu(u1, xv));
    v = subii(muliu(v,xu), muliu(v1, xv));
    if (s < 0) { sa = -sa; sb = -sb; }
    avma = av;
    if (pu) *pu = sa < 0 ? negi(u) : icopy(u);
    if (pv) *pv = sb < 0 ? negi(v) : icopy(v);
    if (g == 1) return gen_1;
    else if (g == 2) return gen_2;
    else
    {
      r = cgeti(3);
      r[1] = evalsigne(1)|evallgefint(3);
      r[2] = g;
      return r;
    }
  }
  /* get here when the final sprint was skipped (d1 was zero already).
   * Now the matrix is final, and d contains the gcd.
   */
  avma = av;
  if (pu) *pu = sa < 0 ? negi(u) : icopy(u);
  if (pv) *pv = sb < 0 ? negi(v) : icopy(v);
  return icopy(d);
}

#line 2 "../src/kernel/none/invmod.c"
/* $Id: invmod.c 7534 2005-12-12 08:58:13Z kb $

Copyright (C) 2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/*==================================
 * invmod(a,b,res)
 *==================================
 *    If a is invertible, return 1, and set res  = a^{ -1 }
 *    Otherwise, return 0, and set res = gcd(a,b)
 *
 * This is sufficiently different from bezout() to be implemented separately
 * instead of having a bunch of extra conditionals in a single function body
 * to meet both purposes.
 */

#ifdef INVMOD_PARI
INLINE int
invmod_pari(GEN a, GEN b, GEN *res)
#else
int
invmod(GEN a, GEN b, GEN *res)
#endif
{
  GEN v,v1,d,d1,q,r;
  pari_sp av, av1, lim;
  long s;
  ulong g;
  ulong xu,xu1,xv,xv1;		/* Lehmer stage recurrence matrix */
  int lhmres;			/* Lehmer stage return value */

  if (typ(a) != t_INT || typ(b) != t_INT) pari_err(arither1);
  if (!signe(b)) { *res=absi(a); return 0; }
  av = avma;
  if (lgefint(b) == 3) /* single-word affair */
  {
    ulong d1 = umodiu(a, (ulong)(b[2]));
    if (d1 == 0)
    {
      if (b[2] == 1L)
        { *res = gen_0; return 1; }
      else
        { *res = absi(b); return 0; }
    }
    g = xgcduu((ulong)(b[2]), d1, 1, &xv, &xv1, &s);
#ifdef DEBUG_LEHMER
    fprintferr(" <- %lu,%lu\n", (ulong)(b[2]), (ulong)(d1[2]));
    fprintferr(" -> %lu,%ld,%lu; %lx\n", g,s,xv1,avma);
#endif
    avma = av;
    if (g != 1UL) { *res = utoipos(g); return 0; }
    xv = xv1 % (ulong)(b[2]); if (s < 0) xv = ((ulong)(b[2])) - xv;
    *res = utoipos(xv); return 1;
  }

  (void)new_chunk(lgefint(b));
  d = absi(b); d1 = modii(a,d);

  v=gen_0; v1=gen_1;	/* general case */
#ifdef DEBUG_LEHMER
  fprintferr("INVERT: -------------------------\n");
  output(d1);
#endif
  av1 = avma; lim = stack_lim(av,1);

  while (lgefint(d) > 3 && signe(d1))
  {
#ifdef DEBUG_LEHMER
    fprintferr("Calling Lehmer:\n");
#endif
    lhmres = lgcdii((ulong*)d, (ulong*)d1, &xu, &xu1, &xv, &xv1, MAXULONG);
    if (lhmres != 0)		/* check progress */
    {				/* apply matrix */
#ifdef DEBUG_LEHMER
      fprintferr("Lehmer returned %d [%lu,%lu;%lu,%lu].\n",
	      lhmres, xu, xu1, xv, xv1);
#endif
      if ((lhmres == 1) || (lhmres == -1))
      {
	if (xv1 == 1)
	{
	  r = subii(d,d1); d=d1; d1=r;
	  a = subii(v,v1); v=v1; v1=a;
	}
	else
	{
	  r = subii(d, mului(xv1,d1)); d=d1; d1=r;
	  a = subii(v, mului(xv1,v1)); v=v1; v1=a;
	}
      }
      else
      {
	r  = subii(muliu(d,xu),  muliu(d1,xv));
	a  = subii(muliu(v,xu),  muliu(v1,xv));
	d1 = subii(muliu(d,xu1), muliu(d1,xv1)); d = r;
	v1 = subii(muliu(v,xu1), muliu(v1,xv1)); v = a;
        if (lhmres&1)
	{
          setsigne(d,-signe(d));
          setsigne(v,-signe(v));
        }
        else
	{
          if (signe(d1)) { setsigne(d1,-signe(d1)); }
          setsigne(v1,-signe(v1));
        }
      }
    }
#ifdef DEBUG_LEHMER
    else
      fprintferr("Lehmer returned 0.\n");
    output(d); output(d1); output(v); output(v1);
    sleep(1);
#endif

    if (lhmres <= 0 && signe(d1))
    {
      q = dvmdii(d,d1,&r);
#ifdef DEBUG_LEHMER
      fprintferr("Full division:\n");
      printf("  q = "); output(q); sleep (1);
#endif
      a = subii(v,mulii(q,v1));
      v=v1; v1=a;
      d=d1; d1=r;
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      GEN *gptr[4]; gptr[0]=&d; gptr[1]=&d1; gptr[2]=&v; gptr[3]=&v1;
      if(DEBUGMEM>1) pari_warn(warnmem,"invmod");
      gerepilemany(av1,gptr,4);
    }
  } /* end while */

  /* Postprocessing - final sprint */
  if (signe(d1))
  {
    /* Assertions: lgefint(d)==lgefint(d1)==3, and
     * gcd(d,d1) is nonzero and fits into one word
     */
    g = xxgcduu((ulong)d[2], (ulong)d1[2], 1, &xu, &xu1, &xv, &xv1, &s);
#ifdef DEBUG_LEHMER
    output(d);output(d1);output(v);output(v1);
    fprintferr(" <- %lu,%lu\n", (ulong)d[2], (ulong)d1[2]);
    fprintferr(" -> %lu,%ld,%lu; %lx\n", g,s,xv1,avma);
#endif
    if (g != 1UL) { avma = av; *res = utoipos(g); return 0; }
    /* (From the xgcduu() blurb:)
     * For finishing the multiword modinv, we now have to multiply the
     * returned matrix  (with properly adjusted signs)  onto the values
     * v' and v1' previously obtained from the multiword division steps.
     * Actually, it is sufficient to take the scalar product of [v',v1']
     * with [u1,-v1], and change the sign if s==1.
     */
    v = subii(muliu(v,xu1),muliu(v1,xv1));
    if (s > 0) setsigne(v,-signe(v));
    avma = av; *res = modii(v,b);
#ifdef DEBUG_LEHMER
    output(*res); fprintfderr("============================Done.\n");
    sleep(1);
#endif
    return 1;
  }
  /* get here when the final sprint was skipped (d1 was zero already) */
  avma = av;
  if (!equalii(d,gen_1)) { *res = icopy(d); return 0; }
  *res = modii(v,b);
#ifdef DEBUG_LEHMER
  output(*res); fprintferr("============================Done.\n");
  sleep(1);
#endif
  return 1;
}

#line 2 "../src/kernel/none/mp_indep.c"
/* $Id: mp_indep.c 10294 2008-06-10 15:58:51Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* Find c such that 1=c*b mod 2^BITS_IN_LONG, assuming b odd (unchecked) */
ulong
invrev(ulong b)
{
  static int tab[] = { 0, 0, 0, 8, 0, 8, 0, 0 };
  ulong x = b + tab[b & 7]; /* b^(-1) mod 2^4 */

  /* Newton applied to 1/x - b = 0 */
#ifdef LONG_IS_64BIT
  x = x*(2-b*x); /* one more pass necessary */
#endif
  x = x*(2-b*x);
  x = x*(2-b*x); return x*(2-b*x);
}

void
affrr(GEN x, GEN y)
{
  long lx,ly,i;

  y[1] = x[1]; if (!signe(x)) return;

  lx=lg(x); ly=lg(y);
  if (lx <= ly)
  {
    for (i=2; i<lx; i++) y[i]=x[i];
    for (   ; i<ly; i++) y[i]=0;
    return;
  }
  for (i=2; i<ly; i++) y[i]=x[i];
  /* lx > ly: round properly */
  if (x[ly] & HIGHBIT) roundr_up_ip(y, ly);
}

GEN
ishiftr(GEN x, long s)
{
  long ex,lx,n;
  if (!signe(x)) return gen_0;
  ex = expo(x) + s; if (ex < 0) return gen_0;
  lx = lg(x);
  n=ex - bit_accuracy(lx) + 1;
  return ishiftr_lg(x, lx, n);
}

GEN
icopy_spec(GEN x, long nx)
{
  GEN z;
  long i;
  if (!nx) return gen_0;
  z = cgeti(nx+2); z[1] = evalsigne(1)|evallgefint(nx+2);
  for (i=0; i<nx; i++) z[i+2] = x[i];
  return z;
}

GEN
mului(ulong x, GEN y)
{
  long s = signe(y);
  GEN z;

  if (!s || !x) return gen_0;
  z = muluispec(x, y+2, lgefint(y)-2);
  setsigne(z,s); return z;
}

GEN
mulsi(long x, GEN y)
{
  long s = signe(y);
  GEN z;

  if (!s || !x) return gen_0;
  if (x<0) { s = -s; x = -x; }
  z = muluispec((ulong)x, y+2, lgefint(y)-2);
  setsigne(z,s); return z;
}

/* assume x > 1, y != 0. Return u * y with sign s */
static GEN
mulur_2(ulong x, GEN y, long s)
{
  long m, sh, i, lx = lg(y), e = expo(y);
  GEN z = cgetr(lx);
  ulong garde;
  LOCAL_HIREMAINDER;

  y--; garde = mulll(x,y[lx]);
  for (i=lx-1; i>=3; i--) z[i]=addmul(x,y[i]);
  z[2]=hiremainder; /* != 0 since y normalized and |x| > 1 */

  sh = bfffo(hiremainder); m = BITS_IN_LONG-sh;
  if (sh) shift_left2(z,z, 2,lx-1, garde,sh,m);
  z[1] = evalsigne(s) | evalexpo(m+e); return z;
}

GEN
mulsr(long x, GEN y)
{
  long s, e;

  if (!x) return gen_0;
  s = signe(y);
  if (!s)
  {
    if (x < 0) x = -x;
    e = expo(y) + (BITS_IN_LONG-1)-bfffo(x);
    return real_0_bit(e);
  }
  if (x==1)  return rcopy(y);
  if (x==-1) return negr(y);
  if (x < 0)
    return mulur_2((ulong)-x, y, -s);
  else
    return mulur_2((ulong)x, y, s);
}

GEN
mulur(ulong x, GEN y)
{
  long s, e;

  if (!x) return gen_0;
  s = signe(y);
  if (!s)
  {
    e = expo(y) + (BITS_IN_LONG-1)-bfffo(x);
    return real_0_bit(e);
  }
  if (x==1) return rcopy(y);
  return mulur_2(x, y, s);
}

#ifdef KARAMULR_VARIANT
static GEN addshiftw(GEN x, GEN y, long d);
static GEN
karamulrr1(GEN y, GEN x, long ly, long lz)
{
  long i, l, lz2 = (lz+2)>>1, lz3 = lz-lz2;
  GEN lo1, lo2, hi;

  hi = muliispec_mirror(x,y, lz2,lz2);
  i = lz2; while (i<lz && !x[i]) i++;
  lo1 = muliispec_mirror(y,x+i, lz2,lz-i);
  i = lz2; while (i<ly && !y[i]) i++;
  lo2 = muliispec_mirror(x,y+i, lz2,ly-i);
  if (signe(lo1))
  {
    if (ly!=lz) { lo2 = addshiftw(lo1,lo2,1); lz3++; }
    else lo2 = addii(lo1,lo2);
  }
  l = lgefint(lo2)-(lz3+2);
  if (l > 0) hi = addiispec(hi+2,lo2+2, lgefint(hi)-2,l);
  return hi;
}
#endif

/* set z <-- x*y, floating point multiplication.
 * lz = lg(z) = lg(x) <= ly <= lg(y), sz = signe(z). flag = lg(x) < lg(y) */
INLINE void
mulrrz_i(GEN z, GEN x, GEN y, long lz, long flag, long sz)
{
  long ez = expo(x) + expo(y);
  long i, j, lzz, p1;
  ulong garde;
  GEN y1;
  LOCAL_HIREMAINDER;
  LOCAL_OVERFLOW;

  if (lz > KARATSUBA_MULR_LIMIT) 
  {
    pari_sp av = avma;
#ifdef KARAMULR_VARIANT
    GEN hi = karamulrr1(y+2, x+2, lz+flag-2, lz-2); 
#else
    GEN hi = muliispec_mirror(y+2, x+2, lz+flag-2, lz-2);
#endif
    garde = hi[lz];
    if (hi[2] < 0)
    {
      ez++;
      for (i=2; i<lz ; i++) z[i] = hi[i];
    }
    else
    {
      shift_left(z,hi,2,lz-1, garde, 1);
      garde <<= 1;
    }
    if (garde & HIGHBIT)
    { /* round to nearest */
      i = lz; do z[--i]++; while (z[i]==0 && i > 1);
      if (i == 1) { z[2] = HIGHBIT; ez++; }
    }
    z[1] = evalsigne(sz)|evalexpo(ez);
#if 0
{
GEN U;
KARATSUBA_MULR_LIMIT = 100000;
U = mulrr(x, y);
KARATSUBA_MULR_LIMIT = 4;
if (!gequal(U, z)) pari_err(talker,"toto");
}
#endif
    avma = av; return;
  }
  if (lz == 3)
  {
    if (flag)
    {
      (void)mulll(x[2],y[3]);
      garde = addmul(x[2],y[2]);
    }
    else
      garde = mulll(x[2],y[2]);
    if (hiremainder & HIGHBIT)
    {
      ez++;
      /* hiremainder < (2^BIL-1)^2 / 2^BIL, hence hiremainder+1 != 0 */
      if (garde & HIGHBIT) hiremainder++; /* round properly */
    }
    else
    {
      hiremainder = (hiremainder<<1) | (garde>>(BITS_IN_LONG-1));
      if (garde & (HIGHBIT-1))
      {
        hiremainder++; /* round properly */
        if (!hiremainder) { hiremainder = HIGHBIT; ez++; }
      }
    }
    z[1] = evalsigne(sz) | evalexpo(ez);
    z[2] = hiremainder; return;
  }

  if (flag) { (void)mulll(x[2],y[lz]); garde = hiremainder; } else garde = 0;
  lzz=lz-1; p1=x[lzz];
  if (p1)
  {
    (void)mulll(p1,y[3]);
    garde = addll(addmul(p1,y[2]), garde);
    z[lzz] = overflow+hiremainder;
  }
  else z[lzz]=0;
  for (j=lz-2, y1=y-j; j>=3; j--)
  {
    p1 = x[j]; y1++;
    if (p1)
    {
      (void)mulll(p1,y1[lz+1]);
      garde = addll(addmul(p1,y1[lz]), garde);
      for (i=lzz; i>j; i--)
      {
        hiremainder += overflow;
	z[i] = addll(addmul(p1,y1[i]), z[i]);
      }
      z[j] = hiremainder+overflow;
    }
    else z[j]=0;
  }
  p1 = x[2]; y1++;
  garde = addll(mulll(p1,y1[lz]), garde);
  for (i=lzz; i>2; i--)
  {
    hiremainder += overflow;
    z[i] = addll(addmul(p1,y1[i]), z[i]);
  }
  z[2] = hiremainder+overflow;

  if (z[2] < 0)
    ez++;
  else
  {
    shift_left(z,z,2,lzz, garde, 1);
    garde <<= 1;
  }
  if (garde & HIGHBIT)
  { /* round to nearest */
    i = lz; do z[--i]++; while (z[i]==0 && i > 2);
    if (z[i] == 0) { z[2] = (long)HIGHBIT; ez++; }
  }
  z[1] = evalsigne(sz) | evalexpo(ez);
}

GEN
mulrr(GEN x, GEN y)
{
  long flag, ly, lz, sx = signe(x), sy = signe(y);
  GEN z;

  if (!sx || !sy) return real_0_bit(expo(x) + expo(y));
  if (sy < 0) sx = -sx;
  lz = lg(x);
  ly = lg(y);
  if (lz > ly) { lz = ly; swap(x, y); flag = 1; } else flag = (lz != ly);
  z = cgetr(lz);
  mulrrz_i(z, x,y, lz,flag, sx);
  return z;
}

GEN
mulir(GEN x, GEN y)
{
  long sx = signe(x), sy, lz;
  GEN z;

  if (!sx) return gen_0;
  if (!is_bigint(x)) return mulsr(itos(x), y);
  sy = signe(y);
  if (!sy) return real_0_bit(expi(x) + expo(y));
  if (sy < 0) sx = -sx;
  lz = lg(y); z = cgetr(lz);
  mulrrz_i(z, itor(x,lz),y, lz,0, sx);
  avma = (pari_sp)z; return z;
}

/* written by Bruno Haible following an idea of Robert Harley */
long
vals(ulong z)
{
  static char tab[64]={-1,0,1,12,2,6,-1,13,3,-1,7,-1,-1,-1,-1,14,10,4,-1,-1,8,-1,-1,25,-1,-1,-1,-1,-1,21,27,15,31,11,5,-1,-1,-1,-1,-1,9,-1,-1,24,-1,-1,20,26,30,-1,-1,-1,-1,23,-1,19,29,-1,22,18,28,17,16,-1};
#ifdef LONG_IS_64BIT
  long s;
#endif

  if (!z) return -1;
#ifdef LONG_IS_64BIT
  if (! (z&0xffffffff)) { s = 32; z >>=32; } else s = 0;
#endif
  z |= ~z + 1;
  z += z << 4;
  z += z << 6;
  z ^= z << 16; /* or  z -= z<<16 */
#ifdef LONG_IS_64BIT
  return s + tab[(z&0xffffffff)>>26];
#else
  return tab[z>>26];
#endif
}

GEN
divsi(long x, GEN y)
{
  long p1, s = signe(y);
  LOCAL_HIREMAINDER;

  if (!s) pari_err(gdiver);
  if (!x || lgefint(y)>3 || ((long)y[2])<0) return gen_0;
  hiremainder=0; p1=divll(labs(x),y[2]);
  if (x<0) { hiremainder = -((long)hiremainder); p1 = -p1; }
  if (s<0) p1 = -p1;
  return stoi(p1);
}

GEN
divir(GEN x, GEN y)
{
  GEN z;
  long ly;
  pari_sp av;

  if (!signe(y)) pari_err(gdiver);
  if (!signe(x)) return gen_0;
  ly = lg(y); z = cgetr(ly); av = avma; 
  affrr(divrr(itor(x, ly+1), y), z);
  avma = av; return z;
}

void
mpdivz(GEN x, GEN y, GEN z)
{
  pari_sp av = avma;
  long tx = typ(x), ty = typ(y);
  GEN q = tx==t_INT && ty==t_INT? divii(x,y): mpdiv(x,y);

  if (typ(z) == t_REAL) affrr(q, z);
  else
  {
    if (typ(q) == t_REAL) pari_err(gdiver);
    affii(q, z); 
  }
  avma = av;
}

GEN
divsr(long x, GEN y)
{
  pari_sp av;
  long ly;
  GEN z;

  if (!signe(y)) pari_err(gdiver);
  if (!x) return gen_0;
  ly = lg(y); z = cgetr(ly); av = avma;
  affrr(divrr(stor(x,ly+1), y), z);
  avma = av; return z;
}

GEN
modii(GEN x, GEN y)
{
  switch(signe(x))
  {
    case 0: return gen_0;
    case 1: return remii(x,y);
    default:
    {
      pari_sp av = avma;
      (void)new_chunk(lgefint(y));
      x = remii(x,y); avma=av;
      if (x==gen_0) return x;
      return subiispec(y+2,x+2,lgefint(y)-2,lgefint(x)-2);
    }
  }
}

void
modiiz(GEN x, GEN y, GEN z)
{
  const pari_sp av = avma;
  affii(modii(x,y),z); avma=av;
}

GEN
divrs(GEN x, long y)
{
  long i,lx,garde,sh,s=signe(x);
  GEN z;
  LOCAL_HIREMAINDER;

  if (!y) pari_err(gdiver);
  if (!s) return real_0_bit(expo(x) - (BITS_IN_LONG-1)+bfffo(y));
  if (y<0) { s = -s; y = -y; }
  if (y==1) { z=rcopy(x); setsigne(z,s); return z; }

  z=cgetr(lx=lg(x)); hiremainder=0;
  for (i=2; i<lx; i++) z[i] = divll(x[i],y);

  /* we may have hiremainder != 0 ==> garde */
  garde=divll(0,y); sh=bfffo(z[2]);
  if (sh) shift_left(z,z, 2,lx-1, garde,sh);
  z[1] = evalsigne(s) | evalexpo(expo(x)-sh);
  if ((garde << sh) & HIGHBIT) roundr_up_ip(z, lx);
  return z;
}

GEN
truedvmdii(GEN x, GEN y, GEN *z)
{
  pari_sp av;
  GEN r, q, *gptr[2];
  if (!is_bigint(y)) return truedvmdis(x, itos(y), z);

  av = avma;
  q = dvmdii(x,y,&r); /* assume that r is last on stack */

  if (signe(r)>=0)
  {
    if (z == ONLY_REM) return gerepileuptoint(av,r);
    if (z) *z = r; else cgiv(r);
    return q;
  }

  if (z == ONLY_REM)
  { /* r += sign(y) * y, that is |y| */
    r = subiispec(y+2,r+2, lgefint(y)-2,lgefint(r)-2);
    return gerepileuptoint(av, r);
  }
  q = addis(q, -signe(y));
  if (!z) return gerepileuptoint(av, q);

  *z = subiispec(y+2,r+2, lgefint(y)-2,lgefint(r)-2);
  gptr[0]=&q; gptr[1]=z; gerepilemanysp(av,(pari_sp)r,gptr,2);
  return q;
}
GEN
truedvmdis(GEN x, long y, GEN *z)
{
  pari_sp av = avma;
  long r;
  GEN q = divis_rem(x,y,&r);

  if (r >= 0)
  {
    if (z == ONLY_REM) { avma = av; return utoi(r); }
    if (z) *z = utoi(r);
    return q;
  }
  if (z == ONLY_REM) { avma = av; return utoi(r + labs(y)); }
  q = gerepileuptoint(av, addis(q, (y < 0)? 1: -1));
  if (z) *z = utoi(r + labs(y));
  return q;
}

/* 2^n = shifti(gen_1, n) */
GEN
int2n(long n) {
  long i, m, d, l;
  GEN z;
  if (n < 0) return gen_0;
  if (n == 0) return gen_1;

  d = n>>TWOPOTBITS_IN_LONG;
  m = n & (BITS_IN_LONG-1);
  l = d + 3; z = cgeti(l);
  z[1] = evalsigne(1) | evallgefint(l);
  for (i = 2; i < l; i++) z[i] = 0;
  *int_MSW(z) = 1L << m; return z;
}
/* To avoid problems when 2^(BIL-1) < n. Overflow cleanly, where int2n
 * returns gen_0 */
GEN
int2u(ulong n) {
  ulong i, m, d, l;
  GEN z;
  if (n == 0) return gen_1;

  d = n>>TWOPOTBITS_IN_LONG;
  m = n & (BITS_IN_LONG-1);
  l = d + 3; z = cgeti(l);
  z[1] = evalsigne(1) | evallgefint(l);
  for (i = 2; i < l; i++) z[i] = 0;
  *int_MSW(z) = 1L << m; return z;
}

/* actual operations will take place on a+2 and b+2: we strip the codewords */
GEN
mulii(GEN a,GEN b)
{
  long sa,sb;
  GEN z;

  sa=signe(a); if (!sa) return gen_0;
  sb=signe(b); if (!sb) return gen_0;
  if (sb<0) sa = -sa;
  z = muliispec(a+2,b+2, lgefint(a)-2,lgefint(b)-2);
  setsigne(z,sa); return z;
}

GEN
remiimul(GEN x, GEN sy)
{
  GEN r, q, y = (GEN)sy[1], invy;
  long k;
  pari_sp av = avma;

  k = cmpii(x, y);
  if (k <= 0) return k? icopy(x): gen_0;
  invy = (GEN)sy[2];
  q = mulir(x,invy);
  q = truncr(q); /* differs from divii(x, y) at most by 1 */
  r = subii(x, mulii(y,q));
  if (signe(r) < 0)
    r = subiispec(y+2,r+2, lgefint(y)-2, lgefint(r)-2); /* y - |r| */
  else
  {
    /* remii(x,y) + y >= r >= remii(x,y) */
    k = absi_cmp(r, y);
    if (k >= 0)
    {
      if (k == 0) { avma = av; return gen_0; }
      r = subiispec(r+2, y+2, lgefint(r)-2, lgefint(y)-2);
    }
  }
#if 0
  q = subii(r,remii(x,y));
  if (signe(q))
    pari_err(talker,"bug in remiimul: x = %Z\ny = %Z\ndif = %Z", x,y,q);
#endif
  return gerepileuptoint(av, r); /* = remii(x, y) */
}

GEN
sqri(GEN a) { return sqrispec(a+2, lgefint(a)-2); }

/* Old cgiv without reference count (which was not used anyway)
 * Should be a macro.
 */
void
cgiv(GEN x)
{
  if (x == (GEN) avma)
    avma = (pari_sp) (x+lg(x));
}

/* sqrt()'s result may be off by 1 when a is not representable exactly as a
 * double [64bit machine] */
ulong
usqrtsafe(ulong a)
{
  ulong x = (ulong)sqrt((double)a);
#ifdef LONG_IS_64BIT
  ulong y = x+1; if (y <= MAXHALFULONG && y*y <= a) x = y;
#endif
  return x;
}

/********************************************************************/
/**                                                                **/
/**                         RANDOM INTEGERS                        **/
/**                                                                **/
/********************************************************************/
static long pari_randseed = 1;

/* BSD rand gives this: seed = 1103515245*seed + 12345 */
/*Return 31 ``random'' bits.*/
long
pari_rand31(void)
{
  pari_randseed = (1000276549*pari_randseed + 12347) & 0x7fffffff;
  return pari_randseed;
}

long
setrand(long seed) { return (pari_randseed = seed); }

long
getrand(void) { return pari_randseed; }

static ulong
pari_rand(void)
{
#define GLUE2(hi, lo) (((hi) << BITS_IN_HALFULONG) | (lo))
#if !defined(LONG_IS_64BIT)
  return GLUE2((pari_rand31()>>12)&LOWMASK,
               (pari_rand31()>>12)&LOWMASK);
#else
#define GLUE4(hi1,hi2, lo1,lo2) GLUE2(((hi1)<<16)|(hi2), ((lo1)<<16)|(lo2))
#  define LOWMASK2 0xffffUL
  return GLUE4((pari_rand31()>>12)&LOWMASK2,
               (pari_rand31()>>12)&LOWMASK2,
               (pari_rand31()>>12)&LOWMASK2,
               (pari_rand31()>>12)&LOWMASK2);
#endif
}

#if 1
/* assume N > 0 */
GEN
randomi(GEN N)
{
  ulong n;
  long lx, i;
  GEN x, xMSW, xd, Nd;

  lx = lgefint(N); x = cgeti(lx);
  x[1] = evalsigne(1) | evallgefint(lx);
  xMSW = xd = int_MSW(x);
  for (i=2; i<lx; i++) { *xd = pari_rand(); xd = int_precW(xd); }

  Nd = int_MSW(N); n = (ulong)*Nd;
  xd = xMSW;
  if (lx == 3) n--;
  else
    for (i=3; i<lx; i++)
    {
      xd = int_precW(xd);
      Nd = int_precW(Nd);
      if (*xd != *Nd) {
        if ((ulong)*xd > (ulong)*Nd) n--;
        break;
      }
    }
  /* MSW needs to be generated between 0 and n */
  if (n) {
    LOCAL_HIREMAINDER;
    (void)mulll((ulong)*xMSW, n + 1);
    n = hiremainder;
  }
  *xMSW = (long)n;
  if (!n) x = int_normalize(x, 1);
  return x;
}
#else
/* assume N > 0 */
GEN
randomi(GEN N)
{
  long i, lx = lgefint(N), shift = bfffo(*int_MSW(N));
  GEN x = cgeti(lx), xMSW, xd;

  x[1] = evalsigne(1) | evallgefint(lx);
  xMSW = int_MSW(x);
  for (xd = xMSW;;) {
    for (i=2; i<lx; i++) { *xd = pari_rand(); xd = int_precW(xd); }
    *xMSW = ((ulong)*xMSW) >> shift;
    x = int_normalize(x, 0);
    if (absi_cmp(x, N) < 0) return x;
  }
}
#endif

/********************************************************************/
/**                                                                **/
/**              EXPONENT / CONVERSION t_REAL --> double           **/
/**                                                                **/
/********************************************************************/

#ifdef LONG_IS_64BIT
long
dblexpo(double x)
{
  union { double f; ulong i; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */

  if (x==0.) return -exp_mid;
  fi.f = x;
  return ((fi.i & (HIGHBIT-1)) >> mant_len) - exp_mid;
}

ulong
dblmantissa(double x)
{
  union { double f; ulong i; } fi;
  const int expo_len = 11; /* number of bits of exponent */

  if (x==0.) return 0;
  fi.f = x;
  return (fi.i << expo_len) | HIGHBIT;
}

GEN
dbltor(double x)
{
  GEN z;
  long e;
  union { double f; ulong i; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */
  const int expo_len = 11; /* number of bits of exponent */

  if (x==0.) return real_0_bit(-exp_mid);
  fi.f = x; z = cgetr(DEFAULTPREC);
  {
    const ulong a = fi.i;
    ulong A;
    e = ((a & (HIGHBIT-1)) >> mant_len) - exp_mid;
    if (e == exp_mid+1) pari_err(talker, "NaN or Infinity in dbltor");
    A = a << expo_len;
    if (e == -exp_mid)
    { /* unnormalized values */
      int sh = bfffo(A);
      e -= sh-1;
      z[2] = A << sh;
    }
    else
      z[2] = HIGHBIT | A;
    z[1] = evalexpo(e) | evalsigne(x<0? -1: 1);
  }
  return z;
}

double
rtodbl(GEN x)
{
  long ex,s=signe(x);
  ulong a;
  union { double f; ulong i; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */
  const int expo_len = 11; /* number of bits of exponent */

  if (typ(x)==t_INT && !s) return 0.0;
  if (typ(x)!=t_REAL) pari_err(typeer,"rtodbl");
  if (!s || (ex=expo(x)) < - exp_mid) return 0.0;

  /* start by rounding to closest */
  a = (x[2] & (HIGHBIT-1)) + 0x400;
  if (a & HIGHBIT) { ex++; a=0; }
  if (ex >= exp_mid) pari_err(rtodber);
  fi.i = ((ex + exp_mid) << mant_len) | (a >> expo_len);
  if (s<0) fi.i |= HIGHBIT;
  return fi.f;
}

#else /* LONG_IS_64BIT */

#if   PARI_DOUBLE_FORMAT == 1
#  define INDEX0 1
#  define INDEX1 0
#elif PARI_DOUBLE_FORMAT == 0
#  define INDEX0 0
#  define INDEX1 1
#endif

long
dblexpo(double x)
{
  union { double f; ulong i[2]; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */
  const int shift = mant_len-32;

  if (x==0.) return -exp_mid;
  fi.f = x;
  {
    const ulong a = fi.i[INDEX0];
    return ((a & (HIGHBIT-1)) >> shift) - exp_mid;
  }
}

ulong
dblmantissa(double x)
{
  union { double f; ulong i[2]; } fi;
  const int expo_len = 11; /* number of bits of exponent */

  if (x==0.) return 0;
  fi.f = x;
  {
    const ulong a = fi.i[INDEX0];
    const ulong b = fi.i[INDEX1];
    return HIGHBIT | b >> (BITS_IN_LONG-expo_len) | (a << expo_len);
  }
}

GEN
dbltor(double x)
{
  GEN z;
  long e;
  union { double f; ulong i[2]; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */
  const int expo_len = 11; /* number of bits of exponent */
  const int shift = mant_len-32;

  if (x==0.) return real_0_bit(-exp_mid);
  fi.f = x; z = cgetr(DEFAULTPREC);
  {
    const ulong a = fi.i[INDEX0];
    const ulong b = fi.i[INDEX1];
    ulong A, B;
    e = ((a & (HIGHBIT-1)) >> shift) - exp_mid;
    if (e == exp_mid+1) pari_err(talker, "NaN or Infinity in dbltor");
    A = b >> (BITS_IN_LONG-expo_len) | (a << expo_len);
    B = b << expo_len;
    if (e == -exp_mid)
    { /* unnormalized values */
      int sh;
      if (A)
      {
        sh = bfffo(A);
        e -= sh-1;
        z[2] = (A << sh) | (B >> (32-sh));
        z[3] = B << sh;
      }
      else 
      {
        sh = bfffo(B); /* B != 0 */
        e -= sh-1 + 32;
        z[2] = B << sh;
        z[3] = 0;
      }
    }
    else
    {
      z[3] = B;
      z[2] = HIGHBIT | A;
    }
    z[1] = evalexpo(e) | evalsigne(x<0? -1: 1);
  }
  return z;
}

double
rtodbl(GEN x)
{
  long ex,s=signe(x),lx=lg(x);
  ulong a,b,k;
  union { double f; ulong i[2]; } fi;
  const int mant_len = 52;  /* mantissa bits (excl. hidden bit) */
  const int exp_mid = 0x3ff;/* exponent bias */
  const int expo_len = 11; /* number of bits of exponent */
  const int shift = mant_len-32;

  if (typ(x)==t_INT && !s) return 0.0;
  if (typ(x)!=t_REAL) pari_err(typeer,"rtodbl");
  if (!s || (ex=expo(x)) < - exp_mid) return 0.0;

  /* start by rounding to closest */
  a = x[2] & (HIGHBIT-1);
  if (lx > 3)
  {
    b = x[3] + 0x400UL; if (b < 0x400UL) a++;
    if (a & HIGHBIT) { ex++; a=0; }
  }
  else b = 0;
  if (ex >= exp_mid) pari_err(rtodber);
  ex += exp_mid;
  k = (a >> expo_len) | (ex << shift);
  if (s<0) k |= HIGHBIT;
  fi.i[INDEX0] = k;
  fi.i[INDEX1] = (a << (BITS_IN_LONG-expo_len)) | (b >> expo_len);
  return fi.f;
}
#endif /* LONG_IS_64BIT */

#line 2 "../src/kernel/none/add.c"
/* $Id: add.c 8602 2007-04-18 20:06:59Z kb $

Copyright (C) 2002-2003  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* prototype of positive small ints */
static long pos_s[] = {
  evaltyp(t_INT) | _evallg(3), evalsigne(1) | evallgefint(3), 0 };

/* prototype of negative small ints */
static long neg_s[] = {
  evaltyp(t_INT) | _evallg(3), (long)evalsigne(-1) | evallgefint(3), 0 };

GEN
addss(long x, long y)
{
  if (!x) return stoi(y);
  if (x>0) { pos_s[2] = x; return addsi(y,pos_s); }
  neg_s[2] = -x; return addsi(y,neg_s);
}

INLINE GEN
icopy_sign(GEN x, long sx)
{
  GEN y=icopy(x);
  setsigne(y,sx);
  return y;
}

GEN
addsi_sign(long x, GEN y, long sy)
{
  long sx,ly;
  GEN z;

  if (!x) return icopy_sign(y, sy);
  if (!sy) return stoi(x);
  if (x<0) { sx=-1; x=-x; } else sx=1;
  if (sx==sy)
  {
    z = addsispec(x,y+2, lgefint(y)-2);
    setsigne(z,sy); return z;
  }
  ly=lgefint(y);
  if (ly==3)
  {
    const long d = y[2] - x;
    if (!d) return gen_0;
    z=cgeti(3);
    if (y[2] < 0 || d > 0) {
      z[1] = evalsigne(sy) | evallgefint(3);
      z[2] = d;
    }
    else {
      z[1] = evalsigne(-sy) | evallgefint(3);
      z[2] =-d;
    }
    return z;
  }
  z = subisspec(y+2,x, ly-2);
  setsigne(z,sy); return z;
}

GEN
addii_sign(GEN x, long sx, GEN y, long sy)
{
  long lx,ly;
  GEN z;

  if (!sx) return sy? icopy_sign(y, sy): gen_0;
  if (!sy) return icopy_sign(x, sx);
  lx=lgefint(x); ly=lgefint(y);

  if (sx==sy)
    z = addiispec(x+2,y+2,lx-2,ly-2);
  else
  { /* sx != sy */
    long i = lx - ly;
    if (i==0) /* lx == ly */
    {
      i = absi_cmp_lg(x,y,lx);
      if (!i) return gen_0;
    }
    if (i<0) { sx=sy; swapspec(x,y, lx,ly); } /* ensure |x| >= |y| */
    z = subiispec(x+2,y+2,lx-2,ly-2);
  }
  setsigne(z,sx); return z;
}

INLINE GEN
rcopy_sign(GEN x, long sx) { GEN y = rcopy(x); setsigne(y,sx); return y; }

GEN
addir_sign(GEN x, long sx, GEN y, long sy)
{
  long e,l,ly;
  GEN z;

  if (!sx) return rcopy_sign(y, sy);
  e = expo(y) - expi(x);
  if (!sy)
  {
    if (e > 0) return rcopy_sign(y, sy);
    z = itor(x, 3 + ((-e)>>TWOPOTBITS_IN_LONG));
    setsigne(z, sx); return z;
  }

  ly = lg(y);
  if (e > 0)
  {
    l = ly - (e>>TWOPOTBITS_IN_LONG);
    if (l < 3) return rcopy_sign(y, sy);
  }
  else l = ly + ((-e)>>TWOPOTBITS_IN_LONG)+1;
  z = (GEN)avma;
  y = addrr_sign(itor(x,l), sx, y, sy);
  ly = lg(y); while (ly--) *--z = y[ly];
  avma = (pari_sp)z; return z;
}

GEN
addsr(long x, GEN y)
{
  if (!x) return rcopy(y);
  if (x>0) { pos_s[2]=x; return addir_sign(pos_s, 1, y, signe(y)); }
  neg_s[2] = -x; return addir_sign(neg_s, -1, y, signe(y));
}

GEN
subsr(long x, GEN y)
{
  if (!x) return rcopy_sign(y, -signe(y));
  if (x>0) { pos_s[2]=x; return addir_sign(pos_s, 1, y, -signe(y)); }
  neg_s[2] = -x; return addir_sign(neg_s, -1, y, -signe(y));
}

/* return x + 1, assuming x > 0 is a normalized t_REAL of exponent 0 */
GEN
addrex01(GEN x)
{
  long l = lg(x);
  GEN y = cgetr(l);
  y[1] = evalsigne(1) | evalexpo(1);
  y[2] = HIGHBIT | (((ulong)x[2] & ~HIGHBIT) >> 1);
  shift_right(y, x, 3,l, x[2], 1); 
  return y;
}
/* return x - 1 to same accuracy as x, assuming x > 1 is a normalized t_REAL
 * of exponent 0 */
GEN
subrex01(GEN x)
{
  long i, sh, k, l = lg(x);
  ulong u;
  GEN y = cgetr(l);
  k = 2;
  u = (ulong)x[2] & (~HIGHBIT);
  while (!u) u = x[++k]; /* terminates: x not a power of 2 */
  sh = bfffo(u);
  if (sh)
  { shift_left(y+2, x+k, 0, l-k-1, 0, sh); }
  else
  { for (i = 2; i < l-k+2; i++) y[i] = x[k-2 + i]; }
  for (i = l-k+2; i < l; i++) y[i] = 0;
  y[1] = evalsigne(1) | evalexpo(- ((k-2)*BITS_IN_LONG + sh));
  return y;
}

GEN
addrr_sign(GEN x, long sx, GEN y, long sy)
{
  long lx, ex = expo(x);
  long ly, ey = expo(y), e = ey - ex;
  long i, j, lz, ez, m;
  int extend, f2;
  GEN z;
  LOCAL_OVERFLOW;

  if (!sy)
  {
    if (!sx)
    {
      if (e > 0) ex = ey;
      return real_0_bit(ex);
    }
    if (e > 0) return real_0_bit(ey);
    lz = 3 + ((-e)>>TWOPOTBITS_IN_LONG);
    lx = lg(x); if (lz > lx) lz = lx;
    z = cgetr(lz); while(--lz) z[lz] = x[lz];
    setsigne(z,sx); return z;
  }
  if (!sx)
  {
    if (e < 0) return real_0_bit(ex);
    lz = 3 + (e>>TWOPOTBITS_IN_LONG);
    ly = lg(y); if (lz > ly) lz = ly;
    z = cgetr(lz); while (--lz) z[lz] = y[lz];
    setsigne(z,sy); return z;
  }

  if (e < 0) { z=x; x=y; y=z; ey=ex; i=sx; sx=sy; sy=i; e=-e; }
  /* now ey >= ex */
  lx = lg(x);
  ly = lg(y);
  /* If exponents differ, need to shift one argument, here x. If
   * extend = 1: extension of x,z by m < BIL bits (round to 1 word) */
  /* in this case, lz = lx + d + 1, otherwise lx + d */
  extend = 0;
  if (e)
  {
    long d = e >> TWOPOTBITS_IN_LONG, l = ly-d;
    if (l <= 2) return rcopy_sign(y, sy);
    m = e & (BITS_IN_LONG-1);
    if (l > lx) { lz = lx + d + 1; extend = 1; }
    else        { lz = ly; lx = l; }
    if (m)
    { /* shift x right m bits */
      const pari_sp av = avma;
      const ulong sh = BITS_IN_LONG-m;
      GEN p1 = x; x = new_chunk(lx + lz + 1);
      shift_right2(x,p1,2,lx, 0,m,sh);
      if (extend) x[lx] = p1[lx-1] << sh;
      avma = av; /* HACK: cgetr(lz) will not overwrite x */
    }
  }
  else
  { /* d = 0 */
    m = 0;
    if (lx > ly) lx = ly;
    lz = lx;
  }

  if (sx == sy)
  { /* addition */
    i = lz-1;
    j = lx-1;
    if (extend) {
      ulong garde = addll(x[lx], y[i]);
      if (m < 4) /* don't extend for few correct bits */
        z = cgetr(--lz);
      else
      {
        z = cgetr(lz);
        z[i] = garde;
      }
    }
    else
    {
      z = cgetr(lz);
      z[i] = addll(x[j], y[i]); j--;
    }
    i--;
    for (; j>=2; i--,j--) z[i] = addllx(x[j],y[i]);
    if (overflow)
    {
      z[1] = 1; /* stops since z[1] != 0 */
      for (;;) { z[i] = y[i]+1; if (z[i--]) break; }
      if (i <= 0)
      {
        shift_right(z,z, 2,lz, 1,1);
        z[1] = evalsigne(sx) | evalexpo(ey+1); return z;
      }
    }
    for (; i>=2; i--) z[i] = y[i];
    z[1] = evalsigne(sx) | evalexpo(ey); return z;
  }

  /* subtraction */
  if (e) f2 = 1;
  else
  {
    i = 2; while (i < lx && x[i] == y[i]) i++;
    if (i==lx) return real_0_bit(ey - bit_accuracy(lx));
    f2 = ((ulong)y[i] > (ulong)x[i]);
  }
  /* result is non-zero. f2 = (y > x) */
  i = lz-1; z = cgetr(lz);
  if (f2)
  {
    j = lx-1;
    if (extend) z[i] = subll(y[i], x[lx]);
    else        z[i] = subll(y[i], x[j--]);
    for (i--; j>=2; i--) z[i] = subllx(y[i], x[j--]);
    if (overflow) /* stops since y[1] != 0 */
      for (;;) { z[i] = y[i]-1; if (y[i--]) break; }
    for (; i>=2; i--) z[i] = y[i];
    sx = sy;
  }
  else
  {
    if (extend) z[i] = subll(x[lx], y[i]);
    else        z[i] = subll(x[i],  y[i]);
    for (i--; i>=2; i--) z[i] = subllx(x[i], y[i]);
  }

  x = z+2; i = 0; while (!x[i]) i++;
  lz -= i; z += i;
  j = bfffo(z[2]); /* need to shift left by j bits to normalize mantissa */
  ez = ey - (j | (i<<TWOPOTBITS_IN_LONG));
  if (extend)
  { /* z was extended by d+1 words [should be e bits = d words + m bits] */
    /* not worth keeping extra word if less than 5 significant bits in there */
    if (m - j < 5 && lz > 3)
    { /* shorten z */
      ulong last = (ulong)z[--lz]; /* cancelled word */

      /* if we need to shift anyway, shorten from left
       * If not, shorten from right, neutralizing last word of z */
      if (j == 0)
        /* stackdummy((pari_sp)(z + lz+1), (pari_sp)(z + lz)); */
        z[lz] = evaltyp(t_VECSMALL) | _evallg(1);
      else
      {
        GEN t = z;
        z++; shift_left(z,t,2,lz-1, last,j);
      }
      if ((last<<j) & HIGHBIT)
      { /* round up */
        i = lz-1;
        while (++z[i] == 0 && i > 1) i--;
        if (i == 1) { ez++; z[2] = HIGHBIT; }
      }
    }
    else if (j) shift_left(z,z,2,lz-1, 0,j);
  }
  else if (j) shift_left(z,z,2,lz-1, 0,j);
  z[1] = evalsigne(sx) | evalexpo(ez);
  z[0] = evaltyp(t_REAL) | evallg(lz);
  avma = (pari_sp)z; return z;
}
