/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU GO, a Go program. Contact gnugo@gnu.org, or see   *
 * http://www.gnu.org/software/gnugo/ for more information.      *
 *                                                               *
 * Copyright 1999 and 2000 by the Free Software Foundation.      *
 *                                                               *
 * This program is free software; you can redistribute it and/or *
 * modify it under the terms of the GNU General Public License   *
 * as published by the Free Software Foundation - version 2.     *
 *                                                               *
 * This program is distributed in the hope that it will be       *
 * useful, but WITHOUT ANY WARRANTY; without even the implied    *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU General Public License in file COPYING  *
 * for more details.                                             *
 *                                                               *
 * You should have received a copy of the GNU General Public     *
 * License along with this program; if not, write to the Free    *
 * Software Foundation, Inc., 59 Temple Place - Suite 330,       *
 * Boston, MA 02111, USA                                         *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "liberty.h"

static void propagate_worm_recursive(int m, int n, 
				     int mw[MAX_BOARD][MAX_BOARD]);
static int genus(int i, int j);
static void markcomponent(int i, int j, int m, int n,
			  int mg[MAX_BOARD][MAX_BOARD]);
static void border_recurse(int i, int j, 
			   int mx[MAX_BOARD][MAX_BOARD], 
			   int *border_color, int *edge, int *oi, int *oj);
static void ping_cave(int i, int j, 
		      int *result1,  int *result2, int *result3, int *result4);
static int libertiesrec(int i, int j, int mx[MAX_BOARD][MAX_BOARD]);
static int touching(int i, int j, int color);
static void ping_recurse(int i, int j, int *counter, 
			 int mx[MAX_BOARD][MAX_BOARD], 
			 int mr[MAX_BOARD][MAX_BOARD], int color);


/* A STRING is a maximal connected set of stones of the same color, 
 * black or white. A WORM is the same thing as a string, except that
 * its color can be empty. An empty worm is called a CAVITY.
 *
 * Worms are eventually amalgamated into dragons. An empty dragon
 * is called a CAVE.
 */



/* make_worms() finds all worms and assembles some data about them.
 *
 * It returns 0 if the board is empty.
 *
 * Each worm is marked with an origin, having coordinates (origini, originj).
 * This is an arbitrarily chosen element of the worm, in practice the
 * algorithm puts the origin at the first element when they are given
 * the lexicographical order, though its location is irrelevant for
 * applications. To see if two stones lie in the same worm, compare
 * their origins.
 *
 * We will use the field dragon[m][n].genus to keep track of
 * black- or white-bordered cavities (essentially eyes) which are found.  
 * so this field must be zero'd now.
 */

int
make_worms(void)
{
  int m,n; /* iterate over board */
  int transferrable_distance_to_black[MAX_BOARD][MAX_BOARD];
  int transferrable_distance_to_white[MAX_BOARD][MAX_BOARD];
  int board_not_empty=0;

  /* Initialize distance_to_black[][], distance_to_white[][],
   * strategic_distance_to_black[][], strategic_distance_to_white[][].
   *
   * transferrable_distance_to_black[][], and 
   * transferrable_distance_to_white[][] are just temporary data. 
   */
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++) {
      dragon[m][n].genus=0;
      if (p[m][n] == BLACK) {
	distance_to_black[m][n] = 0;
	strategic_distance_to_black[m][n] = 0;
	transferrable_distance_to_black[m][n] = 0;
      } else {
	distance_to_black[m][n] = -1;
	strategic_distance_to_black[m][n] = -1;
	transferrable_distance_to_black[m][n] = -1;
      }
      if (p[m][n] == WHITE) {
	distance_to_white[m][n] = 0;
	strategic_distance_to_white[m][n] = 0;
	transferrable_distance_to_white[m][n] = 0;
      } else {
	distance_to_white[m][n] = -1;
	strategic_distance_to_white[m][n] = -1;
	transferrable_distance_to_white[m][n] = -1;
      }
      if (p[m][n]) 
	board_not_empty=1;
    }
  
  /* Set rest of distance_to_black[][] and distance_to_white[][]. */
  {
    int c=0;
    int found_one=1;
    while (found_one) {
      c++;
      found_one=0;
      for (m=0;m<board_size;m++)
	for (n=0;n<board_size;n++) {
	  /* If empty and not already done and neighbour to a 
	     finished intersection, set distance. */
	  if (p[m][n] == EMPTY && distance_to_black[m][n] == -1)
	    if ((m>0 && distance_to_black[m-1][n] == c-1) 
		|| (m<board_size-1 && distance_to_black[m+1][n] == c-1) 
		|| (n>0 && distance_to_black[m][n-1] == c-1) 
		|| (n<board_size-1 && distance_to_black[m][n+1] == c-1)) 
	    {
	      found_one=1;
	      distance_to_black[m][n]=c;
	    }

	  if (p[m][n] == EMPTY && distance_to_white[m][n] == -1)
	    if ((m>0 && distance_to_white[m-1][n] == c-1)
		|| (m<board_size-1 && distance_to_white[m+1][n] == c-1) 
		|| (n>0 && distance_to_white[m][n-1] == c-1) 
		|| (n<board_size-1 && distance_to_white[m][n+1] == c-1)) 
	    {
	      found_one=1;
	      distance_to_white[m][n]=c;
	    }
	}
    }
  }
  
  /* Set rest of strategic_distance_to_black[][] and 
   * strategic_distance_to_white[][], using
   * transferrable_distance_to_{black|white}[][] as temporary data. 
   */
  {
    int c=0;
    int found_one=1;
    while (found_one) {
      c++;
      found_one=0;
      for (m=0;m<board_size;m++)
	for (n=0;n<board_size;n++) {

	  if (p[m][n] == EMPTY && strategic_distance_to_black[m][n] == -1)
	    if ((m>0 && transferrable_distance_to_black[m-1][n] == c-1) 
		|| (m<board_size-1 &&
		    transferrable_distance_to_black[m+1][n] == c-1) 
		|| (n>0 && transferrable_distance_to_black[m][n-1] == c-1) 
		|| (n<board_size-1 &&
		    transferrable_distance_to_black[m][n+1] == c-1)) 
	    {
	      found_one=1;
	      strategic_distance_to_black[m][n]=c;
	    }

	  if (p[m][n] == EMPTY && transferrable_distance_to_black[m][n] == -1)
	    if (   (m>0
		    && transferrable_distance_to_black[m-1][n] == c-1 
		    && strategic_distance_to_white[m-1][n] != 1) 
		|| (m<board_size-1 
		    && transferrable_distance_to_black[m+1][n] == c-1
		    && strategic_distance_to_white[m+1][n] != 1) 
		|| (n>0
		    && transferrable_distance_to_black[m][n-1] == c-1
		    && strategic_distance_to_white[m][n-1] != 1) 
		|| (n<board_size-1 
		    && transferrable_distance_to_black[m][n+1] == c-1 
		    && strategic_distance_to_white[m][n+1] != 1)) 
	    {
	      found_one=1;
	      transferrable_distance_to_black[m][n]=c;
	    }

	  if (p[m][n] == EMPTY && strategic_distance_to_white[m][n] == -1)
	    if ((m>0 && transferrable_distance_to_white[m-1][n] == c-1) 
		|| (m<board_size-1 
		    && transferrable_distance_to_white[m+1][n] == c-1) 
		|| (n>0 && transferrable_distance_to_white[m][n-1] == c-1) 
		|| (n<board_size-1 
		    && transferrable_distance_to_white[m][n+1] == c-1)) 
	    {
	      found_one=1;
	      strategic_distance_to_white[m][n]=c;
	    }

	  if (p[m][n] == EMPTY && transferrable_distance_to_white[m][n] == -1)
	    if ((m>0 
		 && transferrable_distance_to_white[m-1][n] == c-1 
		 && strategic_distance_to_black[m-1][n] != 1) 
		|| (m<board_size-1 
		    && transferrable_distance_to_white[m+1][n] == c-1
		    && strategic_distance_to_black[m+1][n] != 1) 
		|| (n>0 
		    && transferrable_distance_to_white[m][n-1] == c-1
		    && strategic_distance_to_black[m][n-1] != 1) 
		|| (n<board_size-1 
		    && transferrable_distance_to_white[m][n+1] == c-1 
		    && strategic_distance_to_black[m][n+1] != 1)) 
	    {
	      found_one=1;
	      transferrable_distance_to_white[m][n]=c;
	    }
	}
    }
  }
  
  /* Initialize the worm data for each worm. */
  {
    int mw[MAX_BOARD][MAX_BOARD];
    memset(mw, 0, sizeof(mw));
    
    for (m=0;m<board_size;m++)
      for (n=0;n<board_size;n++)
	if (!mw[m][n]) {
	  worm[m][n].color=p[m][n];
	  worm[m][n].origini=m;
	  worm[m][n].originj=n;
	  worm[m][n].ko=0;
	  worm[m][n].inessential=0;
	  propagate_worm_recursive(m, n, mw);
	}
  } /* scope of mw */

  if (!board_not_empty)
    return (0);

  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++) {
      if ((worm[m][n].origini!=m) || (worm[m][n].originj!=n))
	continue;

      if (p[m][n] == EMPTY) {
	  worm[m][n].color=find_border(m, n, NULL);
	  worm[m][n].size=size;
	} else {
	  int ti,tj;
	  int acode, dcode;
	  TRACE ("considering attack and defense of %m\n", m, n);
	  worm[m][n].attacki=-1;
	  worm[m][n].defendi=-1;
	  worm[m][n].liberties=countlib(m, n, p[m][n]);
	  worm[m][n].size=size;
	  worm[m][n].attack_code=0;
	  worm[m][n].defend_code=0;
	  acode=attack(m, n, &ti, &tj);
	  if (acode) {
	    TRACE ("worm at %m can be attacked at %m\n", m,n,ti,tj);
	    worm[m][n].attacki = ti;
	    worm[m][n].attackj = tj;
	    worm[m][n].attack_code=acode;
	    dcode=find_defense(m, n, &ti, &tj);
	    if (dcode) {
	      TRACE ("worm at %m can be defended at %m\n", m,n,ti,tj);
	      worm[m][n].defendi = ti;
	      worm[m][n].defendj = tj;
	      worm[m][n].defend_code=dcode;
	    } else {
	      /* If the point of attack is not adjacent to the worm, 
	       * it is possible that this is an overlooked point of
	       * defense, so we try and see if it defends.
	       */
	      int ai = worm[m][n].attacki;
	      int aj = worm[m][n].attackj;
	      if (((ai == 0) 
		   || (worm[ai-1][aj].origini != m)
		   || (worm[ai-1][aj].originj != n))
		  &&
		  ((ai == board_size-1)
		   || (worm[ai+1][aj].origini != m)
		   || (worm[ai+1][aj].originj != n))
		  &&
		  ((aj == 0)
		   || (worm[ai][aj-1].origini != m)
		   || (worm[ai][aj-1].originj != n))
		  &&
		  ((aj == board_size-1)
		   || (worm[ai][aj+1].origini != m)
		   || (worm[ai][aj+1].originj != n)))
		if (trymove(ai, aj, worm[m][n].color, "make_worms", -1, -1)) {
		  acode=attack(m, n, NULL, NULL);
		  if (acode != 1) {
		    worm[m][n].defendi=ai;
		    worm[m][n].defendj=aj;
		    if (acode==0)
		      worm[m][n].defend_code=1;
		    else if (acode==2)
		      worm[m][n].defend_code=3;
		    else if (acode==3)
		      worm[m][n].defend_code=2;
		    TRACE ("worm at %m can be defended at %m\n", m,n,ai,aj);
		  }	 
		  popgo();
		}
	    }
	  }
	}
	propagate_worm(m, n);
      }
  
  /* Find kos. Check carefully that the purported ko move doesn't actually
   * capture more than one stone.
   */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      int captures;
      int capi, capj;

      if (p[m][n] != EMPTY
	  || (worm[m][n].size!=1)
	  || (worm[m][n].color == GRAY_BORDER)) 
	continue;

      captures=0;
      capi=-1;
      capj=-1;

      if ((m>0)
	  && (worm[m-1][n].size==1)
	  && (worm[m-1][n].liberties==1)) {
	captures++;
	capi=m-1;
	capj=n;
      }
      if ((m<board_size-1)
	  && (worm[m+1][n].size==1)
	  && (worm[m+1][n].liberties==1)) {
	captures++;
	capi=m+1;
	capj=n;
      }
      if ((n>0)
	  && (worm[m][n-1].size==1)
	  && (worm[m][n-1].liberties==1)) {
	captures++;
	capi=m;
	capj=n-1;
      }
      if ((n<board_size-1)
	  && (worm[m][n+1].size==1)
	  && (worm[m][n+1].liberties==1)) {
	captures++;
	capi=m;
	capj=n+1;
      }
      if (captures==1) {
	worm[m][n].ko=1;
	worm[capi][capj].ko=1;
      }
    }

/* Find adjacent worms that can be easily captured */

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) 
      if ((worm[m][n].origini==m) && (worm[m][n].originj==n) && p[m][n]) {
	int i,j;
	if (find_lunch(m, n, &i, &j, NULL, NULL)
	    && ((worm[i][j].attack_code==1) 
		|| (worm[i][j].attack_code==2))) {
	    TRACE("lunch found for %m at %m\n", m, n, i, j);
	    worm[m][n].lunchi=i;
	    worm[m][n].lunchj=j;
	}
	else {
	  worm[m][n].lunchi=-1;
	  worm[m][n].lunchj=-1;
	}
	propagate_worm(m, n);
      }

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if ((p[m][n]) && (worm[m][n].origini == m) && (worm[m][n].originj == n)) {
	int lib1, lib2, lib3, lib4;

	ping_cave(m, n, &lib1, &lib2, &lib3, &lib4);
	assert(worm[m][n].liberties==lib1);
	worm[m][n].liberties2=lib2;
	worm[m][n].liberties3=lib3;
	worm[m][n].liberties4=lib4;
	worm[m][n].cutstone=0;
	propagate_worm(m,n);
      }

/* 
A CUTTING STRING is one adjacent to two enemy strings,
which do not have a liberty in common, and which
have no other common adjacent friendly stone which
can be captured. The most common type of cutting string 
is in this situation.

OX
XO

Exception: in the above diagram if one of the O stones
can be captured, the other is only a POTENTIAL CUTTING STONE.

A POTENTIAL CUTTING STRING, is one adjacent to two other
strings which have a common liberty where one can actually
cut by safely playing on the liberty, or which have an
adjacent stone other than the one in queston which can
be both attacked and defended.

For example, O in:

OX
X.

is a potential cutting stone provided a move at the empty
intersection is safe for O.

For cutting strings we set worm[m][n].cutstone=2. For potential
cutting strings we set worm[m][n].cutstone=1. For other strings,
worm[m][n].cutstone=0.

*/

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {

      /* Only work on each worm once. This is easiest done if we only 
	 work with the origin of each worm. */
      if (p[m][n] && (worm[m][n].origini==m) && (worm[m][n].originj==n)) {

	/* Try to find two adjacent worms (ui,uj) and (ti,tj) 
	   of opposite colour from (m, n). */
	int ui=-1, uj=-1;
	int ti=-1, tj=-1;
	int i,j;

	for (i=0; i<board_size; i++)
	  for (j=0; j<board_size; j++) {

	    /* Work only with the opposite color from (m, n). */
	    if (p[i][j] != OTHER_COLOR(p[m][n])) 
	      continue;
	      
	    if ((i>0)
		&& (worm[i-1][j].origini==m)
		&& (worm[i-1][j].originj==n)) {

	      /* If the worm to the left of this worm is the (m, n) worm,
		 we have found it. */
	      ASSERT(p[i-1][j]==p[m][n], m, n);

	      /* If we have not already found a worm which meets the criteria,
		 store it into (ti, tj), otherwise store it into (ui, uj). */
	      if (ti == -1) {
		ti=worm[i][j].origini;
		tj=worm[i][j].originj;
	      } else if ((ti != worm[i][j].origini) 
			 || (tj != (worm[i][j].originj))) {
		ui=worm[i][j].origini;
		uj=worm[i][j].originj;
	      }
	    }
	    else if ((i<board_size-1)
		     && (worm[i+1][j].origini==m)
		     && (worm[i+1][j].originj==n)) {

	      /* If the worm to the right is (m, n), we have found it. */
	      ASSERT(p[i+1][j]==p[m][n], m, n);
	      if (ti == -1) {
		ti=worm[i][j].origini;
		tj=worm[i][j].originj;
	      } else if ((ti != worm[i][j].origini) || 
			 (tj != (worm[i][j].originj))) {
		ui=worm[i][j].origini;
		uj=worm[i][j].originj;
	      }
	    }
	    else if ((j>0) 
		     && (worm[i][j-1].origini==m)
		     && (worm[i][j-1].originj==n)) {

	      /* If the worm upwards is (m, n), we have found it. */
	      ASSERT(p[i][j-1]==p[m][n], m, n);
	      if (ti == -1) {
		ti=worm[i][j].origini;
		tj=worm[i][j].originj;
	      } else if ((ti != worm[i][j].origini) || 
			 (tj != (worm[i][j].originj))) {
		ui=worm[i][j].origini;
		uj=worm[i][j].originj;
	      }
	    }
	    else if ((j<board_size-1)
		     && (worm[i][j+1].origini==m)
		     && (worm[i][j+1].originj==n)) {

	      /* If the worm downwards is (m, n), we have found it. */
	      ASSERT(p[i][j+1]==p[m][n], m, n);
	      if (ti == -1) {
		ti=worm[i][j].origini;
		tj=worm[i][j].originj;
	      } else if ((ti != worm[i][j].origini) || 
			 (tj != (worm[i][j].originj))) {
		ui=worm[i][j].origini;
		uj=worm[i][j].originj;
	      }
	    }
	  } /* loop over i,j */

/* 
 *  We are verifying the definition of cutting stones. We have verified that
 *  the string at (m,n) is adjacent to two enemy strings at (ti,tj) and
 *  (ui,uj). We need to know if these strings share a liberty.
 */

	/* Only do this if we really found anything. */
	if (ui != -1) {
	  int vi,vj;  /* look for a common liberty vi,vj */

	  worm[m][n].cutstone=2;
	  for (vi=0; vi<board_size-1; vi++)
	    for (vj=0; vj<board_size-1; vj++) {

	      if ((((vi>0) 
		    && (worm[vi-1][vj].origini==ti) 
		    && (worm[vi-1][vj].originj==tj)) 
		   || ((vi<board_size-1) 
		       && (worm[vi+1][vj].origini==ti)
		       && (worm[vi+1][vj].originj==tj)) 
		   || ((vj>0) 
		       && (worm[vi][vj-1].origini==ti)
		       && (worm[vi][vj-1].originj==tj)) 
		   || ((vj<board_size-1)
		       && (worm[vi][vj+1].origini==ti)
		       && (worm[vi][vj+1].originj==tj)))
		  &&
		  (((vi>0) 
		    && (worm[vi-1][vj].origini==ui) 
		    && (worm[vi-1][vj].originj==uj)) 
		   || ((vi<board_size-1) 
		       && (worm[vi+1][vj].origini==ui) 
		       && (worm[vi+1][vj].originj==uj)) 
		   || ((vj>0) 
		       && (worm[vi][vj-1].origini==ui) 
		       && (worm[vi][vj-1].originj==uj)) 
		   || ((vj<board_size-1) 
		       && (worm[vi][vj+1].origini==ui)
		       && (worm[vi][vj+1].originj==uj)))) {
		if (((p[vi][vj]==EMPTY)
		     && !safe_move(vi, vj, p[m][n]))
		    || ((p[vi][vj]==p[m][n])
			&& (worm[m][n].attacki != -1)
			&& (worm[m][n].defendi == -1)))
		  worm[m][n].cutstone=0;
		else if (((p[vi][vj]==EMPTY)
			  && safe_move(vi, vj, p[m][n]))
			 || ((p[vi][vj]==p[m][n])
			     && (worm[m][n].attacki != -1)
			     && (worm[m][n].defendi != -1)))
		  if (worm[m][n].cutstone>0)
		    worm[m][n].cutstone=1;
	      }
	    }
	} /* ui != -1, ie we found two adjacent strings */

      } /* if (m,n) is origin of a string */

    } /* loop over m,n */

  
  /* Set the value of all worms.  Worms with cutting stones are worth more. */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if (p[m][n] && (worm[m][n].origini==m) && (worm[m][n].originj==n)) {
	if (worm[m][n].cutstone == 2)
	  worm[m][n].value=83+4*worm[m][n].size;
	else if (worm[m][n].cutstone == 1)
	  worm[m][n].value=76+4*worm[m][n].size;
	else
	  worm[m][n].value=70+4*worm[m][n].size;
	worm[m][n].genus=genus(m, n);
	propagate_worm(m, n);
      }
    }

/* We try first to resolve small semeais. */

  small_semeai();

/* Now we try to improve the values of worm.attack and worm.defend. If we
 * find that capturing the string at (m,n) also defends the string at (i,j),
 * or attacks it, then we move the point of attack and defense.
 * We don't move the attacking point of strings that can't be defended.
 */
  {
    int mi[MAX_BOARD][MAX_BOARD]; /* mark changed information */
    int mx[MAX_BOARD][MAX_BOARD]; /* mark tried moves */
    int i, j;

    memset(mi, 0, sizeof(mi));
    memset(mx, 0, sizeof(mi));
    
    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++) {

	/* For each worm, only work with the origin. */
	if (p[m][n]
	    && (worm[m][n].origini == m)
	    && (worm[m][n].originj == n)) {
	  int color = p[m][n];
	  int other = OTHER_COLOR(color);

	  int ai = worm[m][n].attacki;
	  int aj = worm[m][n].attackj;
	  int di = worm[m][n].defendi;
	  int dj = worm[m][n].defendj;
	  
	  /* If the opponent has an attack on the worm (m, n), and we
	   * have not tried this move before, carry it out and see
	   * what it leads to.
	   */
	  if ((ai != -1) && (mx[ai][aj] == 0)) {

	    mx[ai][aj] = 1;
	    /* First, carry out the attacking move. */
	    if (trymove(ai, aj, other, "make_worms", -1, -1)) {

	      /* We must read to the same depth that was used in the
	       * initial determination of worm.attack and worm.defend
	       * to avoid horizon effect. Since stackp has been
	       * incremented we must also increment depth and
	       * backfill_depth. */
	      
	      /* Now we try to find a group which is saved or attacked as well
		 by this move. */
	      TRACE("trying %m\n", ai, aj);
	      depth++;
	      backfill_depth++;
	      fourlib_depth++;
	      ko_depth++;

	      for (i=0; i< board_size; i++)
		for (j=0; j < board_size; j++) {

		  /* If a worm has its origin (i, j), and it's not (m, n)...*/
		  if (p[i][j]
		      && (worm[i][j].origini == i) 
		      && (worm[i][j].originj == j) 
		      && ((i != m) || (j != n))) {

		    /* Either the worm is of the same color as (m, n),
		       then we try to attack it.  If there was a previous 
		       attack and defense of it, and there is no defence
		       for the attack now... */
		    if ((worm[i][j].color == color) 
			&& (worm[i][j].attacki != -1)
			&& (worm[i][j].defendi != -1)
			&& !find_defense(i, j, NULL, NULL)) {

		      int attack_works = 1;
		      /* Sometimes find_defense() fails to find a
                         defense which has been found by other means.
                         Try if the old defense move still works. */
		      if (trymove(worm[i][j].defendi, worm[i][j].defendj,
				  color, "make_worms", -1, -1)) {
			if (!attack(i, j, NULL, NULL))
			  attack_works = 0;
			popgo();
		      }
		      
		      /* ...then move the attack point of that worm to
			 the attack point of (m, n). */
		      if (attack_works) {
			TRACE("moving point of attack of %m to %m\n",
			      i, j, ai, aj);
			worm[i][j].attacki = ai;
			worm[i][j].attackj = aj;
			mi[i][j] = 1;
		      }
		    }
		    /* Or the worm is of the opposite color as (m, n).
		       If there was a previous defence for it, and there is
		       still no attack to kill it, then move the defence
		       point of (i, j) to the defence point of (m, n). */
		    else if ((worm[i][j].color == other) 
			     && (worm[i][j].defendi != -1) 
			     && !attack(i, j, NULL, NULL)) {

		      TRACE("moving point of defense of %m to %m\n",
			    i, j, ai, aj);
		      worm[i][j].defendi = ai;
		      worm[i][j].defendj = aj;
		      mi[i][j] = 1;
		    }
		  }
		}
	      popgo();
	      depth--;
	      backfill_depth--;
	      fourlib_depth--;
	      ko_depth--;
	    }
	  }

	  /* If there is a defense point for the worm (m, n), and we
	   * have not tried this move before, move there and see what
	   * it leads to.
	   */
	  if ((di != -1) && (mx[di][dj] == 0)) {

	    mx[di][dj] = 1;
	    /* First carry out the defending move. */
	    if (trymove(di, dj, color, "make_worms", -1, -1)) {
	      
	      TRACE("trying %m\n", di, dj);
	      ko_depth++;
	      fourlib_depth++;
	      depth++;
	      backfill_depth++;

	      for (i=0; i< board_size; i++)
		for (j=0; j < board_size; j++) {

		  /* If a worm has its origin (i, j), and it's not (m, n)...*/
		  if (p[i][j]
		      && (worm[i][j].origini == i) 
		      && (worm[i][j].originj == j) 
		      && ((i != m) || (j != n))) {

		    /* Either the worm is of the opposite color as (m, n),
		       then we try to attack it.  If there was a previous 
		       attack and defense of it, and there is no defence
		       for the attack now... */
		    if ((worm[i][j].color == other) 
			&& (worm[i][j].attacki != -1) 
			&& (worm[i][j].defendi != -1)
			&& !find_defense(i, j, NULL, NULL)) {

		      int attack_works = 1;
		      /* Sometimes find_defense() fails to find a
                         defense which has been found by other means.
                         Try if the old defense move still works. */
		      if (trymove(worm[i][j].defendi, worm[i][j].defendj,
				  other, "make_worms", -1, -1)) {
			if (!attack(i, j, NULL, NULL))
			  attack_works = 0;
			popgo();
		      }
		      
		      /* ...then move the attack point of that worm to
			 the defense point of (m, n). */
		      if (attack_works) {
			TRACE("moving point of attack of %m to %m\n",
			      i, j, di, dj);
			worm[i][j].attacki = di;
			worm[i][j].attackj = dj;
			mi[i][j] = 1;
		      }
		    }
		    /* Or the worm is of the same color as (m, n).
		       If there was a previous defence for it, and there is
		       still no attack to kill it, then move the defence
		       point of (i, j) to the defence point of (m, n). */
		    else if ((worm[i][j].color == color)
			     && (worm[i][j].defendi != -1) 
			     && !attack(i, j, NULL, NULL)) {
		      TRACE("moving point of defense of %m to %m\n",
			    i, j, di, dj);
		      worm[i][j].defendi = di;
		      worm[i][j].defendj = dj;
		      mi[i][j] = 1;
		    }
		  }
		}
	      popgo();
	      ko_depth--;
	      fourlib_depth--;
	      depth--;
	      backfill_depth--;
	    }
	  }
	}
      }

    /* Propagate the newly generated info to all other stones of each worm. */
    for (i=0; i<board_size; i++)
      for (j=0; j<board_size; j++)
	if (mi[i][j])
	  propagate_worm(i, j);
  }

  /* Sometimes it happens that the tactical reading finds adjacent
   * strings which both can be attacked but not defended. (The reason
   * seems to be that the attacker tries harder to attack a string,
   * than the defender tries to capture it's neighbors.) When this
   * happens, the eyes code produces overlapping eye spaces and still
   * worse all the nondefendable stones actually get amalgamated with
   * their allies on the outside.
   *
   * To solve this we scan through the strings which can't be defended
   * and check whether they have a neighbor that can be attacked. In
   * this case we set the defense point of the former string to the
   * attacking point of the latter.
   *
   * Please notice that find_defense() will still read this out
   * incorrectly, which may lead to some confusion later.
   */

  /* First look for vertical neighbors. */
  for (m=0; m<board_size-1; m++)
    for (n=0; n<board_size; n++)
      if ((worm[m][n].origini != worm[m+1][n].origini
	   || worm[m][n].originj != worm[m+1][n].originj)
	  && p[m][n]!=EMPTY && p[m+1][n]!=EMPTY) {
        if (worm[m][n].attacki != -1 && worm[m+1][n].attacki != -1) {
	  if ((worm[m][n].defendi == -1)
	      && (does_defend(worm[m+1][n].attacki,
			      worm[m+1][n].attackj, m, n))) {
	    
	    change_defense(m, n, worm[m+1][n].attacki, worm[m+1][n].attackj);
	  }
	  if ((worm[m+1][n].defendi == -1)
              && (does_defend(worm[m][n].attacki,
			      worm[m][n].attackj, m+1, n))) {
	    
	    change_defense(m+1, n, worm[m][n].attacki, worm[m][n].attackj);
	  }
        }
      }
  
  /* Then look for horizontal neighbors. */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size-1; n++)
      if ((worm[m][n].origini != worm[m][n+1].origini ||
	   worm[m][n].originj != worm[m][n+1].originj) &&
	  p[m][n]!=EMPTY && p[m][n+1]!=EMPTY) {
        if (worm[m][n].attacki != -1 && worm[m][n+1].attacki != -1) {
	  if ((worm[m][n].defendi == -1)
              && (does_defend(worm[m][n+1].attacki,
			      worm[m][n+1].attackj, m, n))) {

	    change_defense(m, n, worm[m][n+1].attacki, worm[m][n+1].attackj);
	  }
	  if ((worm[m][n+1].defendi == -1)
              && (does_defend(worm[m][n].attacki,
			      worm[m][n].attackj, m, n+1))) {

	    change_defense(m, n+1, worm[m][n].attacki, worm[m][n].attackj);
	  }
	}
      }

  return 1;
}


/* 
 * propagate_worm() takes the worm data at one stone and copies it to 
 * the remaining members of the worm.  It uses propagate_worm_recursive()
 * to do the actual work.
 */

void 
propagate_worm(int m, int n)
{
  int mw[MAX_BOARD][MAX_BOARD];
  assert(stackp==0);

  memset(mw, 0, sizeof(mw));
  propagate_worm_recursive(m, n, mw);
}


/* Recursive worker function called by propagate_worms() and make_worms(). 
 *
 * Even though we don't need to copy all the fields, it's probably
 * better to do a structure copy which should compile to a block copy.
 */

static void 
propagate_worm_recursive(int m, int n, int mw[MAX_BOARD][MAX_BOARD])
{
  mw[m][n]=1;

  if (m>0 && (!mw[m-1][n]) && (p[m-1][n]==p[m][n])) {
    worm[m-1][n]=worm[m][n];
    propagate_worm_recursive(m-1, n, mw);
  }

  if ((m<board_size-1) && (!mw[m+1][n]) && (p[m+1][n]==p[m][n])) {
    worm[m+1][n]=worm[m][n];
    propagate_worm_recursive(m+1, n, mw);
  }

  if ((n>0) && (!mw[m][n-1]) && (p[m][n-1]==p[m][n])) {
    worm[m][n-1]=worm[m][n];
    propagate_worm_recursive(m, n-1, mw);
  }

  if ((n<board_size-1) && (!mw[m][n+1]) && (p[m][n+1]==p[m][n])) {
    worm[m][n+1]=worm[m][n];
    propagate_worm_recursive(m, n+1, mw);
  }
}


/* ping_cave(i, j, *result1, ...) is applied when (i, j) points to a string.
 * It computes the vector (*result1, *result2, *result3, *result4), 
 * where *result1 is the number of liberties of the string, 
 * *result2 is the number of second order liberties (empty vertices
 * at distance two) and so forth.
 *
 * The definition of liberties of order >1 is adapted to the problem
 * of detecting the shape of the surrounding cavity. In particular
 * we want to be able to see if a group is loosely surrounded.
 *
 * A liberty of order n is an empty space which may be connected
 * to the string by placing n stones of the same color on the board, 
 * but no fewer. The path of connection may pass through an intervening group
 * of the same color. The stones placed at distance >1 may not touch a
 * group of the opposite color.
 */

static void 
ping_cave(int i, int j, 
	  int *result1,  int *result2, int *result3, int *result4)
{
  int r, s=0, t=0, u=0;
  int m, n;
  int mrc[MAX_BOARD][MAX_BOARD];
  int mse[MAX_BOARD][MAX_BOARD];

  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++)
      mse[m][n]=0;

  r=libertiesrec(i, j, mse);

  s=0;
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++)
      mrc[m][n]=0;
  ping_recurse(i, j, &s, mse, mrc, p[i][j]);

  t=0;
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++)
      mrc[m][n]=0;
  ping_recurse(i, j, &t, mse, mrc, p[i][j]);

  u=0;
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++)
      mrc[m][n]=0;
  ping_recurse(i, j, &u, mse, mrc, p[i][j]);

  *result1=r;
  *result2=s;
  *result3=t;
  *result4=u;
}


static void 
ping_recurse(int i, int j, int *counter, 
	     int mx[MAX_BOARD][MAX_BOARD], 
	     int mr[MAX_BOARD][MAX_BOARD], 
	     int color)
{
  mr[i][j]=1;
  if ((i>0)&&(mx[i-1][j]==0)&&(p[i-1][j]==0)&&(mr[i-1][j]==0)&&
      (!touching(i-1, j, OTHER_COLOR(color)))) {
    (*counter)++;
    mr[i-1][j]=1;
    mx[i-1][j]=1;
  }
  if ((i<board_size-1)&&(mx[i+1][j]==0)&&(p[i+1][j]==0)&&(mr[i+1][j]==0)&&
      (!touching(i+1, j, OTHER_COLOR(color)))) {
    (*counter)++;
    mr[i+1][j]=1;
    mx[i+1][j]=1;
  }
  if ((j>0)&&(mx[i][j-1]==0)&&(p[i][j-1]==0)&&(mr[i][j-1]==0)&&
      (!touching(i, j-1, OTHER_COLOR(color)))) {
    (*counter)++;
    mr[i][j-1]=1;
    mx[i][j-1]=1;
  }
  if ((j<board_size-1)&&(mx[i][j+1]==0)&&(p[i][j+1]==0)&&(mr[i][j+1]==0)&&
      (!touching(i, j+1, OTHER_COLOR(color)))) {
    (*counter)++;
    mr[i][j+1]=1;
    mx[i][j+1]=1;
  }
  if (!worm[i][j].ko) {
    if ((i>0)&&(mr[i-1][j]==0)&&((mx[i-1][j]==1)||(p[i-1][j]==color)))
      ping_recurse(i-1, j, counter, mx, mr, color);
    if ((i<board_size-1)&&(mr[i+1][j]==0)&&((mx[i+1][j]==1)||(p[i+1][j]==color)))
      ping_recurse(i+1, j, counter, mx, mr, color);
    if ((j>0)&&(mr[i][j-1]==0)&&((mx[i][j-1]==1)||(p[i][j-1]==color)))
      ping_recurse(i, j-1, counter, mx, mr, color);
    if ((j<board_size-1)&&(mr[i][j+1]==0)&&((mx[i][j+1]==1)||(p[i][j+1]==color)))
      ping_recurse(i, j+1, counter, mx, mr, color);
  }
}


/* touching(i, j, color) returns true if the vertex at (i, j) is
 * touching any stone of (color).
 */

static int 
touching(int i, int j, int color)
{
  if (i>0 && p[i-1][j]==color)
    return 1;

  if (i<board_size-1 && p[i+1][j]==color)
    return 1;

  if (j>0 && p[i][j-1]==color)
    return 1;

  if (j<board_size-1 && p[i][j+1]==color)
    return 1;

  return 0;
}


/* (i, j) points to a worm.
 *
 * Return the number of liberties for the worm, and mark the locations
 * we have been in mx using a recursive algorithm.  
 */

static int 
libertiesrec(int i, int j, int mx[MAX_BOARD][MAX_BOARD])
{
  int  result = 0;

  size++;

  mx[i][j]=1;

  /* Is there a new liberty upwards? */
  if ((i>0) && (p[i-1][j]==0) && (!mx[i-1][j])) {
    result++;
    mx[i-1][j]=1;
  }

  /* Downwards? */
  if ((i<board_size-1) && (p[i+1][j]==0) && (!mx[i+1][j])) {
    result++;
    mx[i+1][j]=1;
  }

  /* To the left? */
  if ((j>0) && (p[i][j-1]==0) && (!mx[i][j-1])) {
    result++;
    mx[i][j-1]=1;
  }

  /* To the right? */
  if ((j<board_size-1) && (p[i][j+1]==0) && (!mx[i][j+1])) {
    result++;
    mx[i][j+1]=1;
  }

  /* Find out liberties for the stone upwards. */
  if ((i>0) && (p[i-1][j]==p[i][j]) && (!mx[i-1][j])) 
    result += libertiesrec(i-1, j, mx);	

  if ((i<board_size-1) && (p[i+1][j]==p[i][j]) && (!mx[i+1][j])) 
    result += libertiesrec(i+1, j, mx);	

  if ((j>0) && (p[i][j-1]==p[i][j]) && (!mx[i][j-1])) 
    result += libertiesrec(i, j-1, mx);	

  if ((j<board_size-1) && (p[i][j+1]==p[i][j]) && (!mx[i][j+1])) 
    result += libertiesrec(i, j+1, mx);	

  return result;
}


/* The GENUS of a string is the number of connected components of
 * its complement, minus one. It is an approximation to the number of
 * eyes of the string. If (i, j) points to the origin of a string,
 * genus(i, j) returns its genus.
 */

static int 
genus(int i, int j)
{
  int m, n;
  int mg[MAX_BOARD][MAX_BOARD];
  int gen=-1;

  memset(mg, 0, sizeof(mg));
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++) {
      if (p[m][n]==EMPTY
	  || (worm[m][n].origini)!=i
	  || (worm[m][n].originj)!=j)
      {
	if (!mg[m][n]) {
	  markcomponent(i, j, m, n, mg);
	  gen++;
	}
      }
    }

  return gen;
}

/* This recursive function marks the component at (m, n) of the complement
 *   of the string with origin (i, j)
 */

static void 
markcomponent(int i, int j, int m, int n, int mg[MAX_BOARD][MAX_BOARD])
{
  mg[m][n]=1;
  if ((m>0)&&(mg[m-1][n]==0))
    if ((p[m-1][n]==0)||((worm[m-1][n].origini)!=i)||((worm[m-1][n].originj)!=j))
      markcomponent(i, j, m-1, n, mg);
  if ((m<board_size-1)&&(mg[m+1][n]==0))
    if ((p[m+1][n]==0)||((worm[m+1][n].origini)!=i)||((worm[m+1][n].originj)!=j))
      markcomponent(i, j, m+1, n, mg);      
  if ((n>0)&&(mg[m][n-1]==0))
    if ((p[m][n-1]==0)||((worm[m][n-1].origini)!=i)||((worm[m][n-1].originj)!=j))
      markcomponent(i, j, m, n-1, mg);
  if ((n<board_size-1)&&(mg[m][n+1]==0))
    if ((p[m][n+1]==0)||((worm[m][n+1].origini)!=i)||((worm[m][n+1].originj)!=j))
      markcomponent(i, j, m, n+1, mg);
}


/* find_border(m, n, *edge), if (m, n) is EMPTY, examines the cavity
   at (m, n), determines its size and returns its bordercolor, 
   which can be BLACK_BORDER, WHITE_BORDER or GRAY_BORDER. The
   edge parameter is set to the number of edge vertices in the
   cavity.

   If (m, n) is nonempty, it returns the same result, imagining
   that the string at (m, n) is removed. The edge parameter is
   set to the number of vertices where the cavity meets the
   edge in a point outside the removed string.  

   oi and oj will be the origin of the first border string
   found.
*/

int 
find_border(int m, int n, int *edge)
{
  int i, j;
  int oi, oj;
  int border_color=EMPTY;
  int ml[MAX_BOARD][MAX_BOARD];
  
  ASSERT(m>=0 && m < board_size && n >= 0 && n < board_size, m, n);
  
  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++)
      ml[i][j] = 0;
  size=0;
  if (edge) *edge=0;
  border_recurse(m, n, ml, &border_color, edge, &oi, &oj);
  if (border_color==GRAY_BORDER) return GRAY_BORDER;  
  if (border_color==BLACK) return BLACK_BORDER;
  if (border_color==WHITE) return WHITE_BORDER;

  if (color_has_played)
    gprintf("find_border: bordercolor %d not recognized at %m. Is the board empty ?\n", border_color, 
	    m, n);

  return(GRAY_BORDER);
}



/* helper function for find_border.
 * border_color contains information so far : transitions allowed are
 *   EMPTY       -> BLACK/WHITE
 *   BLACK/WHITE -> GRAY_BORDER
 *
 * mx[i][j] is 1 if i, j has already been visited
 *
 * On (fully-unwound) exit
 *   *border_color should be BLACK, WHITE or GRAY_BORDER
 *   *edge is the count of edge pieces
 *   *oi, *oj is the origin of the worm
 *
 * *border_color should only be EMPTY if and only if the board is completely empty
 */

static void 
border_recurse(int i, int j, int mx[MAX_BOARD][MAX_BOARD], int *border_color, int *edge, int *oi, int *oj)
{

  DEBUG(DEBUG_BORDER, "+border_recurse %d, %d : *bc = %d\n", i, j, *border_color);

  ASSERT(mx[i][j] == 0, i, j);

  mx[i][j] = 1;
  size++;
  if ((edge) && ((i==0)||(i==board_size-1)||(j==0)||(j==board_size-1)) && (p[i][j] == EMPTY)) (*edge)++;
  if (i>0 && !mx[i-1][j]) {
    if ((p[i-1][j] != EMPTY) && (p[i][j] != p[i-1][j]))
      {
	/* we are empty, but neighbour is not ... */
	if ((*border_color)==EMPTY) {
	  /* first non-empty square : adopt its color */
	  (*oi)=worm[i-1][j].origini;
	  (*oj)=worm[i-1][j].originj;
	  (*border_color)=p[i-1][j];
	}
	else if ((*border_color)!=p[i-1][j])
	  (*border_color)=GRAY_BORDER;    /* mix of borders */
      }
    else if ( (p[i-1][j] == EMPTY) || (p[i-1][j]==p[i][j]) )
      /* they are same as us (either empty or string-to-be-removed) - recurse */
      border_recurse(i-1, j, mx, border_color, edge, oi, oj);
  }
  if ((i<board_size-1) && !mx[i+1][j]) {
    if ((p[i+1][j] != EMPTY) && (p[i][j] != p[i+1][j]))
      {
	if ((*border_color)==EMPTY) {
	  (*oi)=worm[i+1][j].origini;
	  (*oj)=worm[i+1][j].originj;
	  (*border_color)=p[i+1][j];
	}
	else if ((*border_color)!=p[i+1][j])
	  (*border_color)=GRAY_BORDER;  
      } 
    else if ( (p[i+1][j] == EMPTY) || (p[i+1][j]==p[i][j]) )
      border_recurse(i+1, j, mx, border_color, edge, oi, oj);
  } 
  if ((j>0) && !mx[i][j-1]) {
    if ((p[i][j-1] != EMPTY) && (p[i][j] != p[i][j-1]))
      {
	if ((*border_color)==EMPTY) {
	  (*oi)=worm[i][j-1].origini;
	  (*oj)=worm[i][j-1].originj;
	  (*border_color)=p[i][j-1];
	}
	else if ((*border_color)!=p[i][j-1])
	  (*border_color)=GRAY_BORDER;  
      } 
    else if ((p[i][j-1] == EMPTY) || (p[i][j-1]==p[i][j]))
      border_recurse(i, j-1, mx, border_color, edge, oi, oj);
  }
  if ((j<board_size-1) && !mx[i][j+1]) {
    if ((p[i][j+1] != EMPTY) && (p[i][j] != p[i][j+1]))
      {
	if ((*border_color)==EMPTY) {
	  (*oi)=worm[i][j+1].origini;
	  (*oj)=worm[i][j+1].originj;
	  (*border_color)=p[i][j+1];
	}
	else if ((*border_color)!=p[i][j+1])
	  (*border_color)=GRAY_BORDER;  
      } 
    else if ( (p[i][j+1] == EMPTY) || (p[i][j+1]==p[i][j]) )
      border_recurse(i, j+1, mx, border_color, edge, oi, oj);
  }
  DEBUG(DEBUG_BORDER,"-border_recurse %d, %d : *bc = %d\n", i, j, *border_color);

}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
