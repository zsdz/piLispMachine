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
#include "liberty.h"

static int semeai_analyzer(int m, int n, int i, int j,
			   int *ti, int *tj, int color);


/* semeai() searches for pairs of adjacent dead worms. If such a pair
 * is found, the number of liberties is compared, counting mutual
 * liberties only for strings of genus one. If the number of 
 * liberties is equal, it looks for an external liberty of the
 * opponent's string to fill. The parameter (ai, aj) points to
 * the origins of a pair of adjacent dead worms already evaluated,
 * to avoid redundancy.
 *
 * FIXME:
 *
 * The present implementation of the semeai code sometimes 
 * returns wrong answers. In reality, semeai can only be 
 * analyzed through reading. It will be awhile before GNU
 * Go is able to do this accurately. In the meantime, a
 * good idea would be to return a lower weight (say 60) 
 * if the situation differs from criticality by just
 * one liberty and the opponent has an outside liberty
 * which can be filled.
 */

int 
semeai (int *i, int *j, int *val, int *equal_moves, int color)
{
  int m, n, ti, tj, tval;
  int ai=-1, aj=-1, bi=-1, bj=-1;
  int found_one=0;
  int other=OTHER_COLOR(color);

  TRACE("Semeai Player is THINKING for %s!\n", 
	color==WHITE ? "white" : "black");

  for (m=0;m<board_size;m++) 
    for (n=0;n<board_size;n++) {
      if ((p[m][n]!=color) || (dragon[m][n].status != DEAD))
	continue;

      /* Check to the left. */
      if ((m>0)
	  && (p[m-1][n]==other)
	  && (dragon[m-1][n].vitality <0))
	if ((worm[m][n].origini != ai)
	    || (worm[m][n].originj != aj)
	    || (worm[m-1][n].origini != bi)
	    || (worm[m-1][n].originj != bj)) 
	{
	  ai=worm[m][n].origini;
	  aj=worm[m][n].originj;
	  bi=worm[m-1][n].origini;
	  bj=worm[m-1][n].originj;
	  tval=(semeai_analyzer(ai, aj, bi, bj, &ti, &tj, color));
	  if (tval>0) {
	    TRACE("semeai at %m, %m---%m recommended (%d)\n",
		  ai, aj, bi, bj, ti, tj, tval);
	    move_considered(ti,tj,tval);
	    if (BETTER_MOVE(tval, *val, *equal_moves)) {
	      *i=ti;
	      *j=tj;
	      found_one=1;
	    }
	  }
	  else
	    DEBUG(DEBUG_SEMEAI, "semeai analyzer does not like the look of it\n");
	}

      /* Check to the right. */
      if ((m<board_size-1)
	  && (p[m+1][n]==other)
	  && (dragon[m+1][n].vitality <0))
	if ((worm[m][n].origini != ai)
	    || (worm[m][n].originj != aj)
	    || (worm[m+1][n].origini != bi)
	    || (worm[m+1][n].originj != bj)) 
	{
	  ai=worm[m][n].origini;
	  aj=worm[m][n].originj;
	  bi=worm[m+1][n].origini;
	  bj=worm[m+1][n].originj;
	  tval=(semeai_analyzer(ai, aj, bi, bj, &ti, &tj, color));
	  if (tval>0) {
	    TRACE("semeai at %m, %m---%m recommended\n",
		  ai, aj, bi, bj, ti, tj);
	    move_considered(ti,tj,tval);
	    if (BETTER_MOVE(tval, *val, *equal_moves)) {
	      *i=ti;
	      *j=tj;
	      found_one=1;
	    }
	  }
	  else
	    DEBUG(DEBUG_SEMEAI, "semeai analyzer does not like the look of it\n");
	}

      /* Check to the top. */
      if ((n>0)
	  && (p[m][n-1]==other)
	  && (dragon[m][n-1].vitality <0))
	if ((worm[m][n].origini != ai)
	    || (worm[m][n].originj != aj)
	    || (worm[m][n-1].origini != bi)
	    || (worm[m][n-1].originj != bj)) 
	{
	  ai=worm[m][n].origini;
	  aj=worm[m][n].originj;
	  bi=worm[m][n-1].origini;
	  bj=worm[m][n-1].originj;
	  tval=(semeai_analyzer(ai, aj, bi, bj, &ti, &tj, color));
	  if (tval>0) {
	    TRACE("semeai at %m, %m---%m recommended\n",
		  ai, aj, bi, bj, ti, tj);
	    move_considered(ti,tj,tval);
	    if (BETTER_MOVE(tval, *val, *equal_moves)) {
	      *i=ti;
	      *j=tj;
	      found_one=1;
	    }
	  }
	  else
	    DEBUG(DEBUG_SEMEAI, "semeai analyzer does not like the look of it\n");
	}

      /* Check to the bottom. */
      if ((n<board_size-1)
	  && (p[m][n+1]==other)
	  && (dragon[m][n+1].vitality < 0))
	if ((worm[m][n].origini != ai)
	    || (worm[m][n].originj != aj)
	    || (worm[m][n+1].origini != bi)
	    || (worm[m][n+1].originj != bj)) 
	{
	  ai=worm[m][n].origini;
	  aj=worm[m][n].originj;
	  bi=worm[m][n+1].origini;
	  bj=worm[m][n+1].originj;
	  tval=(semeai_analyzer(ai, aj, bi, bj, &ti, &tj, color));
	  if (tval>0) {
	    TRACE("semeai at %m, %m---%m recommended\n",
		  ai, aj, bi, bj, ti, tj);
	    move_considered(ti,tj,tval);
	    if (BETTER_MOVE(tval, *val, *equal_moves)) {
	      *i=ti;
	      *j=tj;
	      found_one=1;
	    }
	  }
	  else
	    DEBUG(DEBUG_SEMEAI, "semeai analyzer does not like the look of it\n");
	}
    }

  return found_one;
}


/* It is assumed that (m,n) and (i,j) are the origins of adjoining
 * single-worm dragons. Returns 1 if move by (m,n) makes a win, 2 if a move by
 * (m,n) makes seki, 0 if no move by (m,n) has any effect. This function
 * recursively tries to read out the semeai, considering only one variation.  
 *
 * The recommended move is returned as (ti, tj).
 */

#if 0

int
new_semeai_analyzer(int m, int n, int i, int j, int *ti, int *tj)
{
  int mx[19][19], mo[19][19];
  int color=p[m][n];
  int other=p[i][j];

  ASSERT(other == OTHER_COLOR(color), m, n)

#endif  



/* liberty_of(i, j, m, n) returns true if the vertex at (i, j) is a
 * liberty of the dragon with origin at (m, n).
 */

int 
liberty_of(int i, int j, int m, int n)
{
  if ((i>0)
      && (dragon[i-1][j].origini==m)
      && (dragon[i-1][j].originj==n))
    return 1;

  if ((i<board_size-1)
      && (dragon[i+1][j].origini==m)
      && (dragon[i+1][j].originj==n))
    return 1;

  if ((j>0)
      && (dragon[i][j-1].origini==m)
      && (dragon[i][j-1].originj==n))
    return 1;

  if ((j<board_size-1)
      && (dragon[i][j+1].origini==m)
      && (dragon[i][j+1].originj==n))
    return 1;

  return 0;
}


/* 
 *
 * Rules for playing semeai (capturing races). Let M be the number of
 * liberties of my group, excluding common liberties; let Y be the
 * number of liberties of your group, excluding common liberties;
 * and let C be the number of common liberties. 
 * 
 *             If both groups have zero eyes:
 * 
 * (1) If C=0 and M=Y, whoever moves first wins. CRITICAL.
 * 
 * (2) If C=0 and M>Y, I win.
 * 
 * (3) If C=0 and M<Y, you win.
 * 
 * (4) If C>0 and M >= Y+C then your group is dead and mine is alive.
 * 
 * (5) If C>0 and M = Y+C-1 then the situation is CRITICAL. If M=0,
 * then Y=0 and C=1. In this case, whoever moves first kills.
 * If M>0, then I can kill or you can make seki.
 * 
 * (6) If M < Y+C-1 and Y < M+C-1 then the situation is seki.
 * 
 * (7) If C>0 and Y=M+C-1 then the situation is CRITICAL. If Y=0,
 * then M=0 and C=1 as in (5). If Y>0, then you can kill or I can
 * make seki.
 * 
 * (8) If C>0 and Y > M+C then your group is alive and mine is dead.
 * 
 *              If both groups have one eye:
 * 
 * In this case M > 0 and Y > 0.
 * 
 * (1) If M>C+Y then I win.
 * 
 * (2) If Y>C+M then you win.
 * 
 * (3) If C=0 and M=Y then whoever moves first kills. CRITICAL.
 * 
 * (4) If C>0 and M=C+Y then I can kill, you can make seki. CRITICAL.
 * 
 * (5) If C>0 and M<C+Y, Y<C+M, then the situation is seki. 
 * 
 * (6) If C>0 and Y=C+M, then you can kill, I can make seki. CRITICAL.
 * 
 *            If I have an eye and you dont:
 * 
 * In this case, M > 0. This situation (me ari me nashi) can
 * never be seki. The common liberties must be filled by you,
 * making it difficult to win.
 * 
 * 
 * (1) If M+C>Y then I win.
 * 
 * (2) If M+C=Y then whoever moves first wins. CRITICAL.
 * 
 * (3) If M+C<Y then you win.
 * 
 *            If you have an eye and I don't
 * 
 * In this case, Y > 0. 
 * 
 * (1) If Y+C>M you win.
 * 
 * (2) If Y+C=M whoever moves first wins. CRITICAL.
 * 
 * (3) If Y+C<M I win.
 * 
 */


static int
semeai_analyzer(int m, int n, int i, int j, int *ti, int *tj, int color)
{
  int mylibs=0, yourlibs=0, commonlibs=0;
  int yourlibi=-1, yourlibj=-1;
  int commonlibi=-1, commonlibj=-1;
  int k, l;
  int weight=75;
  int bestlib=0, currentlib;
  int other = OTHER_COLOR(color);
  int commonlib_value=0;
  
  if (dragon[m][n].status == DEAD) 
    weight +=10;
  if (dragon[i][j].status == DEAD) 
    weight +=10;

  DEBUG(DEBUG_SEMEAI, "semeai_analyzer : %m (me) vs %m (them)\n", m, n, i,j);

  /* FIXME : rather than looking at all the empty squares on the
   * board, can we not just countlib(i,j) and get a list of
   * the liberties of group at (i,j), and then look at each of
   * those to see which are also liberties of (m,n)
   */

  for (k=0; k<board_size; k++)
    for (l=0; l<board_size; l++) {
      if (   ((dragon[k][l].origini==dragon[m][n].origini) 
	      && (dragon[k][l].originj==dragon[m][n].originj)) 
	  || ((dragon[k][l].origini==dragon[i][j].origini) 
	      && (dragon[k][l].originj==dragon[i][j].originj)))
	dragon[k][l].semeai=1;

      if (p[k][l]==EMPTY) {
	if (liberty_of(k, l, m, n)) {
	    if (liberty_of(k, l, i, j)) {
	      DEBUG(DEBUG_SEMEAI, "- %m is a common liberty\n", k,l);
	      commonlibs++;
	      if ((commonlibi == -1)
		  && trymove(k, l, color, "semeai", -1, -1)) 
	      {
		if (!attack(m, n, NULL, NULL)) {
		  int delta;
		  delta = (approxlib(m,n,color,worm[m][n].liberties+2) 
			   - approxlib(i,j,other,worm[i][j].liberties) 
			   > worm[m][n].liberties - worm[i][j].liberties);
		  if (delta > commonlib_value) {
		    commonlibi=k;
		    commonlibj=l;
		    commonlib_value=delta;
		  }
		}
		popgo();
	      }
	    }
	    else {
	      DEBUG(DEBUG_SEMEAI, "- %m is a liberty of %m only\n", 
		    k,l, m, n);
	      mylibs++;
	    }
	}
	else {
	  if (liberty_of(k, l, i, j)) {
	    DEBUG(DEBUG_SEMEAI, "- %m is a liberty of %m only\n", k,l, i,j);
	    yourlibs++;
	    if (trymove(k, l, color, "semeai", -1, -1)) {
	      if (!attack(m, n, NULL, NULL)) {
		currentlib=approxlib(k, l, color, bestlib+1);
		if ((yourlibi==-1) || (currentlib > bestlib)) {
		  yourlibi=k;
		  yourlibj=l;
		  bestlib=currentlib;
		}
	      }
	      popgo();
	    }
	  }
	}
      }
    }

  if (commonlib_value > 0) {
    *ti = commonlibi;
    *tj = commonlibj;
  }
  else if (yourlibi != -1) {
    /* I can take off this liberty */
    *ti=yourlibi;
    *tj=yourlibj;
  }
  else if (commonlibi != -1) {
    *ti=commonlibi;
    *tj=commonlibj;
  }
  else
    return -1;

  DEBUG(DEBUG_SEMEAI, ": considering move further %m\n", *ti, *tj);

  /* To avoid blunders we verify that the move under consideration is
   * tactically safe. As the semeai player doesn't know about
   * squeezing throw ins yet, this requirement is very reasonable.
   */
  if (!safe_move(*ti, *tj, color)) {
    DEBUG(DEBUG_SEMEAI, "Move at %m not tactically safe.\n", *ti, *tj);
    return -1;
  }
  
  if ((dragon[m][n].genus==0) && (dragon[i][j].genus==0)) {
    if (commonlibs==0) {
      if (mylibs==yourlibs) {
	DEBUG(DEBUG_SEMEAI, "Move is critical if we are to win this semeai\n");
	ASSERT(yourlibi != -1, m, n);
	return weight;
      }
      DEBUG(DEBUG_SEMEAI, "semeai is already %s : not worth playing\n",
	    mylibs > yourlibs ? "won" : "lost");
      return -1;
    } 
    else {
      if (mylibs == yourlibs+commonlibs-1) 
	return weight;
      if (yourlibs == mylibs+commonlibs-1)
	return weight;
      DEBUG(DEBUG_SEMEAI, "seki : not worth playing\n");
      return -1;
    }

  } /* case where both groups have genus 0 */

  if ((dragon[m][n].genus==1) && (dragon[i][j].genus==1)) {

    if ((mylibs==yourlibs+commonlibs) || (yourlibs==mylibs+commonlibs))
      return weight;

    return -1;
  }

  if ((dragon[m][n].genus==1) && (dragon[i][j].genus==0))
    if (mylibs+commonlibs==yourlibs)
      return weight;

  if ((dragon[m][n].genus==0) && (dragon[i][j].genus==1))
    if (yourlibs+commonlibs==mylibs)
      return weight;
  return -1;
}


/* revise_semeai(color) changes the status of any DEAD dragon of
 * OPPOSITE_COLOR(color) which occurs in a semeai to UNKNOWN.
 * It returns true if such a dragon is found.
 */

int
revise_semeai(int color)
{
  int m, n;
  int found_one=0;
  int other=OTHER_COLOR(color);

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((dragon[m][n].semeai) 
	  && (dragon[m][n].status==DEAD) 
	  && (dragon[m][n].color==other)) 
      {
	found_one=1;
	dragon[m][n].status=UNKNOWN;
      }
    }

  return found_one;
}


/* small_semeai() addresses a deficiency in the reading code:
 * for reasons of speed, savestone3 and savestone4 do not
 * sufficiently check whether there may be an adjoining string
 * which can be attacked. So they may overlook a defensive
 * move which consists of attacking an adjoining string.
 *
 * small_semeai(), called by make_worms() searches for a 
 * string A with 3 or 4 liberties such that worm[A].attacki != -1.
 * If there is a string B next to A (of the opposite color)
 * such that worm[B].attacki != -1, the following action is
 * taken: if worm[A].liberties == worm[B].liberties, then
 * worm[A].defend is set to worm[B].defend and vice versa;
 * and if worm[A].liberties > worm[B].liberties, then worm[A].defendi
 * is set to -1.
 */

void
small_semeai()
{
  int i, j;
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
      if (p[i][j]
	  && ((worm[i][j].liberties==3) || (worm[i][j].liberties==4))
	  && (worm[i][j].attacki != -1)) {
	if ((i>0) && (p[i-1][j]==OTHER_COLOR(p[i][j])))
	  small_semeai_analyzer(i, j, i-1, j);
	if ((i<board_size) && (p[i+1][j]==OTHER_COLOR(p[i][j])))
	  small_semeai_analyzer(i, j, i+1, j);
	if ((j>0) && (p[i][j-1]==OTHER_COLOR(p[i][j])))
	  small_semeai_analyzer(i, j, i, j-1);
	if ((j<board_size) && (p[i][j+1]==OTHER_COLOR(p[i][j])))
	  small_semeai_analyzer(i, j, i, j+1);
      }
}

/* Helper function for small_semeai. Tries to resolve the
 * semeai between (i,j) and (m,n), possibly revising points
 * of attack and defense.
 */

void
small_semeai_analyzer(int i, int j, int m, int n)
{
  int ai, aj;
  int color=p[i][j];
  int other=p[m][n];

  if ((worm[m][n].attacki == -1) || (worm[m][n].liberties <3))
    return;
  if ((worm[i][j].attacki == -1) || (worm[i][j].liberties <3))
    return;


  /* FIXME: There are many more possibilities to consider */
  if (trymove(worm[i][j].attacki, worm[i][j].attackj, other, 
	      "small_semeai_analyzer", i, j)) {
    int acode=attack(m, n, &ai, &aj);
    if (acode==0) {
      popgo();
      change_defense(m, n, worm[i][j].attacki, worm[i][j].attackj);
    }
    else if (trymove(ai, aj, color,
		     "small_semeai_analyzer", i, j)) {
      if (attack(i, j, NULL, NULL)==0) {
	popgo();
	popgo();
	change_attack(i, j, -1, -1);
      }
      else {
	popgo();
	popgo();
      }
    }
  }
  if (trymove(worm[m][n].attacki, worm[m][n].attackj, color, 
	      "small_semeai_analyzer", m, n)) {
    int acode=attack(i, j, &ai, &aj);
    if (acode==0) {
      popgo();
      change_defense(i, j, worm[m][n].attacki, worm[m][n].attackj);
    }
    else if (trymove(ai, aj, other,
		     "small_semeai_analyzer", m, n)) {
      if (attack(m, n, NULL, NULL)==0) {
	popgo();
	popgo();
	change_attack(m, n, -1, -1);
      }
      else {
	popgo();
	popgo();
      }
    }
  }
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
