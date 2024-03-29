/* $Id: subcyclo.c 8026 2006-09-09 21:41:06Z kb $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include "pari.h"
#include "paripriv.h"

/*************************************************************************/
/**                                                                     **/
/**              Routines for handling subgroups of (Z/nZ)^*            **/
/**              without requiring discrete logarithms.                 **/
/**                                                                     **/
/*************************************************************************/

/* Subgroups are [gen,ord,bits] where 
 * gen is a vecsmall of generators 
 * ord is theirs relative orders
 * bits is a bit vector of the elements, of length(n).
 */

 /*The algorithm is similar to testpermutation*/
void
znstar_partial_coset_func(long n, GEN H, void (*func)(void *data,long c)
    , void *data, long d, long c)
{
  GEN gen = gel(H,1);
  GEN ord = gel(H,2);
  GEN cache = const_vecsmall(d,c);
  long i, j, card = 1;

  (*func)(data,c);
  for (i = 1; i <= d; i++) card *= ord[i];
  for(i=1; i<card; i++)
  {
    long k, m = i;
    for(j=1; j<d && m%ord[j]==0 ;j++) m /= ord[j];
    cache[j] = Fl_mul(cache[j],gen[j],n);
    for (k=1; k<j; k++) cache[k] = cache[j];
    (*func)(data, cache[j]);
  }
}

void
znstar_coset_func(long n, GEN H, void (*func)(void *data,long c)
    , void *data, long c)
{
  znstar_partial_coset_func(n, H, func,data, lg(H[1])-1, c);
}

/*Add the element of the bitvec of the coset c modulo the subgroup of H
 * generated by the first d generators to the bitvec bits.*/

void
znstar_partial_coset_bits_inplace(long n, GEN H, GEN bits, long d, long c)
{
  pari_sp ltop=avma;
  znstar_partial_coset_func(n,H, (void (*)(void *,long)) &bitvec_set,
      (void *) bits, d, c);
  avma=ltop;
}

void
znstar_coset_bits_inplace(long n, GEN H, GEN bits, long c)
{
  znstar_partial_coset_bits_inplace(n, H, bits, lg(H[1])-1, c);
}

GEN
znstar_partial_coset_bits(long n, GEN H, long d, long c)
{
  GEN bits=bitvec_alloc(n);
  znstar_partial_coset_bits_inplace(n,H,bits,d,c);
  return bits;
}
  
/*Compute the bitvec of the elements of the  subgroup of H generated by the
 * first d generators.
 */

GEN
znstar_coset_bits(long n, GEN H, long c)
{
  return znstar_partial_coset_bits(n, H, lg(H[1])-1, c);
}

/*Compute the bitvec of the elements of the  subgroup of H generated by the
 * first d generators.*/

GEN
znstar_partial_bits(long n, GEN H, long d)
{
  return znstar_partial_coset_bits(n, H, d, 1);
}
/*Compute the bitvec of the elements of H.*/

GEN
znstar_bits(long n, GEN H)
{
  return znstar_partial_bits(n,H,lg(H[1])-1);
}

/*Compute the subgroup of (Z/nZ)^* generated by the elements of
 * the vecsmall V.
 */

GEN
znstar_generate(long n, GEN V)
{
  pari_sp ltop=avma;
  GEN res=cgetg(4,t_VEC);
  GEN gen=cgetg(lg(V),t_VECSMALL);
  GEN ord=cgetg(lg(V),t_VECSMALL);
  GEN bits;
  long i,r=0;
  gel(res,1) = gen;
  gel(res,2) = ord;
  bits=znstar_partial_bits(n,res,r);
  for(i=1;i<lg(V);i++)
  {
    ulong v = (ulong)V[i], g = v;
    long o = 0;
    while (!bitvec_test(bits, (long)g))
    {
      g = Fl_mul(g, v, (ulong)n);
      o++;
    }
    if (o)
    {
      r++;
      gen[r]=v;
      ord[r]=o+1;
      cgiv(bits); 
      bits=znstar_partial_bits(n,res,r);
    }
  }
  setlg(gen,r+1);
  setlg(ord,r+1);
  gel(res,3) = bits;
  return gerepilecopy(ltop,res);
}

/* Return the lists of element of H.
 * This can be implemented with znstar_coset_func instead.
 */

GEN
znstar_elts(long n, GEN H)
{
  long card=group_order(H);
  GEN gen=gel(H,1), ord=gel(H,2);
  GEN sg = cgetg(1 + card, t_VECSMALL);
  long k, j, l;
  sg[1] = 1;
  for (j = 1, l = 1; j < lg(gen); j++)
  {
    long c = l * (ord[j] - 1);
    for (k = 1; k <= c; k++)	/* I like it */
      sg[++l] = Fl_mul((ulong)sg[k], (ulong)gen[j], (ulong)n);
  }
  vecsmall_sort(sg);
  return sg;
}

/* Take a znstar H and n dividing the modulus of H.
 * Output H reduced to modulus n */

GEN
znstar_reduce_modulus(GEN H, long n)
{
  pari_sp ltop=avma;
  GEN gen=cgetg(lg(H[1]),t_VECSMALL);
  long i;
  for(i=1; i < lg(gen); i++)
    gen[i] = mael(H,1,i)%n;
  return gerepileupto(ltop, znstar_generate(n,gen));
}

/*Compute conductor of H*/
long
znstar_conductor(long n, GEN H)
{
  pari_sp ltop=avma;
  long i,j;
  GEN F = factoru(n), P = gel(F,1), E = gel(F,2); 
  long cnd=n;
  for(i=lg(gel(F,1))-1;i>0;i--)
  {
    long p = P[i], e = E[i], q = n;
    if (DEBUGLEVEL>=4)
      fprintferr("SubCyclo: testing %ld^%ld\n",p,e);
    for (  ; e>=1; e--)
    {
      long z = 1;
      q /= p;
      for (j = 1; j < p; j++)
      {
	z += q;
	if (!bitvec_test(gel(H,3),z) && cgcd(z,n)==1)
          break;
      } 
      if ( j < p )
      {
	if (DEBUGLEVEL>=4)
	  fprintferr("SubCyclo: %ld not found\n",z);
	break;
      }
      cnd /= p;
      if (DEBUGLEVEL>=4)
	fprintferr("SubCyclo: new conductor:%ld\n",cnd);
    }
  } 
  if (DEBUGLEVEL>=6)
    fprintferr("SubCyclo: conductor:%ld\n",cnd);
  avma=ltop;
  return cnd;
}

/* Compute the orbits of a subgroups of Z/nZ given by a generator
 * or a set of generators given as a vector. 
 */
GEN
znstar_cosets(long n, long phi_n, GEN H)
{
  long    k;
  long    c = 0;
  long    card   = group_order(H);
  long    index  = phi_n/card;
  GEN     cosets = cgetg(index+1,t_VECSMALL);
  pari_sp ltop = avma;
  GEN     bits   = bitvec_alloc(n);
  for (k = 1; k <= index; k++)
  {
    for (c++ ; bitvec_test(bits,c) || cgcd(c,n)!=1; c++);
    cosets[k]=c;
    znstar_coset_bits_inplace(n, H, bits, c);
  }
  avma=ltop;
  return cosets;
}


/*************************************************************************/
/**                                                                     **/
/**                     znstar/HNF interface                            **/
/**                                                                     **/
/*************************************************************************/

/* Convert a true znstar output by znstar to a `small znstar' */
GEN
znstar_small(GEN zn)
{
  GEN Z = cgetg(4,t_VEC);
  gel(Z,1) = icopy(gmael3(zn,3,1,1));
  gel(Z,2) = gtovecsmall(gel(zn,2));
  gel(Z,3) = lift(gel(zn,3)); return Z;
}

/* Compute generators for the subgroup of (Z/nZ)* given in HNF. */
GEN
znstar_hnf_generators(GEN Z, GEN M)
{
  long l = lg(M);
  GEN gen = cgetg(l, t_VECSMALL);
  pari_sp ltop = avma;
  GEN zgen = gel(Z,3);
  ulong n = itou(gel(Z,1));
  long j, h;
  for (j = 1; j < l; j++)
  {
    gen[j] = 1;
    for (h = 1; h < l; h++)
      gen[j] = Fl_mul((ulong)gen[j], 
                      Fl_pow(itou(gel(zgen,h)), itou(gmael(M,j,h)), n), n);
  }
  avma = ltop; return gen;
}

GEN
znstar_hnf(GEN Z, GEN M)
{
  return znstar_generate(itos(gel(Z,1)),znstar_hnf_generators(Z,M));
}

GEN
znstar_hnf_elts(GEN Z, GEN H)
{
  pari_sp ltop = avma;
  GEN G = znstar_hnf(Z,H);
  long n = itos(gel(Z,1));
  return gerepileupto(ltop, znstar_elts(n,G));
}

/*************************************************************************/
/**                                                                     **/
/**                     subcyclo                                        **/
/**                                                                     **/
/*************************************************************************/

static GEN gscycloconductor(GEN g, long n, long flag)
{
  if (flag==2)
  {
    GEN V=cgetg(3,t_VEC);
    gel(V,1) = gcopy(g);
    gel(V,2) = stoi(n);
    return V;
  }
  return g;
}

static long 
lift_check_modulus(GEN H, long n)
{
  long t=typ(H);
  long h;
  switch(t)
  {
    case t_INTMOD:
      if (!equalsi(n, gel(H,1)))
	pari_err(talker,"wrong modulus in galoissubcyclo");
      H = gel(H,2);
    case t_INT:
      h=smodis(H,n);
      if (cgcd(h,n)!=1)
	pari_err(talker,"generators must be prime to conductor in galoissubcyclo");
      return h;
  }
  pari_err(talker,"wrong type in galoissubcyclo");
  return 0;/*not reached*/
}

/* Compute z^ex using the baby-step/giant-step table powz 
 * with only one multiply.
 * In the modular case, the result is not reduced.
 */

static GEN subcyclo_powz(GEN powz, long ex)
{
  long m=lg(powz[1])-1;
  long q=ex/m, r=ex%m; /*ex=m*q+r*/
  GEN z=gmul(gmael(powz,1,r+1),gmael(powz,2,q+1));
  if (lg(powz)==4) z=greal(z);
  return z;
}

GEN subcyclo_complex_bound(pari_sp ltop, GEN V, long prec)
{
  GEN pol = roots_to_pol(V,0);
  GEN vec = gtovec(real_i(pol));
  GEN borne = ceil_safe(supnorm(vec,prec));
  return gerepileupto(ltop,borne);
}

/* Newton sums mod le. if le==NULL, works with complex instead */
GEN
subcyclo_cyclic(long n, long d, long m ,long z, long g, GEN powz, GEN le)
{
  GEN V=cgetg(d+1,t_VEC);
  ulong base=1;
  long i,k;
  for (i=1; i<=d; i++,base = Fl_mul(base,z,n))
  {
    pari_sp av = avma;
    long ex = base;
    GEN s = gen_0;
    for (k=0; k<m; k++, ex = Fl_mul(ex,g,n)) 
    {
      s = gadd(s,subcyclo_powz(powz,ex));
      if ((k&0xff)==0) s=gerepileupto(av,s);
    }
    if (le) s = modii(s, le);
    gel(V,i) = gerepileupto(av, s);
  }
  return V;
}

struct _subcyclo_orbits_s
{
  GEN powz;
  GEN *s;
  ulong count;
  pari_sp ltop;
};

static void
_subcyclo_orbits(struct _subcyclo_orbits_s *data, long k)
{
  GEN powz = data->powz;
  GEN *s = data->s;
  
  if (!data->count) data->ltop= avma;
  *s = gadd(*s,subcyclo_powz(powz,k));
  data->count++;
  if ((data->count & 0xffUL) == 0)
    *s = gerepileupto(data->ltop, *s);
}

/* Newton sums mod le. if le==NULL, works with complex instead */
GEN
subcyclo_orbits(long n, GEN H, GEN O, GEN powz, GEN le)
{
  long i, d=lg(O);
  GEN V=cgetg(d,t_VEC);
  struct _subcyclo_orbits_s data;
  long lle=le?lg(le)*2+1:2*lg(gmael(powz,1,2))+3;/*Assume dvmdii use lx+ly space*/
  data.powz = powz;
  for(i=1; i<d; i++)
  {
    GEN s = gen_0;
    pari_sp av = avma;
    (void)new_chunk(lle);
    data.count = 0;
    data.s     = &s;
    znstar_coset_func(n, H, (void (*)(void *,long)) _subcyclo_orbits,
      (void *) &data, O[i]);
    avma = av; /* HACK */
    gel(V,i) = le? modii(s,le): gcopy(s);
  }
  return V;
}

GEN 
subcyclo_start(long n, long d, long o, GEN borne, long *ptr_val,long *ptr_l)
{
  pari_sp av;
  GEN l,le,z;
  long i;
  long e,val;
  if (DEBUGLEVEL >= 1) (void)timer2();
  l=utoipos(n+1);e=1;
  while(!isprime(l)) { l=addis(l,n); e++; }
  if (DEBUGLEVEL >= 4)
    fprintferr("Subcyclo: prime l=%Z\n",l);
  av=avma;
  if (!borne)
  {
    /*We use the following trivial bound:
      Vecmax(Vec((x+o)^d)=max{binomial(d,i)*o^i ;1<=i<=d} 
     */
    i=d-(1+d)/(1+o);
    borne=mulii(binomial(utoipos(d),i),powuu(o,i));
  }
  if (DEBUGLEVEL >= 4)
    fprintferr("Subcyclo: borne=%Z\n",borne);
  val=logint(shifti(borne,2),l,NULL);
  avma=av;
  if (DEBUGLEVEL >= 4)
    fprintferr("Subcyclo: val=%ld\n",val);
  le=powiu(l,val);
  z = Fp_pow(gener_Fp(l), utoipos(e), l);
  z=padicsqrtnlift(gen_1,utoipos(n),z,l,val);
  if (DEBUGLEVEL >= 1)
    msgtimer("padicsqrtnlift.");
  *ptr_val=val;
  *ptr_l=itos(l);
  return gmodulo(z,le);
}

/*Fill in the powz table:
 *  powz[1]: baby-step 
 *  powz[2]: giant-step
 *  powz[3] exists only if the field is real (value is ignored).
 */
GEN
subcyclo_complex_roots(long n, long real, long prec)
{
  long i;
  long m = (long)(1+sqrt((double) n));
  GEN powbab, powgig, powz;
  powz      = cgetg(real?4:3,t_VEC);
  powbab    = cgetg(m+1,t_VEC);
  gel(powbab,1) = gen_1;
  gel(powbab,2) = exp_Ir(divrs(Pi2n(1, prec), n)); /* = e_n(1) */
  for (i=3; i<=m; i++) gel(powbab,i) = gmul(gel(powbab,2),gel(powbab,i-1));
  powgig    = cgetg(m+1,t_VEC);
  gel(powgig,1) = gen_1;
  gel(powgig,2) = gmul(gel(powbab,2),gel(powbab,m));;
  for (i=3; i<=m; i++) gel(powgig,i) = gmul(gel(powgig,2),gel(powgig,i-1));
  gel(powz,1) = powbab;
  gel(powz,2) = powgig;
  if (real) gel(powz,3) = gen_0;
  return powz;
}

static GEN muliimod_sz(GEN x, GEN y, GEN l, long siz)
{
  pari_sp av=avma;
  GEN p1;
  (void)new_chunk(siz); /* HACK */
  p1 = mulii(x,y);
  avma=av;
  return modii(p1,l);
}

GEN
subcyclo_roots(long n, GEN zl)
{
  GEN le=gel(zl,1);
  GEN z=gel(zl,2);
  long lle=lg(le)*3; /*Assume dvmdii use lx+ly space*/
  long i;
  long m = (long)(1+sqrt((double) n));
  GEN powbab, powgig, powz;
  powz=cgetg(3,t_VEC);
  powbab    = cgetg(m+1,t_VEC);
  gel(powbab,1) = gen_1;
  gel(powbab,2) = gcopy(z);
  for (i=3; i<=m; i++) 
    gel(powbab,i) = muliimod_sz(z,gel(powbab,i-1),le,lle);
  powgig    = cgetg(m+1,t_VEC);
  gel(powgig,1) = gen_1;
  gel(powgig,2) = muliimod_sz(z,gel(powbab,m),le,lle);;
  for (i=3; i<=m; i++)
    gel(powgig,i) = muliimod_sz(gel(powgig,2),gel(powgig,i-1),le,lle);
  gel(powz,1) = powbab;
  gel(powz,2) = powgig;
  return powz;
}

GEN
galoiscyclo(long n, long v)
{
  ulong ltop=avma;
  GEN grp,G;
  GEN z, le;
  long val,l;
  GEN L;
  long i,j,k;
  GEN zn=znstar(stoi(n));
  long card=itos(gel(zn,1));
  GEN gen=lift(gel(zn,3));
  GEN ord=gtovecsmall(gel(zn,2));
  GEN elts;
  z=subcyclo_start(n,card/2,2,NULL,&val,&l);
  le=gel(z,1);
  z=gel(z,2);
  L = cgetg(1+card,t_VEC);
  gel(L,1) = z;
  for (j = 1, i = 1; j < lg(gen); j++)
  {
    long c = i * (ord[j] - 1);
    for (k = 1; k <= c; k++)	/* I like it */
      gel(L,++i) = Fp_pow(gel(L,k),gel(gen,j),le);
  }
  G=abelian_group(ord);
  elts = group_elts(G, card); /*not stack clean*/
  grp = cgetg(9, t_VEC);
  gel(grp,1) = cyclo(n,v);
  gel(grp,2) = cgetg(4,t_VEC); 
  gmael(grp,2,1) = stoi(l);
  gmael(grp,2,2) = stoi(val);
  gmael(grp,2,3) = icopy(le);
  gel(grp,3) = gcopy(L);
  gel(grp,4) = vandermondeinversemod(L, gel(grp,1), gen_1, le);
  gel(grp,5) = gen_1;
  gel(grp,6) = gcopy(elts);
  gel(grp,7) = gcopy(gel(G,1));
  gel(grp,8) = gcopy(gel(G,2));
  return gerepileupto(ltop,grp);
}

/* Convert a bnrinit(Q,n) to a znstar(n)
 * complex is set to 0 if the bnr is real and to 1 if it is complex.
 * Not stack clean */
GEN
bnr_to_znstar(GEN bnr, long *complex)
{
  GEN zk, gen, cond, v;
  long l2, i;
  checkbnrgen(bnr);
  if (degpol(gmael3(bnr,1,7,1))!=1)
    pari_err(talker,"bnr must be over Q in bnr_to_znstar");
  zk = gel(bnr,5);
  gen = gel(zk,3);
  /* cond is the finite part of the conductor,
   * complex is the infinite part*/
  cond = gcoeff(gmael3(bnr,2,1,1), 1, 1);
  *complex = signe(gmael4(bnr,2,1,2,1));
  l2 = lg(gen);
  v = cgetg(l2, t_VEC);
  for (i = 1; i < l2; ++i)
  {
    GEN x=gel(gen,i);
    if (typ(x) == t_MAT)
      x = gcoeff(x, 1, 1);
    else if (typ(x) == t_COL)
      x = gel(x,1);
    gel(v,i) = gmodulo(absi(x), cond);
  }
  return mkvec3(gel(zk,1), gel(zk,2), v);
}

GEN 
galoissubcyclo(GEN N, GEN sg, long flag, long v)
{
  pari_sp ltop=avma,av;
  GEN H, V;
  long i;
  GEN O;
  GEN Z=NULL;
  GEN B,zl,L,T,le,powz;
  long val,l;
  long n, cnd, complex=1;
  long card, phi_n;
  if (flag<0 || flag>2) pari_err(flagerr,"galoissubcyclo");
  if ( v==-1 ) v=0;
  if (!sg) sg=gen_1;
  switch(typ(N))
  {
    case t_INT:
      n = itos(N);
      if (n < 1) pari_err(talker,"degree <= 0 in galoissubcyclo");
      break;
    case t_VEC:
      if (lg(N)==7)
        N=bnr_to_znstar(N,&complex);
      if (lg(N)==4)
      {
        Z = N;
        if (typ(Z[3])!=t_VEC)
          pari_err(typeer,"galoissubcyclo");
        if (lg(Z[3])==1)
          n=1;
        else
        {
          if (typ(gmael(Z,3,1))!= t_INTMOD)
#ifdef NETHACK_MESSAGES
            pari_err(talker,"You have transgressed!");
#else
            pari_err(talker,"Please do not try to break PARI with ridiculous counterfeit data. Thanks!");
#endif
          n=itos(gmael3(Z,3,1,1));
        }
        break;
      }
    default: /*fall through*/
      pari_err(typeer,"galoissubcyclo");
      return NULL;/*Not reached*/
  }
  if (n==1) {avma=ltop; return deg1pol(gen_1,gen_m1,v);}
  switch(typ(sg))
  {
     case t_INTMOD: case t_INT: 
      V = mkvecsmall( lift_check_modulus(sg,n) );
      break;
    case t_VECSMALL:
      V = gcopy(sg);
      for (i=1;i<lg(V);i++)
      {
        V[i] %= n; if (V[i] < 0) V[i] += n;
      }
      break;
    case t_VEC:
    case t_COL:
      V = cgetg(lg(sg),t_VECSMALL);
      for(i=1;i<lg(sg);i++) V[i] = lift_check_modulus(gel(sg,i),n);
      break;
    case t_MAT:/*Fall through*/
      {
        if (lg(sg) == 1 || lg(sg) != lg(sg[1]))
          pari_err(talker,"not a HNF matrix in galoissubcyclo");
        if (!Z)
          pari_err(talker,"N must be a bnrinit or a znstar if H is a matrix in galoissubcyclo");
        if ( lg(Z[2]) != lg(sg) || lg(Z[3]) != lg(sg))
          pari_err(talker,"Matrix of wrong dimensions in galoissubcyclo");
        V = znstar_hnf_generators(znstar_small(Z),sg);
      }
      break;
    default:
      pari_err(typeer,"galoissubcyclo");
      return NULL;/*Not reached*/
  }
  if (!complex) /*Add complex conjugation*/
    V=vecsmall_append(V,n-1);
  H = znstar_generate(n,V);
  /* compute the complex/real status
   * it is real iff z -> conj(z)=z^-1=z^(n-1) is in H
   */
  if (DEBUGLEVEL >= 6)
  {
    fprintferr("Subcyclo: elements:");
    for (i=1;i<n;i++)
      if (bitvec_test(gel(H,3),i))
        fprintferr(" %ld",i);
    fprintferr("\n");
  }
  complex = !bitvec_test(gel(H,3),n-1);
  if (DEBUGLEVEL >= 6)
    fprintferr("Subcyclo: complex=%ld\n",complex);
  if (DEBUGLEVEL >= 1) (void)timer2();
  cnd = znstar_conductor(n,H);
  if (DEBUGLEVEL >= 1)
    msgtimer("znstar_conductor");
  if ( flag == 1 )  { avma=ltop; return stoi(cnd); }
  if (cnd == 1) 
  { 
    avma=ltop; 
    return gscycloconductor(deg1pol(gen_1,gen_m1,v),1,flag);
  }
  if (n != cnd)
  {
    H = znstar_reduce_modulus(H, cnd);
    n = cnd;
  }
  card = group_order(H);
  phi_n= itos(phi(utoipos(n)));
  if ( card==phi_n )
  {
    avma=ltop;
    if (flag==3) return galoiscyclo(n,v);
    return gscycloconductor(cyclo(n,v),n,flag); 
  }
  O = znstar_cosets(n, phi_n, H);
  if (DEBUGLEVEL >= 1)
    msgtimer("znstar_cosets");
  if (DEBUGLEVEL >= 6)
    fprintferr("Subcyclo: orbits=%Z\n",O);
  if (DEBUGLEVEL >= 4)
    fprintferr("Subcyclo: %ld orbits with %ld elements each\n",phi_n/card,card);
  av=avma;
  powz=subcyclo_complex_roots(n,!complex,3);
  L=subcyclo_orbits(n,H,O,powz,NULL);
  B=subcyclo_complex_bound(av,L,3);
  zl=subcyclo_start(n,phi_n/card,card,B,&val,&l);
  powz=subcyclo_roots(n,zl);
  le=gel(zl,1);
  L=subcyclo_orbits(n,H,O,powz,le);
  T=FpV_roots_to_pol(L,le,v);
  T=FpX_center(T,le);
  return gerepileupto(ltop,gscycloconductor(T,n,flag));
}

GEN
subcyclo(long n, long d, long v)
{
  pari_sp ltop=avma;
  long o,p,al,r,g,gd;
  GEN fa;
  GEN zl,L,T,le;
  long l,val;
  GEN B,powz;
  if (v<0) v = 0;
  if (d==1) return deg1pol(gen_1,gen_m1,v);
  if (d<=0 || n<=0) pari_err(typeer,"subcyclo");
  if ((n & 3) == 2) n >>= 1;
  if (n == 1 || d >= n) pari_err(talker,"degree does not divide phi(n) in subcyclo");
  fa = factoru(n);
  p = mael(fa,1,1);
  al= mael(fa,2,1);
  if (lg(gel(fa,1)) > 2 || (p==2 && al>2))
    pari_err(talker,"non-cyclic case in polsubcyclo: use galoissubcyclo instead");
  avma=ltop;
  r = cgcd(d,n); /* = p^(v_p(d))*/
  n = r*p;
  o = n-r; /* = phi(n) */
  if (o == d) return cyclo(n,v);
  if (o % d) pari_err(talker,"degree does not divide phi(n) in subcyclo");
  o /= d;
  if (p==2) {
    GEN z = mkpoln(3, gen_1,gen_0,gen_1); /* x^2 + 1 */
    setvarn(z,v); return z;
  }
  g=itos(gel(gener(utoipos(n)), 2));
  gd = Fl_pow(g, d, n);
  avma=ltop;
  powz=subcyclo_complex_roots(n,(o&1)==0,3);
  L=subcyclo_cyclic(n,d,o,g,gd,powz,NULL);
  B=subcyclo_complex_bound(ltop,L,3);
  zl=subcyclo_start(n,d,o,B,&val,&l);
  le=gel(zl,1);
  powz=subcyclo_roots(n,zl);
  if (DEBUGLEVEL >= 6) msgtimer("subcyclo_roots"); 
  L=subcyclo_cyclic(n,d,o,g,gd,powz,le);
  if (DEBUGLEVEL >= 6) msgtimer("subcyclo_cyclic"); 
  T=FpV_roots_to_pol(L,le,v);
  if (DEBUGLEVEL >= 6) msgtimer("roots_to_pol"); 
  T=FpX_center(T,le);
  return gerepileupto(ltop,T);
}

GEN polsubcyclo(long n, long d, long v)
{
  pari_sp ltop=avma;
  GEN L, Z=znstar(stoi(n));
  /*subcyclo is twice faster but Z must be cyclic*/
  if (lg(Z[2]) == 2 && dvdii(gel(Z,1),stoi(d)))
  {
    avma=ltop; 
    return subcyclo(n, d, v);
  }
  L=subgrouplist(gel(Z,2), mkvec(stoi(d)));
  if (lg(L) == 2)
    return gerepileupto(ltop, galoissubcyclo(Z, gel(L,1), 0, v));
  else
  {
    GEN V=cgetg(lg(L),t_VEC);
    long i;
    for (i=1; i< lg(V); i++)
      gel(V,i) = galoissubcyclo(Z, gel(L,i), 0, v);
    return gerepileupto(ltop,V);
  }
}
