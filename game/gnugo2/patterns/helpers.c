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



/* Helper functions must be declared in patterns.h */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../engine/liberty.h"
#include "patterns.h"


/* not sure if this is the best way of doing this, but... */
#define UNUSED(x)  x=x
#define COMPUTE_SCORE compute_score(ti, tj, color, pattern)
#define TRYMOVE(x, y, z) trymove(x, y, z, "helper", -1, -1)
#define OFFSET(x, y, z, w) offset(x, y, ti, tj, &z, &w, transformation)
#define ARGS struct pattern *pattern, int transformation, int ti, int tj, int color
#define ON_BOARD(i, j) ((i)>=0 && (i)<board_size && (j)>=0 && (j)<board_size)

/* This file contains helper functions which assist the pattern matcher.
 * They are invoked with ti,tj = the posn on the board marked with '*'.
 * They are invoked with color = WHITE or BLACK : any pieces on the
 * board marked with 'O' in the pattern will always contain color,
 * and 'X's contain OTHER_COLOR(color)
 *
 * The helper must return a weight between 0 and pattern->patwt, plus
 * wind assistance.
 *
 * helper functions must also be declared in patterns.h.
 */





/* this helper is invoked for either of the patterns
 *   *X     tb
 *   XO     ac
 * 
 * or
 * 
 *   *O     tb
 *   OX     ac
 * 
 * In one case, we are considering whether to make a double-atari,
 * and in the other, to protect against it.
 */

int 
double_atari_helper (ARGS)
{
  int ai, aj;
  int bi, bj;
  int ci, cj;

  int ccolor, acolor;  /* colors of the c{i,j} and a{i,j} stones */
  int alib, blib;    /* num. liberties on a{i,j} and b{i,j} stones */

  if (0)
    TRACE("checking for double atari at %m\n", ti, tj);
  OFFSET(1, 0, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(1, 1, ci, cj);
  ccolor=p[ci][cj];
  ASSERT (ccolor!=0, ci, cj);
  acolor=OTHER_COLOR(ccolor);

  /* check the pattern */
  ASSERT( acolor != EMPTY, ai, aj);
  ASSERT( p[bi][bj] == acolor, ai, aj );
  ASSERT( acolor == OTHER_COLOR(ccolor), ai, aj );


  /* first, make sure the cross-cut works */

  if (!safe_move(ti, tj, ccolor) || (worm[ci][cj].liberties < 2))
    return 0;

  alib = worm[ai][aj].liberties;
  blib = worm[bi][bj].liberties;

/* See if the move is a double atari */

  if ( (alib == 2) && /* before this move, both... */
       (blib == 2) &&  /* .. the other stones have 2 libs */
       ((worm[ai][aj].origini!=worm[bi][bj].origini)||  /* and the two stones are ... */
        (worm[ai][aj].originj!=worm[bi][bj].originj))) /* ... not otherwise connected */
    return (min(COMPUTE_SCORE, min(worm[ai][aj].value, worm[bi][bj].value)));

  /* See if the move is atari on one stone with a ladder on the other */

  /* FIXME : this is where having flags on trymove() would be useful : we can
   * tell trymove() to return false if the move has only one liberty, for example
   */

  if ((alib == 2) && (blib < 5))
    if (TRYMOVE(ti, tj, ccolor)) {
      if (countlib(ai, aj, acolor) == 1) {
	if (TRYMOVE(libi[0], libj[0], acolor)) {
	  if (p[bi][bj] && attack(bi, bj, NULL, NULL)) {
	    popgo();
	    popgo();
	    return (min(COMPUTE_SCORE, min(worm[ai][aj].value, worm[bi][bj].value)));
	  }
	  popgo();
	}
      }
      popgo();
    }

  /* See if the move is atari on one stone with a ladder on the
     other */

  if ((blib == 2) && (alib < 5))
    if (TRYMOVE(ti, tj, ccolor)) {
      if (countlib(bi, bj, acolor)==1) {
	if (TRYMOVE(libi[0], libj[0], acolor)) {
	  if (p[ai][aj] && attack(ai, aj, NULL, NULL)) {
	    popgo();
	    popgo();
	    return (min(COMPUTE_SCORE, min(worm[ai][aj].value, worm[bi][bj].value)));
	  }
	  popgo();
	}
      }
      popgo();
    }

  return(0);
}

/* 

Actual score depends on size of O, is it cutstone?

X??     X??   O has 3 liberties, last X has 2 or 3
.O?     .b?   break out!
X*X	Xta
000	000

:8,95,0,0,0,0,break_out_helper

*/

int 
break_out_helper (ARGS)
{
  int ai, aj, bi, bj, tval;
  int other=OTHER_COLOR(color);
  UNUSED(pattern);

  OFFSET(0, 1, ai, aj);  /* 1 to east */
  OFFSET(-1, 0, bi, bj); /* 2 to north */

  ASSERT(p[ai][aj]==other, ai, aj);
  ASSERT(p[bi][bj]==color, ai, aj);
  if ((worm[bi][bj].liberties>2) && (worm[ai][aj].liberties<3)) {
    if (worm[bi][bj].cutstone==2)
      tval=88+worm[bi][bj].size;
    else if (worm[bi][bj].cutstone==1)
      tval=65+4*worm[bi][bj].size;
    else
      tval=8*worm[bi][bj].size;
    return min(tval, COMPUTE_SCORE);
  }
  return (0);
}



/*

XO     cb
O*     at

Check whether a cut is feasible and effective.

*/

int 
basic_cut_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int acolor, ccolor;
  UNUSED(pattern);
  
  OFFSET(0, -1, ai, aj);  /* O to west */
  OFFSET(-1, 0, bi, bj);  /* O to north */
  OFFSET(-1, -1, ci, cj); /* X to northwest */

  acolor=p[ai][aj];
  ccolor=OTHER_COLOR(acolor);

  ASSERT(p[ai][aj]!=EMPTY, ai, aj);
  ASSERT(p[bi][bj]==acolor, bi, bj);
  ASSERT(p[ci][cj]==ccolor, ci, cj);

  /* If c is a ko stone, assume that we would lose the ko. */
  if (worm[ci][cj].attacki != -1
      && (ccolor == color
	  || !worm[ci][cj].ko))
    return 0;
  if (worm[ti][tj].ko)
    return 0;

  if (TRYMOVE(ti, tj, ccolor)) {
    if (attack(ti, tj, NULL, NULL) || attack(ci, cj, NULL, NULL)) {
      popgo();
      return 0;
    }
#if 0
    /* This case should be covered by attack(ci, cj). */
    if (worm[ci][cj].liberties==1) {
      popgo();
      return 0;    /* better to capture (ci,cj) */
    }
#endif
    popgo();
  }
  else
    return 0;

  if (!safe_move(ti, tj, acolor))
    return 0;

  /* Cut ok. */
  return 1;
}


/*

XO     cb
O*     at

:4,82,0,0,0,0,0,20,diag_miai_helper

Connect automatically unless one cutting stone can be captured after
opponent cuts.

*/

int 
diag_miai_helper (ARGS)
{
  int ai, aj, bi, bj, tval;
  
  OFFSET(0, -1, ai, aj);  /* O to west */
  OFFSET(-1, 0, bi, bj);  /* O to north */

  tval = min(connection_value(ai, aj, bi, bj, ti, tj), COMPUTE_SCORE);
  if (tval > 0
      && basic_cut_helper(pattern, transformation, ti, tj, color)) {
    return tval;
  }
  else
    return 0;
}


/*

XO?   cb?
O.*   adt
O.?   O.?

Modification of diag_miai_helper to make hanging connections when feasible

*/

int 
diag_miai2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, tval;
  int acolor, ccolor;
  
  OFFSET(0, -2, ai, aj);  
  OFFSET(-1, -1, bi, bj); 
  OFFSET(-1, -2, ci, cj); 
  OFFSET(0, -1, di, dj); 

  acolor=p[ai][aj];
  ccolor=OTHER_COLOR(acolor);

  ASSERT(p[ai][aj]!=EMPTY, ai, aj);
  ASSERT(p[bi][bj]==acolor, bi, bj);
  ASSERT(p[ci][cj]==ccolor, ci, cj);

  if (worm[bi][bj].liberties==1)
    return 0;

  tval=min(connection_value(ai, aj, bi, bj, ti, tj), COMPUTE_SCORE);
  if (tval == 0)
    return 0;

  if (!basic_cut_helper(pattern, transformation, di, dj, color))
    return 0;

  /* Does the proposed move defend properly? */
  if (TRYMOVE(ti, tj, acolor)) {
    if (attack(ti, tj, NULL, NULL)
	|| attack(ai, aj, NULL, NULL)
	|| attack(bi, bj, NULL, NULL))
      tval=0;
    popgo();
  }
  else
    return (0);
  
  if (!safe_move(ti, tj, acolor))
    return (0);
  return tval;
}



/*

  prevent (or make) connection

  ?O?                   ?c?   
  XOX                   bca
  .*.                   .*.
  ---                   ---
  
*/

int 
connect_under_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval;
  
  OFFSET(-1, 1, ai, aj); 
  OFFSET(-1, -1, bi, bj); 
  OFFSET(-1, 0, ci, cj);
  
  tval=COMPUTE_SCORE;
  
  /* Test whether a cut would be successful. */
  if (!safe_move(ti, tj, p[ci][cj]))
    return 0;

  /* Would a connection be successful? */
  if (TRYMOVE(ti, tj, p[ai][aj])) {
    if (attack(ai, aj, NULL, NULL)
	|| attack(bi, bj, NULL, NULL))
      tval=0;
    popgo();
  }
  else
    return 0;

  return tval;
}


/*

XXX.     XXX.      If a has only 2 liberties this is life/death
OO*O     OatO      urgent if only 3 liberties
....	 ....
----	 ----

or

OOO.     OOO.
XX*X     XatX
....	 ....
----	 ----

:8,92,0,0,0,0,save_on_second_helper

*/

int 
save_on_second_helper (ARGS)
{
  int ai, aj, alib, tval;
  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, -1, ai, aj);
  alib = worm[ai][aj].liberties;
  tval = COMPUTE_SCORE;

  if (alib==2)
    return tval;
  else if (alib==3)
    return tval-20;
  else
    return tval-40;
}





/*

??.??     ??.??
.*X..	  .tb..
?.OO.	  acOO.
?....	  ?....
?....	  ?....
-----	  -----

:8,65,15,1,0,0,hane_on_4_helper

*/

int 
hane_on_4_helper (ARGS)
{
  int ai, aj;
  int bi, bj;
  int ci, cj;
  int other = OTHER_COLOR(color);

  OFFSET(1, -1, ai, aj); /* ? on third line */
  OFFSET(0, 1, bi, bj); /* X on fourth line */
  OFFSET(1, 0, ci, cj); /* . on third line */
  ASSERT((p[bi][bj] == other), bi, bj);
  ASSERT((p[ci][cj] == EMPTY), ci, cj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ci, cj, other)) {
      if (attack(bi, bj, NULL, NULL) || attack(ci, cj, NULL, NULL)) {
	popgo();
	popgo();
	return(COMPUTE_SCORE);
      }
      popgo();
    }
    popgo();
  }
  return (0);
}


/*

O?     b?
*X     tc
XO     da

:8,68,0,0,10,1,alive11_helper

*/

int 
alive11_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;

  OFFSET(1, 1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(0, 1, ci, cj);
  OFFSET(1, 0, di, dj);

  if (TRYMOVE(ti, tj, p[ai][aj])) {
    if (attack(ai,aj,NULL,NULL)) {
      popgo();
      return(0);
    }
    popgo();
    return min(COMPUTE_SCORE, 
	       max(connection_value(ai, aj, bi, bj, ti, tj),
		   connection_value(ci, cj, di, dj, ti, tj)));
  }
  return(0);
}


/*

?XO?     ?be?  Take an important ko
X*XO	 atXd
?XO?	 ?cf?

or:

?OX?     ?be?  Connect an important ko
O*OX	 atOd
?OX?	 ?cf?

The ko is deemed important if it unites distinct dragons, unless they
are already all alive. It is deemed still more important if it cuts or
connects dragons for both colors.

Note: This change may have the effect to overvalue certain kos. I
still would like to try it out though, since it's necessary to somehow
increase the value for many early kos. /gf

*/

int 
ko_important_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj;
  int tval1=0, tval2=0;
  UNUSED(pattern);

  if (!worm[ti][tj].ko)
    return (0);

  if (!safe_move(ti, tj, color))
    return 0;

  OFFSET( 0, -1, ai, aj);
  OFFSET(-1,  0, bi, bj);
  OFFSET( 1,  0, ci, cj);
  OFFSET( 0,  2, di, dj);
  OFFSET(-1,  1, ei, ej);
  OFFSET( 1,  1, fi, fj);
  ASSERT((p[ai][aj]==p[bi][bj]), bi, bj);
  ASSERT((p[ai][aj]==p[ci][cj]), ci, cj);
  ASSERT((p[di][dj]==p[ei][ej]), ei, ej);
  ASSERT((p[di][dj]==p[fi][fj]), fi, fj);
  if ((dragon[ai][aj].origini != dragon[bi][bj].origini) ||
      (dragon[ai][aj].originj != dragon[bi][bj].originj))
    tval1=connection_value(ai, aj, bi, bj, ti, tj);
  if ((dragon[ai][aj].origini != dragon[ci][cj].origini) ||
      (dragon[ai][aj].originj != dragon[ci][cj].originj))
    tval1=max(tval1, connection_value(ai, aj, ci, cj, ti, tj));
  if ((dragon[di][dj].origini != dragon[ei][ej].origini) ||
      (dragon[di][dj].originj != dragon[ei][ej].originj))
    tval2=connection_value(di, dj, ei, ej, ti, tj);
  if ((dragon[di][dj].origini != dragon[fi][fj].origini) ||
      (dragon[di][dj].originj != dragon[fi][fj].originj))
    tval2=max(tval1, connection_value(di, dj, fi, fj, ti, tj));

/* Since the player who loses the ko gets two outside moves, we should 
   divide the value by 2.
*/

  return min(COMPUTE_SCORE, (tval1+tval2)/2);
}



/*

?XO?     ?be?  Take an important ko
X*XO	 atXd
----     ----

*/

int 
edge_ko_important_helper (ARGS)
{
  int ai, aj, bi, bj, di, dj, ei, ej;
  int tval1=0, tval2=0;
  UNUSED(pattern);

  if (!worm[ti][tj].ko) return (0);

  if (!safe_move(ti, tj, color))
    return (0);

  OFFSET( 0, -1, ai, aj);
  OFFSET(-1,  0, bi, bj);
  OFFSET( 0,  2, di, dj);
  OFFSET(-1,  1, ei, ej);

  ASSERT((p[ai][aj]==p[bi][bj]), bi, bj);
  ASSERT((p[di][dj]==p[ei][ej]), ei, ej);
  if ((dragon[ai][aj].origini != dragon[bi][bj].origini) ||
      (dragon[ai][aj].originj != dragon[bi][bj].originj))
    tval1=connection_value(ai, aj, bi, bj, ti, tj);
  if ((dragon[di][dj].origini != dragon[ei][ej].origini) ||
      (dragon[di][dj].originj != dragon[ei][ej].originj))
    tval2=connection_value(di, dj, ei, ej, ti, tj);
  return min(COMPUTE_SCORE, (tval1+tval2)/2);
}


/*


?.?       ?c?
X*X       bta     double attack
?O?       ?O?

We play between the two X strings if they are distinct and
a move there threatens both strings simultaneously. The pattern
is not recognized if the move at * has only two liberties.

*/

int 
double_attack_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, acolor, tcolor;
  int tval=0;
  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(0, -1, bi, bj);
  OFFSET(-1, 0, ci, cj);
  acolor=p[ai][aj];
  tcolor=OTHER_COLOR(acolor);
  
  ASSERT(acolor==p[bi][bj], ai, aj);
  if ((dragon[ai][aj].origini==dragon[bi][bj].origini) && 
      (dragon[ai][aj].originj==dragon[bi][bj].originj))
    return (0);

  if ((worm[ai][aj].liberties==1) || (worm[bi][bj].liberties==1))
    return (0);

  if (TRYMOVE(ti, tj, tcolor)) {
    if (attack(ti, tj, NULL, NULL)) {
      popgo();
      return (0);
    }
    if (TRYMOVE(ci, cj, acolor)) {
      if (p[ai][aj] && attack(ai, aj, NULL, NULL) &&
	  p[bi][bj] && attack(bi, bj, NULL, NULL))
	tval=min(COMPUTE_SCORE, min(worm[ai][aj].value, worm[bi][bj].value));
      popgo();
    }
    popgo();
  }
  return (tval);
}
    


/*

??.??     ??.??
.O.*?	  .aet?
X...?	  Xcd.?
?.O.?	  ?.b.?

*/

int 
tobi_connect_helper (ARGS)
{
  int ai, aj, bi, bj;
  int ci, cj, di, dj;
  int ei, ej;
  int other=OTHER_COLOR(color);
  int tval=0;

  UNUSED(pattern);

  OFFSET(0,-2, ai, aj);
  OFFSET(2,-1, bi, bj);
  OFFSET(1,-2, ci, cj);
  OFFSET(1,-1, di, dj);
  OFFSET(0,-1, ei, ej);
  ASSERT(p[ai][aj]==color, ai, aj);
  ASSERT(p[bi][bj]==color, bi, bj);
  ASSERT(p[ci][cj]==EMPTY, ci, cj);
  ASSERT(p[di][dj]==EMPTY, di, dj);
  ASSERT(p[ei][ej]==EMPTY, ei, ej);

  if (TRYMOVE(ci, cj, other)) {
    if (TRYMOVE(di, dj, color)) {
      if (TRYMOVE(ei, ej, other)) {
	if (p[di][dj] && !attack(di, dj, NULL, NULL) &&
	    p[ei][ej] && !attack(ei, ej, NULL, NULL))
	  tval=min(tval, connection_value(ai, aj, bi, bj, ti, tj));
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return(tval);
}


/*

??X?     ??c?
.O*?	 .Ot?
X...	 dab.
?.O?	 ?.O?

*/

int 
attach_connect_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;

  int other=OTHER_COLOR(color);
  int tval=0;

  OFFSET(1,-1, ai, aj);
  OFFSET(1, 0, bi, bj);
  OFFSET(-1, 0, ci, cj);
  OFFSET(1, -2, di, dj);

  ASSERT(p[ai][aj]==EMPTY, ai, aj);
  ASSERT(p[bi][bj]==EMPTY, bi, bj);

  if (TRYMOVE(ai, aj, other)) {
    if (TRYMOVE(bi, bj, color)) {
      if (TRYMOVE(ti, tj, other)) {
	if (p[ai][aj] && !attack(ai, aj, NULL, NULL) &&
	    !attack(ti, tj, NULL, NULL))
	  tval=min(COMPUTE_SCORE, connection_value(ci, cj, di, dj, ti, tj));
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return(tval);
}


/*

O?    c?
X*    at
O?    b?

*/

int 
take_valuable_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int acolor, bcolor;
  int tval=0;

  UNUSED(pattern);

  OFFSET(0,-1, ai, aj);  
  OFFSET(1,-1, bi, bj);  
  OFFSET(-1,-1, ci, cj);  

  if (worm[ai][aj].defendi == -1)
    return (0);
  if ((worm[ai][aj].attacki == ti) && (worm[ai][aj].attackj == tj)) {
    acolor=p[ai][aj];
    ASSERT(acolor != EMPTY, ai, aj);
    bcolor=OTHER_COLOR(acolor);
    ASSERT(p[bi][bj]==bcolor, bi, bj);
    ASSERT(p[ci][cj]==bcolor, ci, cj);
    if (TRYMOVE(ti, tj, acolor)) {
      if (!attack(ti, tj, NULL, NULL)) {
	popgo();
	if (!snapback(ti, tj, ti, tj, color))
	  tval=min(tval, connection_value(bi, bj, ci, cj, ti, tj));
      } else popgo();
    }
  }
  return(tval);
}


/*

?O.     ?O.
OX.     Oa.       common geta
O.*	O.t
?.?	?.?

*/

int 
common_geta_helper (ARGS)
{
  int ai, aj;
  int tval=COMPUTE_SCORE;
  
  OFFSET(-1, -1, ai, aj);  
  if (worm[ai][aj].attacki != -1)
    return (0);
  if (TRYMOVE(ti, tj, OTHER_COLOR(p[ai][aj]))) {
    if (find_defense(ai, aj, NULL, NULL)) {
      popgo();
      return (0);
    }
    popgo();
  }
  return (min(tval, worm[ai][aj].value));
}



/* 

.*X  .ta
OXO  OXc
?.?  ?b?

*/

int 
atari_attack_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int tval=0;
  
  OFFSET(0, 1, ai, aj);  
  OFFSET(2, 0, bi, bj);  
  OFFSET(1, 1, ci, cj);  

  if ((worm[ai][aj].attacki != -1) || (worm[ci][cj].liberties == 1))
    return (0);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (p[ai][aj] && attack(ai, aj, NULL, NULL))
	tval=min(COMPUTE_SCORE, worm[ai][aj].value);
      popgo();
    }
    popgo();
  }
  return (tval);
}


/*

OX.|     OX.|
X..|	 X..|
X.*|	 a.t|
XX.|	 XX.|
OO.|	 OO.|
?oo|	 ?oo|
??o|	 ??o|

:8,120,0,0,0,0,O,0,eye_stealing_helper

*/


int 
eye_stealing_helper (ARGS)
{
  int ai, aj;

  UNUSED(pattern);
  UNUSED(color);

  
  OFFSET(0, -2, ai, aj);  
  if (worm[ai][aj].liberties==3) {
    if (dragon[ai][aj].genus==0)
      return (120);                /* killing move */
    if (dragon[ai][aj].genus==1)  
      return (75);                 /* might kill --- probably sente */
    return (30);                   /* big endgame move */
  }
  return (0);
}


/*

??o|     ??o|
?Oo|     ?Ob|     sente hane
XX*|	 Xct|
...|	 .da|
?.?|	 ?.?|

:8,75,0,0,0,0,OX,0,sente_hane_helper

FIXME:

Really, we should be determining the score only after
determining if this is double sente. The pattern can
also be generalized and four cases distinguished:
double sente, my sente, your sente, gote.

*/

int 
sente_hane_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(1, 0, ai, aj);  
  OFFSET(-1, 0, bi, bj);  
  OFFSET(0, -1, ci, cj);  
  OFFSET(1, -1, di, dj);


  if (p[bi][bj]==color) {
    if (dragon[ci][cj].status == UNKNOWN)
      return (COMPUTE_SCORE);
    else 
    if (dragon[ci][cj].status == ALIVE)
      return (max(0,COMPUTE_SCORE-15));
  }
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (TRYMOVE(bi, bj, color)) {
	if (TRYMOVE(di, dj, other)) {
	  if (p[ti][tj] && attack(ti, tj, NULL, NULL)) {  
	    popgo();      
	    popgo();	    
	    popgo();
	    popgo();	    
	    return(0);    /* not sente and dangerous */
	  }
	  popgo();
	}
	if (p[ai][aj] && attack(ai, aj, NULL, NULL)) {
	  if (dragon[ci][cj].status == UNKNOWN)
	    tval=COMPUTE_SCORE;
	  else 
	    if (dragon[ci][cj].status == ALIVE)
	    tval=max(0,COMPUTE_SCORE-15);
	}
	else 
	  tval=30;
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return (tval);
}



/*

*X       tc
XO       bO
O.       Oa
?O       ?O

:8,85,0,0,0,0,-,0,fight_ko_helper

*/


int 
fight_ko_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int ei, ej;
  int tval=0;
  UNUSED(pattern);

  OFFSET(2, 1, ai, aj);  
  OFFSET(0, 1, bi, bj);  
  OFFSET(1, 0, ci, cj);  

  if (!worm[ai][aj].ko) 
    return (0);
  if (TRYMOVE(ai, aj, color)) {
    if (p[bi][bj] && attack(bi, bj, &ei, &ej)) {
      if ((ei==ti) && (ej==tj))
	tval=worm[bi][bj].value-5;
	  }
      if (p[ci][cj] && attack(ci, cj, &ei, &ej)) {
	if ((ei==ti) && (ej==tj))
	  tval=max(tval,worm[ci][cj].value-5);
	    }
      popgo();
  }
  tval=min(COMPUTE_SCORE,tval);
  return (tval);
}


/*
      start a ko fight

?OX?     ?Ob?
.XOX	 aXOX
?*.O	 ?tcO
?oOo	 ?oOo

:8,85,0,0,0,0,-,0,fight_ko2_helper

*/

int 
fight_ko2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval=0;
  int other=OTHER_COLOR(color);
  UNUSED(pattern);

  OFFSET(-1, -1, ai, aj);  
  OFFSET(-2, 1, bi, bj);  
  OFFSET(0, 1, ci, cj);  

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (TRYMOVE(ci, cj, color)) {
	if (p[bi][bj] && attack(bi, bj, NULL, NULL))
	  tval=min(COMPUTE_SCORE,worm[bi][bj].value);
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return (tval);
}


/*

XO?      aO?                try to rescue
.*X      .*X


:8,0,0,0,0,0,D,0,try_to_rescue_helper
 
The purpose of this helper is to prevent attacker()
from overlooking the possibility that a move at *
would save the X string at top; without some such measure
GNU Go might wrongly believe that the string is
dead, because the reading routines fail to consider
this move. 

The pattern has no value but is labelled D to force
execution.

*/


int 
try_to_rescue_helper (ARGS)
{
  int ai, aj;
  int other=OTHER_COLOR(color);

  UNUSED(pattern);

  OFFSET(-1, -1, ai, aj);   /* X at top */
  if (worm[ai][aj].attacki != -1)
    if (TRYMOVE(ti, tj, other)) {
      if (!attack(ai, aj, NULL, NULL)) {
	popgo();
	change_defense(ai, aj, ti, tj);
      }
      else
	popgo();
    }
  return COMPUTE_SCORE;
}


/*

??X.|             ??X.|
?OOX|             ?OcX|
..X.|             baX.|
O.*.|             O.*.|
oooo|             oooo|

:8,78,0,0,0,0,-,0,clamp1_helper

*/


int 
clamp1_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(-1, -1, ai, aj);
  OFFSET(-1, -2, bi, bj);
  OFFSET(-2, 0, ci, cj);
  
  ASSERT(p[ai][aj]==EMPTY, ai, aj);
  ASSERT(p[bi][bj]==EMPTY, bi, bj);
  ASSERT(p[ci][cj]==color, ci, cj);
  if (TRYMOVE(ai, aj, other)) {
    if (TRYMOVE(bi, bj, color)) {
      if (p[ci][cj] && !attack(ci, cj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return(tval);
}


/*

|.Xoo             |.Xoo
|XOOo             |XcOo
|.X.O             |.b.O
|.*.?             |.*a?
|..X?             |..X?

*/


int 
clamp2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(-2, 0, ci, cj);
  
  if (worm[ci][cj].liberties<3)
    return (0);
  ASSERT(p[ai][aj]==EMPTY, ai, aj);
  ASSERT(p[bi][bj]==other, bi, bj);
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(bi, bj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return (tval);
}


/* Jump out into nothingness. To avoid jumping into our own territory,
   we don't allow the strategic distance to the opponent to be too
   large. Also we never ever jump into our own established eyespace. */

int 
jump_out_helper (ARGS)
{
  int dist;
  int own_eyespace;

  UNUSED(transformation);

  if (0)
    TRACE("  jump out to %m --- distance to B=%d; to W=%d\n", ti, tj, 
	  distance_to_black[ti][tj], distance_to_white[ti][tj]);
  if (color==WHITE)
  {
    dist = strategic_distance_to_black[ti][tj];
    own_eyespace = (white_eye[ti][tj].color == WHITE_BORDER);
  }
  else
  {
    dist = strategic_distance_to_white[ti][tj];
    own_eyespace = (black_eye[ti][tj].color == BLACK_BORDER);
  }
  
  if (dist != -1 && dist <= 6 && !own_eyespace)
    return COMPUTE_SCORE;
  else
    return 0;
}


/* Make a long jump into nothingness. Since these jumps are not
   securely connected we don't use them to jump into the opponent's
   zone of control. */

int 
jump_out_far_helper (ARGS)
{
  if (area_color(ti, tj) != OTHER_COLOR(color))
    return jump_out_helper(pattern, transformation, ti, tj, color);
  else
    return 0;
}


/*

double connection 

?O*               ?dt  
X.O               caO
?O.               ?eb

*/

int 
double_connection_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int acolor, dcolor, tval;
  
  if (0)
    TRACE("double connection helper at %m\n", ti, tj);

  OFFSET(1, -1, ai, aj);
  OFFSET(2, 0, bi, bj);
  OFFSET(1, -2, ci, cj);
  OFFSET(0, -1, di, dj);
  OFFSET(2, -1, ei, ej);
  
  ASSERT(p[ai][aj]==EMPTY, ai, aj);
  ASSERT(p[bi][bj]==EMPTY, bi, bj);
  ASSERT(p[ci][cj]!=EMPTY, ci, cj);

  acolor=p[ci][cj];
  dcolor=OTHER_COLOR(acolor);

  ASSERT(p[di][dj]==dcolor, di, dj);
  ASSERT(p[ei][ej]==dcolor, ei, ej);

  tval=min(connection_value(di, dj, ei, ej, ti, tj), COMPUTE_SCORE);
  if (tval==0)
    return (0);
  if (TRYMOVE (ai, aj, acolor)) 
    {
      if (TRYMOVE (ti, tj, dcolor)) {
	if (TRYMOVE (bi, bj, acolor)) 
	  {
	    if (!p[ci][cj] ||
		!p[bi][bj] ||
		attack(ci, cj, NULL, NULL) ||
		attack(bi, bj, NULL, NULL))
	      tval=0;
	    popgo();
	  }
	else 
	  tval=0;        /* cutting at b is illegal */
	popgo();
      }
      if (TRYMOVE (bi, bj, dcolor)) {           /* defender gets to decide */
	if (TRYMOVE (ti, tj, acolor)) 
	  {
	    if (!p[ci][cj] ||
		!p[ti][tj] ||
		attack(ci, cj, NULL, NULL) ||
		attack(ti, tj, NULL, NULL))
	      tval=0;
	    popgo();
	  }
	else 
	  tval=0;       /* cutting at t is illegal */
	popgo();
      }
      popgo();
    }
  else                   
    tval=0;         /* pushing at t is illegal; */
  return (tval);
}


/*

break double connection 

?X.               ?da  
O*X               ctO
?X.               ?eb

*/

int
double_does_break_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int acolor, dcolor;
  int safe=0;
  UNUSED(pattern);
  UNUSED(color);
  
  OFFSET(-1, 1, ai, aj);
  OFFSET(1, 1, bi, bj);
  OFFSET(0, -1, ci, cj);
  OFFSET(-1, 0, di, dj);
  OFFSET(1, 0, ei, ej);
  
  ASSERT(p[ai][aj]==EMPTY, ai, aj);
  ASSERT(p[bi][bj]==EMPTY, bi, bj);
  ASSERT(p[ci][cj]!=EMPTY, ci, cj);

  acolor=p[ci][cj];
  dcolor=OTHER_COLOR(acolor);

  ASSERT(p[di][dj]==dcolor, di, dj);
  ASSERT(p[ei][ej]==dcolor, ei, ej);

  if (TRYMOVE (ti, tj, acolor)) 
    {
      if (TRYMOVE (ai, aj, dcolor)) {
	if (TRYMOVE (bi, bj, acolor)) 
	  {
	    if (!p[ci][cj] ||
		!p[bi][bj] ||
		attack(ci, cj, NULL, NULL) ||
		attack(bi, bj, NULL, NULL))
	      safe = 1;
	    popgo();
	  }
	else 
	  safe = 1;        /* cutting at b is illegal */
	popgo();
      }
      if (TRYMOVE (bi, bj, dcolor)) {           /* defender gets to decide */
	if (TRYMOVE (ai, aj, acolor)) 
	  {
	    if (!p[ci][cj] ||
		!p[ai][aj] ||
		attack(ci, cj, NULL, NULL) ||
		attack(ai, aj, NULL, NULL))
	      safe = 1;
	    popgo();
	  }
	else 
	  safe = 1;       /* cutting at a is illegal */
	popgo();
      }
      popgo();
    }
  else                   
    safe = 1;         /* pushing at a is illegal; */
  return !safe;
}
  

int 
double_break_helper (ARGS)
{
  int di, dj, ei, ej;
  int tval;

  if (0)
    TRACE("double break helper at %m\n", ti, tj);

  OFFSET(-1, 0, di, dj);
  OFFSET(1, 0, ei, ej);
  
  tval=min(connection_value(di, dj, ei, ej, ti, tj), COMPUTE_SCORE);
  if (tval==0)
    return (0);

  if (double_does_break_helper(pattern, transformation, ti, tj, color))
    return tval;
  
  return 0;
}



/*

?.?                 ?.?
...                 abc
O*O                 OtO
?X?                 ?X?

*/

int 
peep_connect_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval;
  int other=OTHER_COLOR(color);

  if (0)
    TRACE("peep connect helper at %m\n", ti, tj);
  tval=COMPUTE_SCORE;

  OFFSET(-1, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(-1, 1, ci, cj);

  if (TRYMOVE(ti, tj, other)) {
    if (TRYMOVE(bi, bj, color)) {
      if (approxlib(ai, aj, other, 3) < 2)
	if (TRYMOVE(ci, cj, other)) {
	  if (!p[ci][cj] || attack(ci, cj, NULL, NULL))
	    tval=0;
	  popgo();
	}
      if (approxlib(ci, cj, other, 3)<2)
	if (TRYMOVE(ai, aj, other)) {
	  if (attack(ai, aj, NULL, NULL))
	    tval=0;
	  popgo();
	}
      popgo();
    }
    popgo();
  }
  return (tval);
}


/*

XO        bO
*.        ta
.X        .X

*/

int 
break_hane_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 0, bi, bj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (p[bi][bj]==EMPTY) {
	popgo();
	popgo();
	return(0);
      }
      if (!p[bi][bj] || attack(bi, bj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return (tval);
}

/*

Look for a single move that destroys two half eyes

?X?   ?X?
O*O   O*O
.O.   abc

*/

int 
two_half_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;

  OFFSET(1, -1, ai, aj);
  OFFSET(1, 0, bi, bj);
  OFFSET(1, 1, ci, cj);

  if ((2*dragon[bi][bj].genus + dragon[bi][bj].heyes==4)
      && (half_eye[ai][aj].type==HALF_EYE)
      && (half_eye[ci][cj].type==HALF_EYE))
    return (COMPUTE_SCORE);
  return (0);
}  


/*

?X??                   ?ef?
.*..                   .*c.
OX..                   Oabd
?XO.                   ?XO.

:8,80,0,0,0,0,-,0,trap_helper

*/

int 
trap_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj;
  int other=OTHER_COLOR(color);

  OFFSET(1, 0, ai, aj);
  OFFSET(1, 1, bi, bj);
  OFFSET(0, 1, ci, cj);
  OFFSET(1, 2, di, dj);
  OFFSET(-1, 0, ei, ej);
  OFFSET(-1, 1, fi, fj);

  if (worm[ai][aj].liberties != 2) 
    return (0);
  if (worm[ei][ej].liberties==1)
    return (COMPUTE_SCORE);
  if ((worm[ei][ej].liberties==2) && (p[ei][ej]==p[fi][fj]))
    return (COMPUTE_SCORE);
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (TRYMOVE (ci, cj, color)) {
	if (TRYMOVE(di, dj, other)) {
	  if (!p[ei][ej] || attack(ei, ej, NULL, NULL)) {
	    popgo();
	    popgo();
	    popgo();    
	    popgo();
	    return (COMPUTE_SCORE);
	  }
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return (0);
}


/*

OOX      Oea          defend connection
..O      .db
.*X      .*c

*/

int 
tiger_defense_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int other=OTHER_COLOR(color);

  OFFSET(-2, 1, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(0, 1, ci, cj);

  if ((worm[bi][bj].liberties >1) && (worm[ai][aj].attacki == -1) &&
      (worm[ci][cj].attacki == -1)) {

    if (TRYMOVE(ti, tj, color)) {
      if (!p[ci][cj] || attack(ci, cj, NULL, NULL)) {
	popgo();
	return (min(COMPUTE_SCORE, worm[ci][cj].value));
      }
      popgo();
    }
    
    OFFSET(-1, 0, di, dj);
    OFFSET(-2, 0, ei, ej);
    
    if (TRYMOVE(di, dj, other)) {
      if (!attack(di, dj, NULL, NULL)) {
	popgo();
	return (min(COMPUTE_SCORE, connection_value(bi, bj, ei, ej, ti, tj)));
      }
      popgo();
    }
    return (min(20, COMPUTE_SCORE));
  }
  return (0);
}


/*

The purpose of this pattern is to override double_atari_helper when
it proposes move that will backfire. If capturing the string at c 
results in the loss of e, we'd rather just connect.

?..               ?fc
*OX               tOd
OX.               eXa
?.?               ?b?

*/

int 
backfire_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj;
  int other=OTHER_COLOR(color);
  int tval=0;

  OFFSET(1, 2, ai, aj);
  OFFSET(2, 1, bi, bj);
  OFFSET(-1, 2, ci, cj);
  OFFSET(0, 2, di, dj);
  OFFSET(1, 0, ei, ej);
  OFFSET(-1, 1, fi, fj);

  if (approxlib(ai, aj, color,2) == 1)
    return (0);

  if (TRYMOVE(ai, aj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (attack(di, dj, NULL, NULL) && TRYMOVE(ci, cj, color)) {
	if ((approxlib(ti, tj, other, 2) >1) && TRYMOVE(ti, tj, other)) {
	  if (TRYMOVE(fi, fj, color)) {
	    if (p[ei][ej] && attack(ei, ej, NULL, NULL))
	      tval = COMPUTE_SCORE;
	    popgo();
	  }
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return tval;
}

/*

catch two birds with one stone

OX*             Oat
XO?             bO?        

:8,30,0,0,0,0,-,0,two_bird_helper

*/

int 
two_bird_helper (ARGS)
{
  int ai, aj, bi, bj;

  OFFSET(0, -1, ai, aj);
  OFFSET(1, -2, bi, bj);

  if ((worm[ai][aj].attacki == ti) && (worm[ai][aj].attackj == tj) &&
      (worm[bi][bj].attacki != -1) && (worm[ai][aj].defendi != -1) &&
      (worm[bi][bj].defendi != -1))
    if (TRYMOVE(ti, tj, color)) {
      if (!(p[bi][bj] && find_defense(bi, bj, NULL, NULL))) {
	popgo();
	change_attack(bi, bj, ti, tj);
      }
      else
	popgo();
      return (COMPUTE_SCORE);
    }
  return (0);
}


/*

.X.X      .X.X       kosumi tesuji
.*.X      b*.a
X.O?      X.O?

*/

int
kosumi_tesuji_helper (ARGS)
{
  int ai, aj, bi, bj;
  int other=OTHER_COLOR(color);

  OFFSET(0, 2, ai, aj);
  OFFSET(0, -1, bi, bj);

  if (worm[ai][aj].attacki != -1)
    return (0);
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (attack(ai, aj, NULL, NULL)) {
	popgo();
	popgo();
	return (COMPUTE_SCORE);
      }
      popgo();
    }
    popgo();
  }
  return (0);
}

/*

O??           a??
..X           bcX
X*X           XtX
?O?           ?O?

*/

int
break_connection_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval;
  int other=OTHER_COLOR(color);

  OFFSET(-2, -1, ai, aj);
  OFFSET(-1, -1, bi, bj);
  OFFSET(-1, 0, ci, cj);

  tval=COMPUTE_SCORE;
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE (ci, cj, other)) {
      if (TRYMOVE (bi, bj, color)) {
	
	if ((p[ti][tj]==0) || attack(ti, tj, NULL, NULL) || 
	    attack(ai, aj, NULL, NULL))
	  tval=0;
	popgo();
      }
      else tval=0;
      popgo();
    }
    popgo();
  }
  else tval=0;
  return tval;
}



/*

In this situation the reading code currently doesn't find the move.

...??              ...??           
O.*.?              c t.?
?X...              ?a.b.
?X...              ?a...
??O??              ??O??

*/

int
cap3_helper (ARGS)
{
  int ai, aj, bi, bj;

  OFFSET(1, -1, ai, aj);
  OFFSET(1, 1, bi, bj);

  if (worm[ai][aj].attacki != -1)
    return 0;

  if (TRYMOVE(ti, tj, color)) {
    if (attack(ai, aj, NULL, NULL) && !find_defense(ai, aj, NULL, NULL)) {
      popgo();
      change_attack(ai, aj, ti, tj);
      change_defense(ai, aj, bi, bj);
      return  COMPUTE_SCORE;
    }
    popgo();
  }
  return 0;
}



/*

?O..          ?O..
?O..          ?Oc.
X...          Xab.
..*.          ..t.
??.?          ??.?


*/


int
knight_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(-1, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(-2, 0, ci, cj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (TRYMOVE(bi, bj, color)) {
	if (TRYMOVE(ci, cj, other)) {
	  if (attack(ci, cj, NULL, NULL))
	    tval=COMPUTE_SCORE;  
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return (tval);
}



/*

?X?        ?X?         
O*O        Otc
O..        Oab 

*/


int
defend_solid_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(1, 0, ai, aj);
  OFFSET(1, 1, bi, bj);
  OFFSET(0, 1, ci, cj);

  if (strategic_distance_to(other, bi, bj)<8)
    return (0);                                   /* bamboo joint is better */
  if (worm[ci][cj].liberties<3)
    return COMPUTE_SCORE;
  if (TRYMOVE(ti, tj, other)) {
    if (TRYMOVE(ai, aj, color)) {
      if (TRYMOVE(bi, bj, other)) {
	if (!attack(bi, bj, NULL, NULL))
	  tval=COMPUTE_SCORE;
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return (tval);
}

/*

?X?        ?X?         
O.O        ObO 
O.*        Oat

*/


int
defend_bamboo_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(0, -1, ai, aj);
  OFFSET(-1, -1, bi, bj);

  if (strategic_distance_to(other, ti, tj)>10)
    return (0);                                   /* solid connection is better */
  if (TRYMOVE(bi, bj, other)) {
    if (TRYMOVE(ai, aj, color)) {
      if (safe_move(ti, tj, other))
	  tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return (tval);
}

/* Active until the opponent has played his first stone. */

int
high_handicap_helper (ARGS)
{
  int other=OTHER_COLOR(color);
  UNUSED(transformation);
  
  if(!(color_has_played & other)) {
    return COMPUTE_SCORE;
  }
  return -1;
}


/*

O...                Oab.
.*O.                .tO.
.XOX                .XOX
..Xx                ..Xx
----                ----

*/


int 
draw_back_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(-1, 0, ai, aj);
  OFFSET(-1, 1, bi, bj);
  if (TRYMOVE(ti, tj, other)) {
    if (TRYMOVE(ai, aj, color)) {
      if (safe_move(bi, bj, other))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return tval;
}


/*

.O             .O        
X*             at

*/

int 
threaten_helper (ARGS)
{
  int ai, aj;
  int tval=0;

  OFFSET(0, -1, ai, aj);
  if (worm[ai][aj].attacki != -1)
    return (0);
  if (TRYMOVE(ti, tj, color)) {
    if (attack(ai, aj, NULL, NULL))
      tval=COMPUTE_SCORE;
    popgo();
  }
  return tval;
}


/*

.X?     .X?    extend, don't capture
*OX     tOX
OXO	OaO

*/

int 
no_capture_helper (ARGS)
{
  int ai, aj;
  int tval=0;

  OFFSET(1, 1, ai, aj);
  if (worm[ai][aj].liberties==1)
    if (TRYMOVE(ti, tj, color)) {
      if (!find_defense(ai, aj, NULL, NULL)) 
	tval=COMPUTE_SCORE;
      popgo();
    }
  return tval;
}


/*

?o??             ?o??
O.*.             c.t. 
..OX             .bOX
?OX?             ?aX?
?OX?             ?OX?

*/

int 
safe_connection_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);

  OFFSET(2, -1, ai, aj);
  OFFSET(1, -1, bi, bj);
  OFFSET(0, -2, ci, cj);
  if (safe_move(bi, bj, other) && (worm[ai][aj].liberties>2))
    return (min(COMPUTE_SCORE, 
	     connection_value(ai, aj, ci, cj, ti, tj)));
  return (0);
}


/*

?XO            ?XO
?OX            ?Oa
..*            ..t
?O.            ?O.

*/


int 
threaten_capture_helper (ARGS)
{
  int ai, aj;
  int tval=0;

  OFFSET(-1, 0, ai, aj);
  if (worm[ai][aj].attacki != -1)
    return (0);
  if (TRYMOVE(ti, tj, color)) {
    if (attack(ai, aj, NULL, NULL))
      tval=COMPUTE_SCORE;
    popgo();
  }
  return tval;
}


/*

?O.           ?Ob
.X*	      aXt
?O.	      ?Oc

:8,83,0,0,0,0,C,0,wedge_helper

*/

int 
wedge_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int tval=0;
  
  OFFSET(0, -2, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(1, 0, ci, cj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (!p[ai][aj] || attack(ai, aj, NULL, NULL))
	tval=COMPUTE_SCORE;
      else if (TRYMOVE(bi, bj, color)) {
	if (!safe_move(ci, cj, other))
	  tval=COMPUTE_SCORE;
	popgo();
      }
      popgo();
    }
    popgo();
  }
  
  return tval;
}


/*

?O.x          ?gbx
.X*X	      ahtf
?O..	      ?icd
??.?          ??e?

:8,83,0,0,0,0,-,0,wedge2_helper

Playing out the sequence t, a, b, c, d, e leads to

?OOx          ?gbx
XXOX	      ahtf
?OXO	      ?icd
??X?          ??e?

*/

int 
wedge2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj, gi, gj, hi, hj, ii, ij;
  int other=OTHER_COLOR(color);
  int tval=0;
  int connvalue1;
  int connvalue2;
  
  OFFSET(0, -2, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(1, 0, ci, cj);
  OFFSET(1, 1, di, dj);
  OFFSET(2, 0, ei, ej);
  OFFSET(0, 1, fi, fj);
  OFFSET(-1, -1, gi, gj);
  OFFSET(0, -1, hi, hj);
  OFFSET(1, -1, ii, ij);

  connvalue1 = connection_value(gi, gj, ii, ij, ti, tj); /* O stones */
  connvalue2 = connection_value(fi, fj, hi, hj, ti, tj); /* X stones */
  
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (!p[ai][aj] || attack(ai, aj, NULL, NULL))
	tval = max(connvalue1, connvalue2);
      else if (TRYMOVE(bi, bj, color)) {
	if (TRYMOVE(ci, cj, other)) {
	  if (!p[ci][cj] || attack(ci, cj, NULL, NULL))
	    tval = max(connvalue1, connvalue2);
	  else if (TRYMOVE(di, dj, color)) {
	    if (TRYMOVE(ei, ej, other)) {
	      if (!p[fi][fj] || attack(fi, fj, NULL, NULL))
		tval = max(connvalue2, worm[fi][fj].value);
	      popgo();
	    }
	    popgo();
	  }
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return min(tval,COMPUTE_SCORE);
}


/*

..*O        .atO
OOX?	    OOb?

:8,83,0,0,0,0,CDnX,0,weak_connection_helper

*/

int 
weak_connection_helper (ARGS)
{
  int ai, aj, bi, bj;
  int other=OTHER_COLOR(color);
  int tval=0;
  
  OFFSET(0, -1, ai, aj);
  OFFSET(1, 0, bi, bj);

  if (worm[bi][bj].liberties==1)
    tval=COMPUTE_SCORE;
  else if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(ai, aj, NULL, NULL) || attack(bi, bj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }

  return tval;
}




/*

?XO        ?ac
O*X        Otb

*/


int 
sente_cut_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int ccolor;
  int tval=0;

  UNUSED(color);  

  OFFSET(-1, 0, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(-1, 1, ci, cj);

  if ((worm[ci][cj].attacki != -1) && (worm[ci][cj].defendi == -1))
    return (0);

  ccolor=p[ai][aj];
  if (TRYMOVE(ti, tj, ccolor)) {
    if ((attack(ai, aj, NULL, NULL) || attack(bi, bj, NULL, NULL)))
      tval=COMPUTE_SCORE;
    popgo();
  }
  return tval;
}


int
wide_break_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int tval;
  int other=OTHER_COLOR(color);

  TRACE("in wide_break_helper at %m\n", ti, tj);
  OFFSET(-1, 0, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(-1, 2, ci, cj);
  OFFSET(0, 1, di, dj);
  OFFSET(0, 2, ei, ej);

  tval=min(COMPUTE_SCORE, connection_value(ai, aj, ei, ej, ti, tj));

  if (tval)
    if (TRYMOVE(ti, tj, color)) {
      if (TRYMOVE(bi, bj, other)) {
	if (TRYMOVE(di, dj, color)) {
	  if ((p[ci][cj]==EMPTY) || (attack(ci, cj, NULL, NULL)) ||
	      (p[ti][tj]==EMPTY) || (attack(ti, tj, NULL, NULL)))
	    tval=0;
	  popgo();
	}
	popgo();
      }
      popgo();
    }
  if (tval)
    if (TRYMOVE(ti, tj, color)) {
      if (TRYMOVE(di, dj, other)) {
	if (TRYMOVE(bi, bj, color)) {
	  if ((p[ci][cj]==EMPTY) || (attack(ci, cj, NULL, NULL)) ||
	      (p[ti][tj]==EMPTY) || (attack(ti, tj, NULL, NULL)))
	    tval=0;
	  popgo();
	}
	popgo();
      }
      popgo();
    }
  return tval;
}


/*

?.?     ?.?        b=sole liberty of a
X*X     cta
?.?     ?.?

*/

int
double_attack2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int tval=0;
  OFFSET(0, 1, ai, aj);
  OFFSET(0, -1, ci, cj);

  if ((approxlib(ai, aj, p[ai][aj], 3)>2) || (worm[ai][aj].attacki != -1) ||
      (worm[ci][cj].attacki != -1))
    return (0);

  bi=libi[0];
  bj=libj[0];
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (attack(ci, cj, NULL, NULL))
	tval=min(COMPUTE_SCORE, 
		 min(worm[ai][aj].value, worm[ci][cj].value));
      popgo();
    }
    popgo();
  }
  return tval;
}


/*

If there are two ways of making atari on the X string,
chosing the wrong one is aji keshi. But often the other
atari isn't possible. In that case, make the atari that
works as long as it is sente.

?.              ?.
.*              ?t       c = other liberty of X
OX              ab

*/

int
only_atari_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int tval=0;
  OFFSET(1, -1, ai, aj);
  OFFSET(1, 0, bi, bj);

  if (((worm[ai][aj].liberties)>1) && (worm[bi][bj].liberties==2)) {
    approxlib(bi, bj, other, 3);
    if ((libi[0]!=ti) || (libj[0]!=tj)) 
      {
	ci=libi[0];
	cj=libj[0];
      }
    else
      {
	ci=libi[1];
	cj=libj[1];
      }
    if (approxlib(ci, cj, color, 2)==1)
      tval=min(COMPUTE_SCORE, worm[ai][aj].value);
  }
  return tval;
}

/*

?.?            ?.?
X*.            at.
OX.            db.
X..            c..
?..            ?..

*/


int
setup_double_atari_helper (ARGS)
{
  int ai, aj, ci, cj, di, dj;
  int tval=0;
  OFFSET(0, -1, ai, aj);
  OFFSET(2, -1, ci, cj);
  OFFSET(1, -1, di, dj);

  if ((worm[di][dj].liberties > 1) && (worm[ci][cj].liberties==2))
    if (TRYMOVE(ti, tj, color)) {
      if (p[ai][aj] && attack(ai, aj, NULL, NULL))
	tval=min(COMPUTE_SCORE, worm[ai][aj].value);
      popgo();
    }
  return tval;
}


/*

?X?          ?X?
O.x          cax
.*O          btd
x?x          x?x

*/

int
urgent_connect_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(-1, 0, ai, aj);
  OFFSET(0, -1, bi, bj);
  if (TRYMOVE(ai, aj, other)) {
    if (TRYMOVE(ti, tj, color)) {
      if (TRYMOVE(bi, bj, other)) {
	if (p[ai][aj] && !attack(ai, aj, NULL, NULL) &&
	    !attack(bi, bj, NULL, NULL)) {
	  OFFSET(-1, -1, ci, cj);
	  OFFSET(0, 1, di, dj);
	  if ((p[ci][cj]==EMPTY) || (p[di][dj]==EMPTY))
	    tval=COMPUTE_SCORE;
	  else
	    tval=min(COMPUTE_SCORE, connection_value(ci, cj, di, dj, ti, tj));
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return tval;
}
      
int
urgent_connect2_helper (ARGS)
{
  int ai, aj;
  OFFSET(1, 0, ai, aj);
  return urgent_connect_helper(pattern, transformation, ai, aj, color);
}

/*

*OX      tab
?XO      ?cd

or

*XO      tab
?OX      ?cd

*/

int
detect_trap_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int ui, uj;
  int ocolor;

  UNUSED(pattern);
  UNUSED(color);
  
  OFFSET(0, 1, ai, aj);
  OFFSET(0, 2, bi, bj);
  OFFSET(1, 1, ci, cj);
  OFFSET(1, 2, di, dj);
  ocolor=p[ai][aj];

  if ((worm[bi][bj].attacki != -1) || (worm[ci][cj].attacki != -1) || 
      (worm[di][dj].liberties==1) || 
      ((dragon[bi][bj].origini==dragon[ci][cj].origini) &&
       (dragon[bi][bj].originj==dragon[ci][cj].originj)))
    return (0);

  if (TRYMOVE(ti, tj, ocolor)) {
    if (!attack(ti, tj, NULL, NULL) && attack(ci, cj, NULL, NULL) 
	&& attack(bi, bj, NULL, NULL) && (find_defense(bi, bj, &ui, &uj)))
      {
	popgo();
	if (worm[ai][aj].attacki != -1)
	  change_defense(bi, bj, worm[ai][aj].attacki, worm[ai][aj].attackj);
	else
	  change_defense(bi, bj, ui, uj);
	return min(COMPUTE_SCORE, min(worm[bi][bj].value, worm[ci][cj].value));
      }
    popgo();
  }
  return 0;
}


/*

?O*X     ?dtb
o.XO     o.Xc
?O.X	 ?OaX

*/

int
double_threat_connection_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int tval = 0;
  int other = OTHER_COLOR(color);

  OFFSET(2, 0, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(1, 1, ci, cj);
  OFFSET(0, -1, di, dj);

  if (TRYMOVE(ti, tj, color)) {
    if (!attack(ci, cj, NULL, NULL)) {
      if (TRYMOVE(ai, aj, other)) {
	if (!p[bi][bj] || attack(bi, bj, NULL, NULL)) {
	  tval = min(COMPUTE_SCORE, connection_value(ci, cj, di, dj, ti, tj)+5);
	}
	popgo();
      }
    } 
    popgo();
  }
  return tval;
}


/*
  
|.XO     |.bO             sacrifice a second stone tesuji
|*OX	 |tOa
|.XO	 |.cO

*/

int
sacrifice_another_stone_tesuji_helper(ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int tval = 0;

  OFFSET(0, 2, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(1, 1, ci, cj);

  if (TRYMOVE(ti, tj, color)) {
    if (attack(ai, aj, NULL, NULL) &&
	(attack(bi, bj, NULL, NULL) || attack(ci, cj, NULL, NULL))) {
      tval = COMPUTE_SCORE;
    }
    popgo();
  }
  return tval;
}


/*
  
XXO      XXc            decrease eye space in sente (unless it kills)
.*X      eta
---      ---

or

XXO      XXc            decrease eye space in sente (unless it kills)
.*X      eta
XXO      XXd

or

|XXO     |XXc           decrease eye space in sente (unless it kills)
|.*X     |eta
|XXO     |XXd

or

|XXO     |XXc           decrease eye space in sente (unless it kills)
|.*X     |eta
+---     +---

*/

int
throw_in_atari_helper(ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int tval=0;
  int other=OTHER_COLOR(color);
  
  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 1, ci, cj);
  OFFSET(1, 1, di, dj);
  OFFSET(0, -1, ei, ej);

  /* Find second liberty of the stone a. */
  approxlib(ai, aj, other, 2);
  if ((libi[0]!=ti) || (libj[0]!=tj)) {
    bi=libi[0];
    bj=libj[0];
  }
  else {
    bi=libi[1];
    bj=libj[1];
  }
  
  if (TRYMOVE(ti, tj, color)) {
    if (!attack(ci, cj, NULL, NULL) &&
	!(ON_BOARD(di, dj) && attack(di, dj, NULL, NULL))) {
      if (TRYMOVE(bi, bj, other)) {
	if (attack(ai, aj, NULL, NULL))
	  tval = COMPUTE_SCORE;
	popgo();
      }
      else {
	tval = COMPUTE_SCORE; /* X move at bi,bj would have been suicide */
      }
    }
    popgo();
  }
  
  /* No more than partial credit if only one stone in atari, as this
   * leaves a potential ko.
   */
  if (worm[ai][aj].size == 1)
    tval -= 20;

  /* Also reduce value if the stone at e might make a difference on
   * the outside.
   */

  if (other==BLACK) 
    {
      if ((black_eye[ei][ej].marginal) ||
	  (black_eye[ei][ej].marginal_neighbors - black_eye[ti][tj].marginal > 0))
	tval -=20;
    }
  else if (white_eye[ei][ej].marginal ||
      (white_eye[ei][ej].marginal_neighbors - white_eye[ti][tj].marginal > 0))
    tval -=20;

  /* Discard move completely if it's unlikely that capturing the
   * stones would make any difference. (Rather crude heuristics.) */
  if (worm[ai][aj].size <= 2) {
    /* Don't consider the edge cases, being nondead is specified in
     * the pattern for those.
     */
    if (ON_BOARD(di, dj))
      if (dragon[ci][cj].status == DEAD && dragon[di][dj].status == DEAD) 
	if (connection_value(ci, cj, di, dj, ti, tj) == 0)
	  tval = 0;
  }
  return tval;
}


/*

.*X     .tX
XO.	aOb
XXO	XXO

*/

int
sente_extend_helper (ARGS)
{
  int ai, aj, bi, bj;
  int other=OTHER_COLOR(color);
  int tval=0;

  OFFSET(1, -1, ai, aj);
  OFFSET(1, 1, bi, bj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (attack(ai, aj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return tval;
}

/*

??x          cde
O*O          atb
??x          fgh

*/

int
straight_connect_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj, gi, gj, hi, hj;
  int other=OTHER_COLOR(color);
  int nocut=0;

  OFFSET(-1, -1, ci, cj);
  OFFSET(-1, 0, di, dj);
  OFFSET(-1, 1, ei, ej);
  OFFSET(0, -1, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(1, -1, fi, fj);
  OFFSET(1, 0, gi, gj);
  OFFSET(1, 1, hi, hj);

  if ((!(dragon[ai][aj].status == DEAD) && !worm[bi][bj].ko && 
      ((dragon[bi][bj].status == DEAD) || (dragon[bi][bj].status == CRITICAL))) || 
      (!(dragon[bi][bj].status == DEAD) && !worm[ai][aj].ko && 
       ((dragon[ai][aj].status == DEAD) || (dragon[ai][aj].status == CRITICAL)))) {
    
    /* avoid pattern if you can make a bamboo joint */

    if ((worm[ci][cj].color == color) && (worm[di][dj].color != other) &&
	(worm[ei][ej].color != other))
      return (0);
    if ((worm[fi][fj].color == color) && (worm[gi][gj].color != other) &&
	(worm[hi][hj].color != other))
      return (0);

    /* avoid pattern if X can't cut */

    if (approxlib(ti, tj, other, 2)==1)
      return (0);

    if ((worm[ci][cj].color != other) && (worm[di][dj].color != other) &&
	(worm[ei][ej].color != other)
	&& (worm[gi][gj].color != other))
      if (TRYMOVE(ti, tj, other)) {
	if (TRYMOVE(di, dj, color)) {
	  if (TRYMOVE(gi, gj, other)) {
	    if (TRYMOVE(ci, cj, color)) {
	      if (TRYMOVE(ei, ej, other)) {
		if (attack(ei, ej, NULL, NULL))
		  nocut=1;
		popgo();
	      }
	      popgo();
	    }
	    if (!nocut && TRYMOVE(ei, ej, color)) {
	      if (TRYMOVE(ci, cj, other)) {
		if (attack(ci, cj, NULL, NULL))
		  nocut=1;
		popgo();
	      }
	      popgo();
	    }
	    popgo();
	  }
	  popgo();
	}
	popgo();
      }

    if (! nocut && (worm[fi][fj].color != other) &&
	(worm[gi][gj].color != other) && (worm[hi][hj].color != other)
	&& (worm[di][di].color != other))
      if (TRYMOVE(ti, tj, other)) {
	if (TRYMOVE(gi, gj, color)) {
	  if (TRYMOVE(di, dj, other)) {
	    if (TRYMOVE(fi, fj, color)) {
	      if (TRYMOVE(hi, hj, other)) {
		if (attack(hi, hj, NULL, NULL))
		  nocut=1;
		popgo();
	      }
	      popgo();
	    }
	    if (!nocut && TRYMOVE(hi, hj, color)) {
	      if (TRYMOVE(fi, fj, other)) {
		if (attack(fi, fj, NULL, NULL))
		  nocut=1;
		popgo();
	      }
	      popgo();
	    }
	    popgo();
	  }
	  popgo();
	}
	popgo();
      }
      
    if (nocut)
      return (0);

    return min(connection_value(ai, aj, bi, bj, ti, tj), COMPUTE_SCORE);
  }
  return (0);
}


/*

|.X           |.c
|XO           |Xd
|*.           |tb
|.X           |.a

*/

int
block_to_kill_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int tval=0;

  OFFSET(1, 1, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(-2, 1, ci, cj);
  OFFSET(-1, 1, di, dj);

  if (TRYMOVE(ti, tj, color)) {
    if (!attack(di, dj, NULL, NULL))
      tval=min(connection_value(ai, aj, ci, cj, ti, tj), COMPUTE_SCORE);
    if (attack(ai, aj, NULL, NULL) && !find_defense(ai, aj, NULL, NULL)) {
      popgo();
      change_attack(ai, aj, ti, tj);
    }
    else
      popgo();
  }
  return tval;
}


/*

*OX        tab
.XO        .XO

*/

int
extend_to_kill_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;
  
  OFFSET (0, 1, ai, aj);
  OFFSET (0, 2, bi, bj);

  if ((worm[bi][bj].defendi != -1) && TRYMOVE(ti, tj, color)) {
    if (!p[bi][bj] || !find_defense(bi, bj, NULL, NULL)) {
      tval = COMPUTE_SCORE;
    }
    popgo();
  }
  if (tval) {
    if (worm[bi][bj].attacki != -1)
      change_attack(bi, bj, ti, tj);
    if (worm[ai][aj].defendi != -1)
      change_defense(ai, aj, ti, tj);
  }
  return tval;
}


/*

?O.               ?Oa
*XO               tcO
?O.               ?Ob

The purpose of this helper is to revise the value of capturing X.
In some cases X is mistakenly perceived as a cutting worm, when
in fact the O worms are effectively already connected. In this
case capturing it is given a very high value. This can lead to 
mistakes in the opening.

FIXME: although the revision of the values comes in time to
inform attacker and defender, it may or may not come in time
to inform diag_miai_helper. Perhaps one day we will want to
run shapes twice, and this helper (together with helpers
of type D) will go in the first run, while anything
involving connection_value will go in the second. 

*/


int
revise_value_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int cuta=0, cutb=0;

  OFFSET(-1, 2, ai, aj);
  OFFSET(1, 2, bi, bj);
  OFFSET(0, 1, ci, cj);
  UNUSED(pattern);

  if (TRYMOVE(ti, tj, other)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(ai, aj, NULL, NULL))
	cuta=1;
      popgo();
    }
    else 
      cuta=2;
    if (TRYMOVE(bi, bj, other)) {
      if (attack(bi, bj, NULL, NULL))
	cutb=1;
      popgo();
    }
    else 
      cutb=2;
    if (((cuta==2) && cutb) || ((cutb==2) && cuta)) {
      worm[ci][cj].cutstone=0;
      worm[ci][cj].value=75;
    } 
    else if ((cuta) && (cutb)) {
      worm[ci][cj].cutstone=1;
      worm[ci][cj].value=78;
    }
    popgo();
  }
  return 0;
}

    
/*

?oOOXX?              ?oOaXX?         
oo.XO*.              oo.bct.
oo....?              oo....?
-------              -------

*/


int
push_to_capture_helper(ARGS)
{
  int ai, aj, bi, bj, ci, cj;

  OFFSET(-1, -2, ai, aj);
  OFFSET(0, -2, bi, bj);
  OFFSET(0, -1, ci, cj);

  if (worm[ai][aj].liberties>3) {
    change_attack(bi, bj, ti, tj);
    change_defense(ci, cj, ti, tj);
    return (COMPUTE_SCORE);
  }
  return 0;
}


/*

This pattern can't be handled using the half
eye database. The reason is a limitation of the 'o'
classification: the test that an X move at such a
location is captured is tried BEFORE the * move is
put on the board.

?..O         ?.aO
?*.O         ?t.b
?.O?         ?.O?

:8,100,0,-,0,0,0,special_half_eye_helper

*/

int
special_half_eye_helper(ARGS)
{
  int ai, aj, bi, bj;
  int true_genus;
  int other=OTHER_COLOR(color);
  int tval=0;

  OFFSET(-1, 1, ai, aj);
  OFFSET(0, 2, bi, bj);

  true_genus = 2*dragon[bi][bj].genus+dragon[bi][bj].heyes;
  
  if ((true_genus>0) && (true_genus<4) &&
      dragon[bi][bj].escape_route<6 && TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(ai, aj, NULL, NULL))
	tval=min(dragon[bi][bj].value, COMPUTE_SCORE);
      popgo();
    }
    popgo();
  }
  return tval;
}
    

/*

?X        ?a
?O        ?b
X*        Xt
O.        O.

*/


int
block_attack_defend_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;

  OFFSET(-2, 0, ai, aj);
  OFFSET(-1, 0, bi, bj);

  if (worm[ai][aj].defendi == -1 || worm[bi][bj].defendi == -1)
    return 0;

  if (TRYMOVE(ti, tj, color)) {
    if (p[ai][aj] && !find_defense(ai, aj, NULL, NULL))
      if (p[bi][bj] && !attack(bi, bj, NULL, NULL)) {
	tval=COMPUTE_SCORE;
      }
    popgo();
  }
  if (tval) {
    change_attack(ai, aj, ti, tj);
    change_defense(bi, bj, ti, tj);
  }

  return tval;
}



/*

----                ----
....                ....
X...                ab..
O*.O                Ot.O

*/

int
pull_back_catch_helper (ARGS)
{
  int ai, aj, bi, bj;
  int tval=0;

  OFFSET(-1, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);

  if (worm[ai][aj].liberties==2)
    return 0;
  
  if (dragon[ai][aj].safety==CRITICAL)
    tval=min(COMPUTE_SCORE, 70);
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, color)) {
      if (attack(ai, aj, NULL, NULL) && !find_defense(ai, aj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return tval;
}




/*

OO?              OO?
.*X              .*a
...              ...
---              ---

*/

int
second_line_block_helper (ARGS)
{
  int ai, aj;
  UNUSED(pattern);

  OFFSET(0, 1, ai, aj);
  if ((dragon[ai][aj].status==UNKNOWN) 
      && (worm[ai][aj].attacki==-1) && (dragon[ai][aj].safety==CRITICAL))
    return COMPUTE_SCORE;
  return 0;
}


/*

Prevent misreporting of a as lunch for b

XO|          ba|
O*|          O*|
oo|          oo|
?o|          ?o|

*/

int
not_lunch_helper (ARGS)
{
  int ai, aj, bi, bj;
  int m, n;

  OFFSET(-1, 0, ai, aj);
  OFFSET(-1, -1, bi, bj);
  UNUSED(pattern);
  UNUSED(color);

  if (worm[ai][aj].size>2)
    return 0;

  if ((dragon[bi][bj].lunchi==ai) && (dragon[bi][bj].lunchj==aj))
    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++)
	if ((dragon[m][n].origini==dragon[bi][bj].origini) &&
	    (dragon[m][n].originj==dragon[bi][bj].originj))
	  dragon[m][n].lunchi=-1;
  return 0;
}
  

/*

|...X..o                 |...X..o
|.....oo                 |..cbdoo
|..*..oo                 |..ta.oo
|....ooo                 |....ooo
|..X.ooo                 |..X.ooo
                 
Check ladder relationship. This helper needs to be rewritten
to check both ladder relationships.

*/

int
invade3sp_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(-1, 0, ci, cj);
  OFFSET(-1, 2, di, dj);

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (TRYMOVE(bi, bj, color)) { 
	if (TRYMOVE(ci, cj, other)) { 
	  if (TRYMOVE(di, dj, color)) {
	    if (attack(ai, aj, NULL, NULL))
	      tval=COMPUTE_SCORE;
	    popgo();
	  }
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();
  }
  return tval;
}




/*

?OO                 ?OO
O.O                 O.b
.*.                 at.
---                 ---

overturn spurious attack found by find_cap2

*/


int
indirect_helper (ARGS)
{
  int ai, aj, bi, bj;

  OFFSET(0, -1, ai, aj);
  OFFSET(-1, 1, bi, bj);
  UNUSED(pattern);
  UNUSED(color);

  if ((worm[bi][bj].attacki==ti) && (worm[bi][bj].attackj==tj)
      && (worm[bi][bj].defendi == -1) && (does_defend(ai, aj, bi, bj)))
    change_attack(bi, bj, -1, -1);
  return 0;
}


  
/*

OOo                 Obo
X*.                 a*c
...                 ...
___                 ___


*/


int
dont_attack_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);
  int dont_attack=0;

  OFFSET(0, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(0, 1, ci, cj);
  UNUSED(pattern);

  if ((worm[ai][aj].defendi==ti) && (worm[ai][aj].defendj==tj) 
      && (worm[ai][aj].liberties==3))
    if (TRYMOVE(ti, tj, other)) {
      if (TRYMOVE(ci, cj, color)) {
	if (attack(ai, aj, NULL, NULL) && (!find_defense(ai, aj, NULL, NULL)))
	  dont_attack=1;
	popgo();
      }
      popgo();
    }
  if (dont_attack) 
    change_defense(ai, aj, -1, -1);

  return 0;
}
    
      

/*

??d?                    ??d?
OOX.                    OOab
.X*.                    .ct.
??.?                    ??.?

*/

int
atari_ladder_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int other=OTHER_COLOR(color);
  int tval=0;
  
  OFFSET(-1, 0, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(0, -1, ci, cj);
  OFFSET(-2, 0, di, dj);

  if (dragon[di][dj].status != DEAD)
    return 0;
  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(bi, bj, other)) {
      if (attack(ci, cj, NULL, NULL))
	tval=COMPUTE_SCORE;
      popgo();
    }
    popgo();
  }
  return tval;
}



/*

XO               cb
*.               ta
.X               .d

*/

int
safe_cut_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int other=OTHER_COLOR(color);
  int tval=0;

  UNUSED(pattern);

  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(-1, 0, ci, cj);
  
  /* after cutting, the condition is that O can save either t or b and
     the other can't be captured. */

  if ((worm[bi][bj].liberties==1) || (worm[ci][cj].liberties==1))
    return 0;

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if ((!attack(bi, bj, NULL, NULL) 
	  && (!attack(ti, tj, NULL, NULL) || find_defense(ti, tj, NULL, NULL)))
	  || (find_defense(bi, bj, NULL, NULL) && (!attack(ti, tj, NULL, NULL)))) {
	OFFSET(1, 1, di, dj);

	tval=min(connection_value(ci, cj, di, dj, ti, tj), COMPUTE_SCORE);
      }
      popgo();
    }
    popgo();
  }
  return tval;
}


/*

To check whether * works, we have to try two ways
for x to defend the cut.

..X?                  ..f?
.*..                  .tad
?X.O                  ?ebc

*/

int
waist_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej, fi, fj;
  int other=OTHER_COLOR(color);
  int tval;

  UNUSED(pattern);

  OFFSET(0, 1, ai, aj);
  OFFSET(1, 1, bi, bj);
  OFFSET(1, 2, ci, cj);
  OFFSET(1, 0, ei, ej);
  OFFSET(-1, 1, fi, fj);

  if (worm[ci][cj].liberties==2)
    return 0;

  tval=min(connection_value(ei, ej, fi, fj, ti, tj), COMPUTE_SCORE);
  if (tval==0)
    return 0;

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (TRYMOVE(bi, bj, color)) {
	if (attack(bi, bj, NULL, NULL) || attack(ti, tj, NULL, NULL))
	  tval=0;
	popgo();
      }
      popgo();
    }
    if (tval==0) {
      popgo();
      return 0;
    }
    OFFSET(0, 2, di, dj);
    if (TRYMOVE(bi, bj, other)) {
      if (TRYMOVE(ai, aj, color)) {
	if (TRYMOVE(di, dj, other)) {
	  if ((attack(ci, cj, NULL, NULL) 
	       || (attack(ti, tj, NULL, NULL) && !find_defense(ti, tj, NULL, NULL)))
	      && (!find_defense(ci, cj, NULL, NULL) || (attack(ti, tj, NULL, NULL))))
	    tval=0;	    /* cut fails because there's no way to save both t and c */	  
	  popgo();
	}
	popgo();
      }
      popgo();
    }
    popgo();  
  }
  return tval;
}




/* This is intended for use in autohelpers. */
/* Check whether the string at (ai, aj) can attack any surrounding
 * string. If so, return false as the move to create a seki (probably)
 * wouldn't work.
 */
int 
seki_helper(int ai, int aj)
{
  int r;
  int adj;
  int adji[MAXCHAIN];
  int adjj[MAXCHAIN];
  int adjsize[MAXCHAIN];
  int adjlib[MAXCHAIN];
  
  chainlinks(ai, aj, &adj, adji, adjj, adjsize, adjlib);
  for (r=0; r<adj; r++)
    if (worm[adji[r]][adjj[r]].attacki != -1)
      return 0;
  return 1;
}


/*

XO     aO
O*     O*

Used in a connection pattern to ensure that X is a cutstone.
*/

int 
ugly_cutstone_helper (ARGS)
{
  int ai, aj;
  UNUSED(pattern);
  UNUSED(color);
  
  OFFSET(-1, -1, ai, aj);

  worm[ai][aj].cutstone++;
  return 0;
}




/*

OX              bX
*O              tO
.X              dX
O?              c?

*/


int
real_connection_helper (ARGS)
{
  int bi, bj, ci, cj, di, dj;
  int tval=0;
  int other=OTHER_COLOR(color);

  OFFSET(-1, 0, bi, bj);
  OFFSET(2, 0, ci, cj);
  OFFSET(1, 0, di, dj);
  
  if (TRYMOVE(ti, tj, color)) {
    if (!safe_move(di, dj, other))
      tval=min(COMPUTE_SCORE, connection_value(bi, bj, ci, cj, ti, tj));
    popgo();
  }
  return tval;
}


/*

XO          Xb
O*          at
..          ..
--          --

Here a can be captured, and b is CRITICAL. The helper artificially assigns a 
genus 1 so that eye_finder will try the connection as a way of saving b. Since
this is a C pattern, the dragons at a and b become friends.

*/

int
connect_to_live_helper (ARGS)
{
  int ai, aj, bi, bj;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);

  if ((worm[ai][aj].attacki != -1) 
      && (dragon[ai][aj].genus == 0) 
      && (dragon[bi][bj].status == CRITICAL)) {
    if (dragon[ai][aj].size==1)
      dragon[ai][aj].genus=1;
    else {
      int m, n;
      for (m=0; m<board_size; m++)
	for (n=0; n<board_size; n++)
	  if ((dragon[m][n].origini==dragon[ai][aj].origini)
	      && (dragon[m][n].originj==dragon[ai][aj].originj))
	    dragon[m][n].genus=1;
    }
  }
  return 0;
}


/* 

This D type pattern provides a better defense in case the reading
code recommends something stupid!

*.o?              t.o?
.O.O              .Oab
?xOO              ?xOO

*/

int
stupid_defense_helper (ARGS)
{
  int ai, aj, bi, bj;

  UNUSED(pattern);

  OFFSET(1, 2, ai, aj);
  OFFSET(1, 3, bi, bj);

  if ((worm[bi][bj].defendi == ai)
      && (worm[bi][bj].defendj == aj)
      && TRYMOVE(ti, tj, color)) {
    if (!attack(bi, bj, NULL, NULL)) {
      popgo();
      change_defense(bi, bj, ti, tj);
      return COMPUTE_SCORE;
    }
    popgo();
  }
  return 0;
}
	
/*

.*O             atd
OX?             cb?


*/

int
threaten_connect_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int other=OTHER_COLOR(color);

  OFFSET(0, -1, ai, aj);
  OFFSET(1, 0, bi, bj);
  OFFSET(1, -1, ci, cj);
  OFFSET(0, 1, di, dj);

  if ((worm[bi][bj].attacki != -1) 
      || (worm[ci][cj].liberties == 1))
    return 0;

  if ((dragon[ci][cj].origini == dragon[di][dj].origini)
      && (dragon[ci][cj].originj == dragon[di][dj].originj))
    return 0;

  if (dragon[ci][cj].safety != ALIVE)
    return COMPUTE_SCORE;

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(ai, aj, NULL, NULL) || attack(bi, bj, NULL, NULL)) {
	popgo();
	popgo();
	return COMPUTE_SCORE;
      }
      popgo();
    }
    popgo();
  }
  return 0;
}


/*

*.O             taO
OX?             cb?

*/

int
threaten_connect1_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int other=OTHER_COLOR(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(1, 1, bi, bj);
  OFFSET(1, 0, ci, cj);

  if (worm[bi][bj].attacki != -1)
    return 0;

  if ((dragon[ci][cj].safety != ALIVE) && TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (!attack(ai, aj, NULL, NULL) && !attack(bi, bj, NULL, NULL)) {
	popgo();
	popgo();
	return COMPUTE_SCORE;
      }
      popgo();
    }
    popgo();
    return 0;
  }
  return 0;
}


/*

This pattern tries to supply eyes which are missed by compute_eyes.

OO.              OOb
O.*              cat
XOO              XdO

*/

int
make_eye_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj;
  int ocolor, xcolor;
  
  UNUSED(pattern);
  UNUSED(color);
  
  OFFSET(0, -1, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(0, -2, ci, cj);
  OFFSET(1, -1, di, dj);

  ocolor=p[di][dj];
  xcolor=OTHER_COLOR(ocolor);

  if (((ocolor==WHITE) && (white_eye[ai][aj].maxeye != 0))
      || ((ocolor==BLACK) && (black_eye[ai][aj].maxeye != 0)))
    return 0;

  if (TRYMOVE(ti, tj, ocolor)) {
    if (!safe_move(bi, bj, xcolor)) {
      popgo();
      retrofit_half_eye(ti, tj, ci, cj);
      return COMPUTE_SCORE;
    }
    popgo();
  }
  return 0;
}

      
/*

This pattern tries to supply eyes which are missed by compute_eyes.
Here the topological eye criterion can erroneously classify the
eye at d as FALSE. If it is actually a half eye, we must make
adjustments.

.OX                 bOX
*.O                 tde
.Oo                 cOa

*/

int
make_eye1_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int ocolor, xcolor;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(1, 2, ai, aj);
  OFFSET(-1, 0, bi, bj);
  OFFSET(1, 0, ci, cj);
  OFFSET(0, 1, di, dj);
  OFFSET(0, 2, ei, ej);

  ocolor=p[ei][ej];
  xcolor=OTHER_COLOR(ocolor);

  if ((ocolor==WHITE) && white_eye[di][dj].maxeye>0)
    return 0;
  if ((ocolor==BLACK) && black_eye[di][dj].maxeye>0)
    return 0;

  if ((p[ai][aj]==ocolor) || !safe_move(ai, aj, xcolor))
    if (TRYMOVE(ti, tj, ocolor)) {
      if (!safe_move(bi, bj, xcolor) && !safe_move(ci, cj, xcolor)) {
	popgo();
	retrofit_half_eye(ti, tj, ei, ej);
	return COMPUTE_SCORE;
      }
      else popgo();
    }
  return 0;
}


/*

This should be classified as

 .
.X!

but sometimes it is not recognised as O eyespace at all. The
helper tries to correct this.

oOo?                dbe?
O.OX                O.OX
.X*.                act.
----                ----

*/

int
make_eye2_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int ocolor, xcolor;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, -2, ai, aj);
  OFFSET(-2, -1, bi, bj);
  OFFSET(0, -1, ci, cj);
  OFFSET(-2, -2, di, dj);
  OFFSET(-2, 0, ei, ej);

  ocolor=p[bi][bj];
  xcolor=OTHER_COLOR(ocolor);

  if ((p[di][dj] == EMPTY) && safe_move(di, dj, xcolor))
    return 0;

  if ((p[ei][ej] == EMPTY) && safe_move(ei, ej, xcolor))
    return 0;

  if (((ocolor == WHITE) && (white_eye[ci][cj].origini == -1))
      || ((ocolor == BLACK) && (black_eye[ci][cj].origini == -1)))
    if (!safe_move(ai, aj, xcolor))
      retrofit_half_eye(ti, tj, bi, bj);
  return 0;
}


/*

oO*ooo?               oatooo?
O...ooo               bc..ooo
-------               -------

*/

int
make_eye3_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int ocolor;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, -1, ai, aj);
  OFFSET(1, -2, bi, bj);
  OFFSET(1, -1, ci, cj);

  ocolor=p[ai][aj];
  if (((ocolor == WHITE) 
       && (white_eye[ci][cj].esize == 3)
       && (white_eye[ci][cj].marginal_neighbors == 2))
      ||((ocolor == BLACK) 
	 && (black_eye[ci][cj].esize == 3)
	 && (black_eye[ci][cj].marginal_neighbors == 2))) {
    retrofit_half_eye(ti, tj, ai, aj);
    return COMPUTE_SCORE;
  }
  return 0;
}



/*

If it is possible to hane in this shape, eye_finder frequently
underestimates the eye potential. This pattern adds a half eye
to try to live.


xXO.             xXab
.*..             .t..
....             ....
----             ----

*/

int
expand_eyespace_helper (ARGS)
{
  int ai, aj, bi, bj;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(-1, 1, ai, aj);
  OFFSET(-1, 2, bi, bj);
  
  if ((dragon[ai][aj].genus == 1) 
      && (dragon[ai][aj].heyes == 0)
      && (((p[ai][aj]==WHITE) 
	   && (white_eye[bi][bj].dragoni == dragon[ai][aj].origini)
	   && (white_eye[bi][bj].dragoni == dragon[ai][aj].origini)
	   && (white_eye[bi][bj].marginal == 0))
	  ||((p[ai][aj]==BLACK) 
	     && (black_eye[bi][bj].dragoni == dragon[ai][aj].origini)
	     && (black_eye[bi][bj].dragoni == dragon[ai][aj].origini)
	     && (black_eye[bi][bj].marginal == 0))))
    retrofit_half_eye(ti, tj, ai, aj);
  return 0;
}


/*

X???            X???
.OO?            .bO?
...X            a..X
.*.?            .t.?

The cap is overlooked by the reading code.

*/

int
cap_kills_helper (ARGS)
{
  int ai, aj, bi, bj;
  int other=OTHER_COLOR(color);

  OFFSET(-2, 0, bi, bj);
  OFFSET(-1, -1, ai, aj);

  if ((worm[bi][bj].liberties>3) || (worm[bi][bj].attack_code==1))
    return 0;

  if (TRYMOVE(ai, aj, other)) {
    if (attack(bi, bj, NULL, NULL) && !find_defense(bi, bj, NULL, NULL)) {
      popgo();
      return COMPUTE_SCORE;
    }
    popgo();
  }
  return 0;
}


/*

?O.             ?Ob
X*O             Xta
?O.             ?Oc

*/


int
chimera_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int m, n;
  int ocolor;
  int status;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, 1, ai, aj);
  OFFSET(-1, 1, bi, bj);
  OFFSET(1, 1, ci, cj);

  ocolor=p[ai][aj];

  if ((dragon[ai][aj].heyes == 2)
      && (dragon[ai][aj].genus == 0)
      && (half_eye[bi][bj].type == HALF_EYE)
      && (half_eye[bi][bj].ki == ti)
      && (half_eye[bi][bj].kj == tj)
      && (half_eye[ci][cj].type == HALF_EYE)
      && (half_eye[ci][cj].ki == ti)
      && (half_eye[ci][cj].kj == tj)) {
    
    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++)
	if ((dragon[m][n].origini == dragon[ai][aj].origini)
	    && (dragon[m][n].originj == dragon[ai][aj].originj)) {
	  dragon[m][n].heyes = 1;
	  dragon[m][n].genus = 1;
	  dragon[m][n].heyei = ti;
	  dragon[m][n].heyej = tj;
	}
    status=dragon_status(ai, aj);
    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++)
	if ((dragon[m][n].origini == dragon[ai][aj].origini)
	    && (dragon[m][n].originj == dragon[ai][aj].originj))
	  dragon[m][n].status = status;

  }
  return 0;
}


/*

--+
*.|
O.|
?O|

*/

int
semimarginal_helper (ARGS)
{
  int ai, aj;
  int ocolor;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(1, 0, ai, aj);
  ocolor = p[ai][aj];

  if ((((ocolor==WHITE)
	&& (white_eye[ti][tj].esize==3)
	&& (white_eye[ti][tj].msize==1)
	&& (white_eye[ti][tj].marginal))
       ||
       ((ocolor==BLACK)
	&& (black_eye[ti][tj].esize==3)
	&& (black_eye[ti][tj].msize==1)
	&& (black_eye[ti][tj].marginal)))
      && (dragon[ai][aj].genus > 0)) {
    retrofit_half_eye(ti, tj, ai, aj);
    retrofit_genus(ai, aj, -1);
  }
  return 0;
}


/*

.O.O               cb.O
O.X*               O.at
----               ----

*/

int
eye_on_edge_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj;
  int ocolor;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(0, -1, ai, aj);
  OFFSET(-1, -2, bi, bj);
  OFFSET(-1, -3, ci, cj);
  ocolor=p[bi][bj];

  if ((worm[ai][aj].attacki == -1) 
      || (worm[ai][aj].defendi == -1))
    return 0;

  if ((((ocolor==WHITE) && (white_eye[ai][aj].origini == -1))
       || ((ocolor==BLACK) && (black_eye[ai][aj].origini == -1)))
      && (does_attack(ti, tj, ai, aj))) {
    retrofit_half_eye(ti, tj, bi, bj);
    if (((ocolor==WHITE) && (white_eye[ci][cj].marginal))
	|| ((ocolor==BLACK) && (black_eye[ci][cj].marginal)))
      retrofit_genus(bi, bj, 1);
  }
  return 0;
}


/*

?..?          ?a.?
OX*.          dXt.
XOXO          cebO

*/

int
make_trouble_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int ui, uj;
  int other=OTHER_COLOR(color);
  int can_attack_b=0;
  int can_defend_e=0;
  int can_attack_c=0;

  UNUSED(pattern);

  OFFSET(-1, -1, ai, aj);
  OFFSET(1, 0, bi, bj);
  OFFSET(1, -2, ci, cj);
  OFFSET(0,-2, di, dj);
  OFFSET(1, -1, ei, ej);

  if ((worm[di][dj].liberties < 2) || (worm[ei][ej].liberties <2))
    return 0;

  if (worm[bi][bj].liberties==1)
    return 0;

  if (TRYMOVE(ti, tj, color)) {
    if (TRYMOVE(ai, aj, other)) {
      if (attack(bi, bj, &ui, &uj)) {
	can_attack_b=1;
	if (worm[ci][cj].attacki != -1) 
	  if (TRYMOVE(ui, uj, color)) {
	    if ((p[ci][cj] == EMPTY)
		|| !find_defense(ci, cj, NULL, NULL)) {
	      can_attack_c=1;
	      if ((p[ei][ej] != EMPTY) && 
		  worm[ei][ej].attacki != -1)
		can_defend_e=1;
	    }
	    popgo();
	  }
      }
      popgo();
    }
    popgo();
  }

  /* The functions change_attack and change_defense must not be called
   * with stackp>0.
   */

  if (can_attack_b) {
    change_attack(bi, bj, ti, tj);
    if (singleton(ci, cj))
      worm[ci][cj].value=76;
  }
  if (can_attack_c)
    change_attack(ci, cj, ti, tj);
  if (can_defend_e)
    change_defense(ei, ej, ti, tj);

  return 0;
}


/* 

A typical situation:

XXXX
XOOOXXXX
Xt.OOOOX
..u...OX
--------

Here the eye pattern !.@... is found but gives an incorrect move
at u in this example. The right move is at t.

?O?                 cad
*.O                 tbO
...                 .e.
---                 ---

So if a half eye is found at e, yet t is a safe move for x,
the half eye point is moved to t.

*/



int
move_half_eye_helper (ARGS)
{
  int ai, aj, bi, bj, ci, cj, di, dj, ei, ej;
  int m, n;
  int ocolor, xcolor;
  int move_eye=0;

  UNUSED(pattern);
  UNUSED(color);

  OFFSET(-1, 1, ai, aj);
  OFFSET(0, 1, bi, bj);
  OFFSET(-1, 0, ci, cj);
  OFFSET(-1, 2, di, dj);
  OFFSET(1, 1, ei, ej);

  ocolor=p[ai][aj];
  xcolor=OTHER_COLOR(ocolor);
  
  if (((p[ci][cj]==ocolor) || !safe_move(ci, cj, xcolor))
      && ((p[di][dj]==ocolor) || !safe_move(di, dj, xcolor))
      && (dragon[ai][aj].heyei==ei)
      && (dragon[ai][aj].heyej==ej)
      && (safe_move(ti, tj, xcolor)))
    if (TRYMOVE(ti, tj, ocolor)) {
      if (!safe_move(ei, ej, xcolor))
	move_eye=1;
      popgo();
    }
  if (move_eye)
    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++)
	if ((dragon[m][n].origini==dragon[ai][aj].origini)
	    && (dragon[m][n].originj==dragon[ai][aj].originj)) {
	  dragon[m][n].heyei=ti;
	  dragon[m][n].heyej=tj;
	}
  return 0;
}



/*
 * LOCAL Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

