/* $Id: subgroup.c 7522 2005-12-09 18:14:24Z kb $

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

void push_val(entree *ep, GEN a);
void pop_val(entree *ep);

typedef struct {
  entree *ep;
  char *s;
} expr_t;

typedef struct slist {
  struct slist *next;
  long *data;
} slist;

typedef struct {
  GEN hnfgroup, gen;
  ulong count;
  slist *list;
} sublist_t;

/* MAX: [G:H] <= bound, EXACT: [G:H] = bound, TYPE: type(H) = bound */
enum { b_NONE, b_MAX, b_EXACT, b_TYPE };

/* SUBGROUPS
 * G = Gp x I, with Gp a p-Sylow (I assumed small).
 * Compute subgroups of I by recursive calls
 * Loop through subgroups Hp of Gp using Birkhoff's algorithm.
 * If (I is non trivial)
 *   lift Hp to G (mul by exponent of I)
 *   for each subgp of I, lift it to G (mult by exponent of Gp)
 *   consider the group generated by the two subgroups (concat)
 *
 * type(H) = mu --> H = Z/p^mu[1] x ... x Z/p^mu[len(mu)] */
typedef struct subgp_iter {
  long *M, *L; /* mu = p-subgroup type, lambda = p-group type */
  long *powlist; /* [i] = p^i, i = 0.. */
  long *c, *maxc, *a, *maxa, **g, **maxg;
  long *available;
  GEN **H; /* p-subgroup of type mu, in matrix form */
  GEN cyc; /* cyclic factors of G */
  GEN subq;/* subgrouplist(I) */
  GEN subqpart; /* J in subq s.t [I:J][Gp:Hp] <= indexbound */
  GEN bound; /* if != NULL, impose a "bound" on [G:H] (see boundtype) */
  long boundtype;
  long countsub; /* number of subgroups of type M (so far) */
  long count; /* number of p-subgroups so far [updated when M completed] */
  GEN expoI; /* exponent of I */
  void(*fun)(struct subgp_iter *, GEN); /* applied to each subgroup */
  void *fundata; /* for fun */
} subgp_iter;

#define len(x)      (x)[0]
#define setlen(x,l) len(x)=(l)

static void
printtyp(const long *typ) /*Used only for ddebugging */
{
  long i, l = len(typ);
  for (i=1; i<=l; i++) fprintferr(" %ld ",typ[i]);
  fprintferr("\n");
}

/* compute conjugate partition of typ */
static long*
conjugate(long *typ)
{
  long *t, i, k = len(typ), l, last;

  if (!k) { t = new_chunk(1); setlen(t,0); return t; }
  l = typ[1]; t = new_chunk(l+2);
  t[1] = k; last = k;
  for (i=2; i<=l; i++)
  {
    while (typ[last] < i) last--;
    t[i] = last;
  }
  t[i] = 0; setlen(t,l);
  return t;
}
/* --- subgp_iter 'fun' associated to forsubgroup -------------- */
static void
std_fun(subgp_iter *T, GEN x)
{
  expr_t *E = (expr_t *)T->fundata;
  E->ep->value = (void*)x;
  (void)readseq(E->s); T->countsub++;
}
/* ----subgp_iter 'fun' associated to subgrouplist ------------- */
static void
addcell(sublist_t *S, GEN H)
{
  long *pt,i,j, k = 0, n = lg(H)-1;
  slist *cell = (slist*) gpmalloc(sizeof(slist) + n*(n+1)/2 * sizeof(long));

  S->list->next = cell; cell->data = pt = (long*) (cell + 1);
  for (j=1; j<=n; j++)
    for(i=1; i<=j; i++) pt[k++] = itos(gcoeff(H,i,j));
  S->list = cell;
  S->count++;
}

static void
list_fun(subgp_iter *T, GEN x)
{
  sublist_t *S = (sublist_t*)T->fundata;
  GEN H = hnf(shallowconcat(S->hnfgroup,x));
  if (S->gen)
  { /* test conductor */
    long i, l = lg(S->gen);
    for (i = 1; i < l; i++)
      if ( hnf_gauss(H, (GEN)S->gen[i]) ) return;
  }
  addcell(S, H); T->countsub++;
}
/* -------------------------------------------------------------- */

/* treat subgroup Hp (not in HNF, T->fun should do it if desired) */
static void
treatsub(subgp_iter *T, GEN H)
{
  long i;
  if (!T->subq) T->fun(T, H);
  else
  { /* not a p group, add the trivial part */
    GEN Hp = gmul(T->expoI, H); /* lift H to G */
    long l = lg(T->subqpart);
    for (i=1; i<l; i++)
      T->fun(T, shallowconcat(Hp, (GEN)T->subqpart[i]));
  }
}

/* assume t>0 and l>1 */
static void
dogroup(subgp_iter *T)
{
  const long *powlist = T->powlist;
  long *M = T->M;
  long *L = T->L;
  long *c = T->c;
  long  *a = T->a,  *maxa = T->maxa;
  long **g = T->g, **maxg = T->maxg;
  GEN **H = T->H;
  pari_sp av = avma;
  long e,i,j,k,r,n,t2,ind, t = len(M), l = len(L);

  t2 = (l==t)? t-1: t;
  n = t2 * l - (t2*(t2+1))/2; /* number of gamma_ij */
  for (i=1, r=t+1; ; i++)
  {
    if (T->available[i]) c[r++] = i;
    if (r > l) break;
  }
  if (DEBUGLEVEL>2) { fprintferr("    column selection:"); printtyp(c); }
  /* a/g and maxa/maxg access the same data indexed differently */
  for (ind=0,i=1; i<=t; ind+=(l-i),i++)
  {
    maxg[i] = maxa + (ind - (i+1)); /* only access maxg[i][i+1..l] */
    g[i] = a + (ind - (i+1));
    for (r=i+1; r<=l; r++)
      if (c[r] < c[i])         maxg[i][r] = powlist[M[i]-M[r]-1];
      else if (L[c[r]] < M[i]) maxg[i][r] = powlist[L[c[r]]-M[r]];
      else                     maxg[i][r] = powlist[M[i]-M[r]];
  }
  av = avma; a[n-1]=0; for (i=0; i<n-1; i++) a[i]=1;
  for(;;)
  {
    a[n-1]++;
    if (a[n-1] > maxa[n-1])
    {
      j=n-2; while (j>=0 && a[j]==maxa[j]) j--;
      if (j < 0) { avma = av; return; }

      a[j]++; for (k=j+1; k<n; k++) a[k]=1;
    }
    for (i=1; i<=t; i++)
    {
      for (r=1; r<i; r++) affsi(0, H[i][c[r]]);
      affsi(powlist[L[c[r]] - M[r]], H[r][c[r]]);
      for (r=i+1; r<=l; r++)
      {
        if (c[r] < c[i])
          e = g[i][r] * powlist[L[c[r]] - M[i]+1];
        else
          if (L[c[r]] < M[i]) e = g[i][r];
          else e = g[i][r] * powlist[L[c[r]] - M[i]];
        affsi(e, H[i][c[r]]);
      }
    }
    treatsub(T, (GEN)H); avma = av;
  }
}

/* T->c[1],...,T->c[r-1] filled */
static void
loop(subgp_iter *T, long r)
{
  long j;

  if (r > len(T->M)) { dogroup(T); return; }

  if (r!=1 && (T->M[r-1] == T->M[r])) j = T->c[r-1]+1; else j = 1;
  for (  ; j<=T->maxc[r]; j++)
    if (T->available[j])
    {
      T->c[r] = j;  T->available[j] = 0;
      loop(T, r+1); T->available[j] = 1;
    }
}

static void
dopsubtyp(subgp_iter *T)
{
  pari_sp av = avma;
  long i,r, l = len(T->L), t = len(T->M);

  if (!t) { treatsub(T, mkmat( zerocol(l) )); avma = av; return; }
  if (l==1) /* imply t = 1 */
  {
    GEN p1 = gtomat(stoi(T->powlist[T->L[1]-T->M[1]]));
    treatsub(T, p1); avma = av; return;
  }
  T->c         = new_chunk(l+1); setlen(T->c, l);
  T->maxc      = new_chunk(l+1);
  T->available = new_chunk(l+1);
  T->a   = new_chunk(l*(t+1));
  T->maxa= new_chunk(l*(t+1));
  T->g    = (long**)new_chunk(t+1);
  T->maxg = (long**)new_chunk(t+1);

  if (DEBUGLEVEL) { fprintferr("  subgroup:"); printtyp(T->M); }
  for (i=1; i<=t; i++)
  {
    for (r=1; r<=l; r++)
      if (T->M[i] > T->L[r]) break;
    T->maxc[i] = r-1;
  }
  T->H = (GEN**)cgetg(t+1, t_MAT);
  for (i=1; i<=t; i++)
  {
    T->H[i] = (GEN*)cgetg(l+1, t_COL);
    for (r=1; r<=l; r++) T->H[i][r] = cgeti(3);
  }
  for (i=1; i<=l; i++) T->available[i]=1;
  for (i=1; i<=t; i++) T->c[i]=0;
  /* go through all column selections */
  loop(T, 1); avma = av; return;
}

static long
weight(long *typ)
{
  long i, l = len(typ), w = 0;
  for (i=1; i<=l; i++) w += typ[i];
  return w;
}

static void
dopsub(subgp_iter *T, GEN p, GEN indexsubq)
{
  long *M, *L = T->L;
  long w,i,j,k,lsubq, wG = weight(L), wmin = 0, wmax = wG, n = len(L);

  if (DEBUGLEVEL) { fprintferr("\ngroup:"); printtyp(L); }
  T->count = 0;
  switch(T->boundtype)
  {
  case b_MAX: /* upper bound */
    wmin = (long) (wG - (log(gtodouble(T->bound)) / log(gtodouble(p))));
    if (cmpii(powiu(p, wG - wmin), T->bound) > 0) wmin++;
    break;
  case b_EXACT: /* exact value */
    wmin = wmax = wG - Z_pval(T->bound, p);
    break;
  }
  T->M = M = new_chunk(n+1);
  if (T->subq)
  {
    lsubq = lg(T->subq);
    T->subqpart = T->bound? cgetg(lsubq, t_VEC): T->subq;
  }
  else
    lsubq = 0; /* -Wall */
  M[1] = -1; for (i=2; i<=n; i++) M[i]=0;
  for(;;) /* go through all vectors mu_{i+1} <= mu_i <= lam_i */
  {
    M[1]++;
    if (M[1] > L[1])
    {
      for (j=2; j<=n; j++)
        if (M[j] < L[j] && M[j] < M[j-1]) break;
      if (j > n) return;

      M[j]++; for (k=1; k<j; k++) M[k]=M[j];
    }
    for (j=1; j<=n; j++)
      if (!M[j]) break;
    setlen(M, j-1); w = weight(M);
    if (w >= wmin && w <= wmax)
    {
      GEN p1 = gen_1;

      if (T->subq && T->bound) /* G not a p-group */
      {
        pari_sp av = avma;
        GEN indexH = powiu(p, wG - w);
        GEN B = divii(T->bound, indexH);
        k = 1;
        for (i=1; i<lsubq; i++)
          if (cmpii(gel(indexsubq,i), B) <= 0)
            T->subqpart[k++] = T->subq[i];
        setlg(T->subqpart, k); avma = av;
      }
      if (DEBUGLEVEL)
      {
        long *Lp = conjugate(L);
        long *Mp = conjugate(M);
        GEN BINMAT = matqpascal(len(L)+1, p);

        if (DEBUGLEVEL > 3)
        {
          fprintferr("    lambda = "); printtyp(L);
          fprintferr("    lambda'= "); printtyp(Lp);
          fprintferr("    mu = "); printtyp(M);
          fprintferr("    mu'= "); printtyp(Mp);
        }
        for (j=1; j<=len(Mp); j++)
        {
          p1 = mulii(p1, powiu(p, Mp[j+1]*(Lp[j]-Mp[j])));
          p1 = mulii(p1, gcoeff(BINMAT, Lp[j]-Mp[j+1]+1, Mp[j]-Mp[j+1]+1));
        }
        fprintferr("  alpha_lambda(mu,p) = %Z\n",p1);
      }

      T->countsub = 0; dopsubtyp(T);
      T->count += T->countsub;

      if (DEBUGLEVEL)
      {
        fprintferr("  countsub = %ld\n", T->countsub);
        msgtimer("for this type");
        if (T->fun != list_fun || !((sublist_t*)(T->fundata))->gen)
        {
          if (T->subq) p1 = mulis(p1,lg(T->subqpart)-1);
          if (!equaliu(p1,T->countsub))
          {
            fprintferr("  alpha = %Z\n",p1);
            pari_err(bugparier,"forsubgroup (alpha != countsub)");
          }
        }
      }
    }
  }
}

static void
parse_bound(subgp_iter *T)
{
  GEN b, B = T->bound;
  if (!B) { T->boundtype = b_NONE; return; }
  switch(typ(B))
  {
  case t_INT: /* upper bound */
    T->boundtype = b_MAX;
    break;
  case t_VEC: /* exact value */
    b = gel(B,1);
    if (lg(B) != 2 || typ(b) != t_INT) pari_err(typeer,"subgroup");
    T->boundtype = b_EXACT;
    T->bound = b;
    break;
  case t_COL: /* exact type */
    pari_err(impl,"exact type in subgrouplist");
    if (lg(B) > len(T->L)+1) pari_err(typeer,"subgroup");
    T->boundtype = b_TYPE;
    break;
  default: pari_err(typeer,"subgroup");
  }
  if (signe(T->bound) <= 0)
    pari_err(talker,"subgroup: index bound must be positive");
}

static GEN
expand_sub(GEN x, long n)
{
  long i,j, m = lg(x);
  GEN p = matid(n-1), q,c;

  for (i=1; i<m; i++)
  {
    q = gel(p,i); c = gel(x,i);
    for (j=1; j<m; j++) q[j] = c[j];
    for (   ; j<n; j++) gel(q,j) = gen_0;
  }
  return p;
}

static GEN
init_powlist(long k, long p)
{
  GEN z = new_chunk(k+1);
  long i;
  z[0] = 1; z[1] = p;
  for (i=1; i<=k; i++)
  {
    GEN t = muluu(p, z[i-1]);
    z[i] = itos(t);
  }
  return z;
}

static GEN subgrouplist_i(GEN cyc, GEN bound, GEN expoI, GEN gen);
static void
subgroup_engine(subgp_iter *T)
{
  pari_sp av = avma;
  GEN B,L,fa,primlist,p,listL,indexsubq = NULL;
  GEN cyc = T->cyc;
  long i,j,k,imax,nbprim, n = lg(cyc);

  if (typ(cyc) != t_VEC)
  {
    if (typ(cyc) != t_MAT) pari_err(typeer,"forsubgroup");
    cyc = mattodiagonal_i(cyc);
  }
  for (i=1; i<n-1; i++)
    if (!dvdii(gel(cyc,i), gel(cyc,i+1)))
      pari_err(talker,"not a group in forsubgroup");
  if (n == 1) {
    parse_bound(T);
    switch(T->boundtype)
    {
      case b_EXACT: if (!is_pm1(T->bound)) break;
      default: T->fun(T, cyc);
    }
    avma = av; return;
  }
  if (!signe(cyc[1])) pari_err(talker,"infinite group in forsubgroup");
  if (DEBUGLEVEL) (void)timer2();
  fa = factor(gel(cyc,1)); primlist = gel(fa,1);
  nbprim = lg(primlist);
  listL = new_chunk(n); imax = k = 0;
  for (i=1; i<nbprim; i++)
  {
    L = new_chunk(n); p = gel(primlist,i);
    for (j=1; j<n; j++)
    {
      L[j] = Z_pval(gel(cyc,j), p);
      if (!L[j]) break;
    }
    j--; setlen(L, j);
    if (j > k) { k = j; imax = i; }
    gel(listL,i) = L;
  }
  L = gel(listL,imax); p = gel(primlist,imax);
  k = L[1];
  T->L = L;
  T->powlist = init_powlist(k, itos(p));
  B = T->bound;
  parse_bound(T);

  if (nbprim == 2)
  {
    T->subq = NULL;
    if (T->boundtype == b_EXACT)
    {
      (void)Z_pvalrem(T->bound,p,&B);
      if (!gcmp1(B)) { avma = av; return; }
    }
  }
  else
  { /* not a p-group */
    GEN cycI = shallowcopy(cyc);
    long lsubq;
    for (i=1; i<n; i++)
    {
      gel(cycI,i) = divis(gel(cycI,i), T->powlist[L[i]]);
      if (gcmp1(gel(cycI,i))) break;
    }
    setlg(cycI, i); /* cyclic factors of I */
    if (T->boundtype == b_EXACT)
    {
      (void)Z_pvalrem(T->bound,p,&B);
      B = mkvec(B);
    }
    T->expoI = gel(cycI,1);
    T->subq = subgrouplist_i(cycI, B, T->expoI, NULL);

    lsubq = lg(T->subq);
    for (i=1; i<lsubq; i++)
      gel(T->subq,i) = expand_sub(gel(T->subq,i), n);
    if (T->bound)
    {
      indexsubq = cgetg(lsubq,t_VEC);
      for (i=1; i<lsubq; i++)
        gel(indexsubq,i) = dethnf_i(gel(T->subq,i));
    }
    /* lift subgroups of I to G */
    for (i=1; i<lsubq; i++)
      gel(T->subq,i) = gmulsg(T->powlist[k],gel(T->subq,i));
    if (DEBUGLEVEL>2)
    {
      fprintferr("(lifted) subgp of prime to %Z part:\n",p);
      outbeaut(T->subq);
    }
  }
  dopsub(T, p,indexsubq);
  if (DEBUGLEVEL) fprintferr("nb subgroup = %ld\n",T->count);
  avma = av;
}

static GEN
get_snf(GEN x, long *N)
{
  GEN cyc;
  long n;
  switch(typ(x))
  {
    case t_MAT:
      if (!isdiagonal(x)) return NULL;
      cyc = mattodiagonal_i(x); break;
    case t_VEC: if (lg(x) == 4 && typ(x[2]) == t_VEC) x = gel(x,2);
    case t_COL: cyc = shallowcopy(x); break;
    default: return NULL;
  }
  *N = lg(cyc)-1;
  for (n = *N; n > 0; n--) /* take care of trailing 1s */
  {
    GEN c = gel(cyc,n);
    if (typ(c) != t_INT) return NULL;
    if (!gcmp1(c)) break;
  }
  setlg(cyc, n+1);
  for ( ; n > 0; n--)
  {
    GEN c = gel(cyc,n);
    if (typ(c) != t_INT) return NULL;
  }
  return cyc;
}

void
forsubgroup(entree *ep, GEN cyc, GEN bound, char *ch)
{
  subgp_iter T;
  expr_t E;
  long N;

  T.fun = &std_fun;
  cyc = get_snf(cyc,&N);
  if (!cyc) pari_err(typeer,"forsubgroup");
  T.bound = bound;
  T.cyc = cyc;
  E.s = ch;
  E.ep= ep; T.fundata = (void*)&E;
  push_val(ep, gen_0);

  subgroup_engine(&T);

  pop_val(ep);
}

static GEN
subgrouplist_i(GEN cyc, GEN bound, GEN expoI, GEN gen)
{
  pari_sp av = avma;
  subgp_iter T;
  sublist_t S;
  slist *list, *sublist;
  long ii,i,j,k,nbsub,n,N;
  GEN z,H;

  cyc = get_snf(cyc, &N);
  if (!cyc) pari_err(typeer,"subgrouplist");
  n = lg(cyc)-1; /* not necessarily = N */

  S.list = sublist = (slist*) gpmalloc(sizeof(slist));
  S.hnfgroup = diagonal_i(cyc);
  S.gen = gen;
  S.count = 0;
  T.fun = &list_fun;
  T.fundata = (void*)&S;

  T.cyc = cyc;
  T.bound = bound;
  T.expoI = expoI;

  subgroup_engine(&T);

  nbsub = (long)S.count;
  avma = av;
  z = cgetg(nbsub+1,t_VEC);
  for (ii=1; ii<=nbsub; ii++)
  {
    list = sublist; sublist = list->next; free(list);
    H = cgetg(N+1,t_MAT); gel(z,ii) = H; k=0;
    for (j=1; j<=n; j++)
    {
      gel(H,j) = cgetg(N+1, t_COL);
      for (i=1; i<=j; i++) gcoeff(H,i,j) = stoi(sublist->data[k++]);
      for (   ; i<=N; i++) gcoeff(H,i,j) = gen_0;
    }
    for (   ; j<=N; j++) gel(H,j) = col_ei(N, j);
  }
  free(sublist); return z;
}

GEN
subgrouplist(GEN cyc, GEN bound)
{
  return subgrouplist_i(cyc,bound,NULL,NULL);
}

GEN
subgroupcondlist(GEN cyc, GEN bound, GEN L)
{
  return subgrouplist_i(cyc,bound,NULL,L);
}