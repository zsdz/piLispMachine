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
#include <assert.h>
#include "../engine/liberty.h"
#include "patterns.h"

/* Try to match all (permutations of) connection patterns at (m,n).
 * For each match, if it is a B pattern, set cutting point in worm
 * data structure and make eye space marginal for the connection
 * inhibiting entries of the pattern. If it is a C pattern, amalgamate
 * the dragons in the pattern.
 */


static void cut_connect_callback(int m, int n, int color,
				 struct pattern *pattern, int ll)
{
  int stari, starj;
  int k;
  int first_dragoni=-1, first_dragonj=-1;
  int second_dragoni=-1, second_dragonj=-1;

  int other=OTHER_COLOR(color);

  if ((pattern->movei) == -1) {
    stari = -1;
    starj = -1;
  } 
  else {
    TRANSFORM(pattern->movei, pattern->movej, &stari, &starj, ll);
    stari += m;
    starj += n;
    if (!safe_move(stari, starj, other))
      return;
  }

  if (pattern->helper) {
    if (!pattern->helper(pattern, ll, stari, starj, color))
      return;
  }

  if (pattern->class & CLASS_B) {
    /* Require that the X stones in the pattern are tactically safe. */
    for (k = 0; k < pattern->patlen; ++k) { /* match each point */
      if (pattern->patn[k].att == ATT_X) {
	/* transform pattern real coordinate */
	int x, y;
	TRANSFORM(pattern->patn[k].x,pattern->patn[k].y,&x,&y,ll);
	x += m;
	y += n;

	if ((worm[x][y].attacki != -1)
	    && ((stari == -1)
		|| !does_defend(stari, starj, x, y)))
	  return; /* Match failed */
      }
    }
  }

  /* get here => pattern matches */

  if (pattern->class & CLASS_B) {
    TRACE("Cutting pattern %s+%d found at %m\n",
	  pattern->name, ll, m, n);
    if (stari != -1)
      TRACE("cutting point %m\n", stari, starj);
  }
  else
    TRACE("Connecting pattern %s+%d found at %m\n",
	  pattern->name, ll, m, n);

  /* If it is a B pattern, set cutting point in worm data and make eye
   * space marginal.
   */
  
  if (pattern->class & CLASS_B) {
    if (stari != -1) {
      if (color == WHITE)
	white_eye[stari][starj].cut = 1;
      else
	black_eye[stari][starj].cut = 1;
      if (color == WHITE && white_eye[stari][starj].color == WHITE_BORDER)
	white_eye[stari][starj].marginal = 1;
      else if (color == BLACK && black_eye[stari][starj].color == BLACK_BORDER)
	black_eye[stari][starj].marginal = 1;
    }
  }
  else if (!(pattern->class & CLASS_C))
    return; /* Nothing more to do, up to the helper or autohelper
	       to amalgamate dragons. */

  /* If it is a C pattern, find the dragons to connect.
   * If it is a B pattern, find eye space points to inhibit connection
   * through.
   */
  for (k = 0; k < pattern->patlen; ++k) { /* match each point */
    int x, y; /* absolute (board) co-ords of (transformed) pattern element */

    /* transform pattern real coordinate */
    TRANSFORM(pattern->patn[k].x,pattern->patn[k].y,&x,&y,ll);
    x+=m;
    y+=n;

    /* Look for dragons to amalgamate. Never amalgamate stones which
     * can be attacked.
     */
    if ((pattern->class & CLASS_C) && (p[x][y] == color)
	&& (worm[x][y].attacki == -1)) {
      if (first_dragoni == -1) {
	first_dragoni = dragon[x][y].origini;
	first_dragonj = dragon[x][y].originj;
      }
      else if ((second_dragoni == -1)
	       && ((dragon[x][y].origini != first_dragoni)
		   || (dragon[x][y].originj != first_dragonj))) {
	second_dragoni = dragon[x][y].origini;
	second_dragonj = dragon[x][y].originj;
	/* A second dragon found, we amalgamate them at once. */
	join_dragons(second_dragoni, second_dragonj,
		     first_dragoni, first_dragonj);
	TRACE("Joining dragon %m to %m\n",
	      second_dragoni, second_dragonj,
	      first_dragoni, first_dragonj);
	/* Now look for another second dragon */
	second_dragoni = -1;
	second_dragonj = -1;
	assert(dragon[x][y].origini == first_dragoni &&
	       dragon[x][y].originj == first_dragonj);
      }
    }

    /* Inhibit connections */
    if (pattern->class & CLASS_B) {
      if (pattern->patn[k].att != ATT_not)
	break; /* The inhibition points are guaranteed to come first. */
      if (color == WHITE && white_eye[x][y].color == WHITE_BORDER)
	white_eye[x][y].type |= INHIBIT_CONNECTION;
      else if (color == BLACK && black_eye[x][y].color == BLACK_BORDER)
	black_eye[x][y].type |= INHIBIT_CONNECTION;
    }
  } /* loop over elements */
}


/* Only consider B patterns. */
static void cut_callback(int m, int n, int color,
			 struct pattern *pattern, int ll)
{
  if (pattern->class & CLASS_B)
    cut_connect_callback(m, n, color, pattern, ll);
}
  

/* Only consider non-B patterns. */
static void conn_callback(int m, int n, int color,
			 struct pattern *pattern, int ll)
{
  if (!(pattern->class & CLASS_B))
    cut_connect_callback(m, n, color, pattern, ll);
}
  
/* Find cutting points which should inhibit amalgamations and sever
 * the adjacent eye space.
 */
void
find_cuts(void)
{
  int m, n;
  for (m = 0; m < board_size; m++)
    for (n = 0; n < board_size; n++)
      if (p[m][n])
	matchpat(m, n, cut_callback, p[m][n], 0, conn);
}

/* Find explicit connection patterns and amalgamate the involved dragons. */
void
find_connections(void)
{
  int m, n;
  for (m = 0; m < board_size; m++)
    for (n = 0; n < board_size; n++)
      if (p[m][n])
	matchpat(m, n, conn_callback, p[m][n], 0, conn);
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
