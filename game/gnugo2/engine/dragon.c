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

static void dragon_ring(int m, int n, int color, int *i, int *j);
static void dragon_eye(int m, int n, struct eye_data[MAX_BOARD][MAX_BOARD]);
static void revise_worm_values(void);
static int evaluate_diagonal_intersection(int m, int n, int color,
					  int *vitali, int *vitalj);

void 
make_dragons()
{
  int m, n, i, j;
  int border_color;

/* We start with the dragon data copied from the worm data, then
 * modify it as the worms are amalgamated into larger dragons.
 */

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      dragon[m][n].size         = worm[m][n].size;
      dragon[m][n].value        = 0;
      dragon[m][n].color        = worm[m][n].color;
      dragon[m][n].origini      = worm[m][n].origini;
      dragon[m][n].originj      = worm[m][n].originj;

      dragon[m][n].lunchi       = -1;
      dragon[m][n].lunchj       = -1;
      dragon[m][n].friendi      = -1;
      dragon[m][n].friendj      = -1;
      dragon[m][n].bridgei      = -1;
      dragon[m][n].bridgej      = -1;

      dragon[m][n].escape_route = worm[m][n].liberties4;
      dragon[m][n].heyes        = 0;
      dragon[m][n].semeai       = 0;

      half_eye[m][n].type       = 0;
      
      if (worm[m][n].origini == m && worm[m][n].originj == n)
	DEBUG(DEBUG_DRAGONS, 
	      "Initialising dragon from worm at %m, size %d, escape_route=%d\n", 
	      m, n, worm[m][n].size, worm[m][n].liberties4);
    }

  /* Amalgamate cavities. 
   *
   * Begin by finding the INESSENTIAL strings. These are defined as
   * surrounded strings which have no life potential unless part of
   * their surrounding chain can be captured. We give a conservative
   * definition of inessential: the genus must be zero and there can no
   * second order liberties or edge liberties; and if it is removed from
   * the board, the remaining cavity has border color the opposite color
   * of the string and contains at most two edge vertices.
   *
   * An inessential string can be ignored for life and death purposes. It then
   * makes sense to amalgamate the surrounding cavities into a single cave
   * (empty dragon) with bordercolor the opposite of the inessential worm.
   *
   * For example, in the following situation:
   *
   *   OOOOO
   *   O.X.O
   *   OOOOO
   *
   * we find two graybordered cavities of size one. The X string is
   * inessential, so these two cavities are amalgamated into a single cave. 
   */

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((p[m][n])
	  && (worm[m][n].origini == m)
	  && (worm[m][n].originj == n)
	  && (worm[m][n].genus == 0)
	  && (worm[m][n].liberties2 == 0)
	  && (worm[m][n].lunchi == -1)) 
      {
	int edge;
	int borigini=-1, boriginj=-1;

	border_color=find_border(m, n, &edge);
	if ((border_color != GRAY_BORDER) && (edge<3)) {
	  dragon_ring(m, n, EMPTY, &borigini, &boriginj);
	  worm[m][n].inessential=1;
	  propagate_worm(m, n);
	  dragon[borigini][boriginj].color=border_color;
	}
      }
    }
  make_domains();

  /* Find explicit connections patterns in database and amalgamate
   * involved dragons.
   */
  find_connections();
  
  /* amalgamate dragons sharing an eyespace (not ko) */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {

      if ((black_eye[m][n].color == BLACK_BORDER) 
	  && (black_eye[m][n].origini == m)
	  && (black_eye[m][n].originj == n)
	  && ((!worm[m][n].ko)
	      || (black_eye[m][n].esize > 1))) /* Only exclude living kos. */
	dragon_eye(m, n, black_eye);

      if ((white_eye[m][n].color == WHITE_BORDER)
	  && (white_eye[m][n].origini == m)
	  && (white_eye[m][n].originj == n)
	  && ((!worm[m][n].ko)
	      || (white_eye[m][n].esize > 1))) /* Only exclude living kos. */
	dragon_eye(m, n, white_eye);
    }

  /* Find adjacent worms which can be easily captured: */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((worm[m][n].origini==m)
	  && (worm[m][n].originj==n)
	  && p[m][n] && (worm[m][n].lunchi != -1)) 
      {
	i=worm[m][n].lunchi;
	j=worm[m][n].lunchj;

	/* In contrast to worm lunches, a dragon lunch must also be
	 * able to defend itself. */

	if (worm[i][j].defendi == -1)
	  continue;

	/* If several lunches are found, we pick the juiciest.
	 * First maximize cutstone, then minimize liberties */

	{
	  int origini = dragon[m][n].origini;
	  int originj = dragon[m][n].originj;

	  if ((dragon[origini][originj].lunchi == -1)
	      || (worm[i][j].cutstone
		  > worm[dragon[origini][originj].lunchi]
 		        [dragon[origini][originj].lunchj].cutstone)
	      || ((worm[i][j].cutstone 
		   == worm[dragon[origini][originj].lunchi]
		          [dragon[origini][originj].lunchj].cutstone) 
		  && (worm[i][j].liberties
		      < worm[dragon[m][n].lunchi][dragon[m][n].lunchj].liberties))) 
	  {
	    dragon[origini][originj].lunchi = worm[i][j].origini;
	    dragon[origini][originj].lunchj = worm[i][j].originj;
	    TRACE("at %m setting %m.lunch to %m (cutstone=%d)\n",
		  m,n, origini,originj,
		  worm[i][j].origini, worm[i][j].originj,worm[i][j].cutstone);
	  }
	}
      }
    }
	    
  /* In case origins of dragons got moved, put the dragons of eyes aright. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if (black_eye[i][j].dragoni != -1) {
	  int di=dragon[black_eye[i][j].dragoni]
                       [black_eye[i][j].dragonj].origini;
	  int dj=dragon[black_eye[i][j].dragoni]
	               [black_eye[i][j].dragonj].originj;
	  black_eye[i][j].dragoni=di;
	  black_eye[i][j].dragonj=dj;
      }

      if (white_eye[i][j].dragoni != -1) {
	  int di=dragon[white_eye[i][j].dragoni]
	               [white_eye[i][j].dragonj].origini;
	  int dj=dragon[white_eye[i][j].dragoni] 
	               [white_eye[i][j].dragonj].originj;
	  white_eye[i][j].dragoni=di;
	  white_eye[i][j].dragonj=dj;
      }
    }

  /* Find topological half eyes and false eyes by analyzing the
   * diagonal intersections, as described in the Texinfo
   * documentation (Eyes/Eye Topology).
   */

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      int sum;
      int ci = -1;
      int cj = -1;

      if (black_eye[m][n].color == BLACK_BORDER
	  && !black_eye[m][n].marginal
	  && (black_eye[m][n].neighbors <= 1)
	  && (black_eye[m][n].dragoni != -1)) {
	sum = evaluate_diagonal_intersection(m+1, n+1, BLACK, &ci, &cj) 
	  + evaluate_diagonal_intersection(m+1, n-1, BLACK, &ci, &cj) 
	  + evaluate_diagonal_intersection(m-1, n+1, BLACK, &ci, &cj) 
	  + evaluate_diagonal_intersection(m-1, n-1, BLACK, &ci, &cj);
	
	if (sum >= 4) {
	  half_eye[m][n].type = FALSE_EYE;
	  if ((black_eye[m][n].esize == 1) || legal(m, n, WHITE))
	    add_half_eye(m, n, black_eye);
	}
	else if (sum == 3) {
	  half_eye[m][n].type = HALF_EYE;
	  half_eye[m][n].ki = ci;
	  half_eye[m][n].kj = cj;
	  add_half_eye(m, n, black_eye);
	}
      }
      
      if (white_eye[m][n].color == WHITE_BORDER
	  && !white_eye[m][n].marginal
	  && (white_eye[m][n].neighbors <= 1)
	  && (white_eye[m][n].dragoni != -1)) {
	sum = evaluate_diagonal_intersection(m+1, n+1, WHITE, &ci, &cj) 
	  + evaluate_diagonal_intersection(m+1, n-1, WHITE, &ci, &cj) 
	  + evaluate_diagonal_intersection(m-1, n+1, WHITE, &ci, &cj) 
	  + evaluate_diagonal_intersection(m-1, n-1, WHITE, &ci, &cj);
	
	if (sum >= 4) {
	  half_eye[m][n].type = FALSE_EYE;
	  if ((white_eye[m][n].esize == 1) || legal(m, n, BLACK))
	    add_half_eye(m, n, white_eye);
	}
	else if (sum == 3) {
	  half_eye[m][n].type = HALF_EYE;
	  half_eye[m][n].ki = ci;
	  half_eye[m][n].kj = cj;
	  add_half_eye(m, n, white_eye);
	}
      }
    }

  /* Compute the number of eyes, half eyes, etc. in an eye space. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((black_eye[i][j].color == BLACK_BORDER) 
	  && (black_eye[i][j].origini==i)
	  && (black_eye[i][j].originj==j)) 
      {
	int max, min, attacki, attackj;

	compute_eyes(i, j, &max, &min, &attacki, &attackj, black_eye);
	DEBUG(DEBUG_EYES, "Black eyespace at %m: min=%d, max=%d\n",
	      i, j, min, max);
	black_eye[i][j].maxeye=max;
	black_eye[i][j].mineye=min;
	if (max != min)
	  DEBUG(DEBUG_EYES, "  vital point: %m\n", attacki, attackj);
	black_eye[i][j].attacki=attacki;
	black_eye[i][j].attackj=attackj;	  
	propagate_eye(i, j, black_eye);
      }
      if ((white_eye[i][j].color == WHITE_BORDER) 
	  && (white_eye[i][j].origini==i)
	  && (white_eye[i][j].originj==j)) 
      {
	int max, min, attacki, attackj;

	compute_eyes(i, j, &max, &min, &attacki, &attackj, white_eye);
	DEBUG(DEBUG_EYES, "White eyespace at %m: min=%d, max=%d\n",
	      i, j, min, max);
	white_eye[i][j].maxeye=max;
	white_eye[i][j].mineye=min;
	if (max != min)
	  DEBUG(DEBUG_EYES, "  vital point: %m\n", attacki, attackj);
	white_eye[i][j].attacki=attacki;
	white_eye[i][j].attackj=attackj;	  
	propagate_eye(i, j, white_eye);
      }
    }

  /* now we compute the genus. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((black_eye[i][j].color == BLACK_BORDER) 
	  && (black_eye[i][j].dragoni != -1) 
	  && (black_eye[i][j].origini==i)
	  && (black_eye[i][j].originj==j)) 
      {
	m = black_eye[i][j].dragoni;
	n = black_eye[i][j].dragonj;
	assert (p[m][n]==BLACK);
	TRACE("eye at %m found for dragon at %m--augmenting genus\n",
	      i, j, m, n);
	dragon[m][n].genus += (black_eye[i][j].mineye);
	dragon[m][n].heyes += (black_eye[i][j].maxeye-black_eye[i][j].mineye);
	if (black_eye[i][j].maxeye-black_eye[i][j].mineye > 0) {
	  dragon[m][n].heyei=black_eye[i][j].attacki;
	  dragon[m][n].heyej=black_eye[i][j].attackj;
	}
      }
      if ((white_eye[i][j].color == WHITE_BORDER) 
	  && (white_eye[i][j].dragoni != -1)
	  && (white_eye[i][j].origini==i)
	  && (white_eye[i][j].originj==j)) 
      {
	m = white_eye[i][j].dragoni;
	n = white_eye[i][j].dragonj;
	assert (p[m][n]==WHITE);
	TRACE("eye at %m found for dragon at %m--augmenting genus\n",
	      i, j, m, n);
	dragon[m][n].genus += (white_eye[i][j].mineye);
	dragon[m][n].heyes += (white_eye[i][j].maxeye-white_eye[i][j].mineye);
	if (white_eye[i][j].maxeye-white_eye[i][j].mineye > 0) {
	  dragon[m][n].heyei=white_eye[i][j].attacki;
	  dragon[m][n].heyej=white_eye[i][j].attackj;
	}
      }
    }

  /* Propagate genus to rest of the dragon. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
      dragon[i][j].genus
	= dragon[dragon[i][j].origini][dragon[i][j].originj].genus;


  /* Determine status: ALIVE, DEAD, CRITICAL or UNKNOWN */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((dragon[m][n].origini==m)
	  && (dragon[m][n].originj==n) && p[m][n])
      {
	/* entirely ad-hoc : aim for -ve value to mean we need
	 * to worry about this dragon 
	 */
	dragon[m][n].vitality = 6 * dragon[m][n].genus 
	  + dragon[m][n].heyes + dragon[m][n].escape_route - 10;
	dragon[m][n].status=dragon_status(m, n);
	dragon[m][n].safety=dragon[m][n].status;
	sgf_dragon_status(m,n,dragon[m][n].status);
      }
    }
  
  /* The dragon data is now correct at the origin of each dragon but
   * we need to copy it to every vertex.  
   */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      struct dragon_data *d = &(dragon[m][n]);

      dragon[m][n] = dragon[d->origini][d->originj];
    }

  revise_worm_values();
  /* Now we compute the dragon values */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if (p[m][n] && (worm[m][n].origini==m) && (worm[m][n].originj==n))
	dragon[dragon[m][n].origini][dragon[m][n].originj].value += worm[m][n].value;
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if (p[m][n])
	dragon[m][n].value = dragon[dragon[m][n].origini][dragon[m][n].originj].value;
}



/* print status info on all dragons. (Can be invoked from gdb) 
 */
void 
show_dragons(void)
{
  static const char *cnames[] = 
    {"(empty)", "white dragon", "black dragon",
     "gray-bordered cave", "black-bordered cave", "white-bordered cave"};
  static const char *snames[] = 
    {"dead", "alive", "critical", "unknown"};

  int m, n;

  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++) {
      struct worm_data *w = &(worm[m][n]);

      if (((w->origini)==m)&&((w->originj)==n)) {
	if (p[m][n]) {
	  gprintf("%m : %s string of size %d,value %d and genus %d: (%d,%d,%d,%d)",
		  m,n,
		  p[m][n]==BLACK ? "black" : "white",
		  w->size,w->value,
		  w->genus,
		  w->liberties,
		  w->liberties2,
		  w->liberties3,
		  w->liberties4);
	  if (w->cutstone==1)
	    gprintf ("%o - is a potential cutting stone\n");
	  else if (w->cutstone==2)
	    gprintf("%o - is a cutting stone\n");
	  else gprintf("%o\n");

	  if (w->attacki != -1)
	    gprintf("- attackable at %m\n", w->attacki, w->attackj);
	  if (w->defendi != -1)
	    gprintf("- defendable at %m\n", w->defendi, w->defendj);

	  if (w->inessential)
	    gprintf("- is inessential\n");
	  
	  if (w->ko == 1)
	    gprintf("- is a ko stone\n");
	}
	else 
	  gprintf("%m : cavity of size %d\n",m,n,w->size);
      }
    }

  gprintf("%o\n");
  for (m=0;m<board_size;m++)
    for (n=0;n<board_size;n++) {
      struct dragon_data *d = &(dragon[m][n]);

      if (((d->origini)==m) && ((d->originj)==n)) {
	if (p[m][n]) {
	  gprintf("%m : %s dragon size %d, value %d, genus %d, half eyes %d, escape factor %d, status %s, safety %s, vitality %d\n",
		  m,n,
		  p[m][n]==BLACK ? "B" : "W",
		  d->size,
		  d->value,
		  d->genus,
		  d->heyes,
		  d->escape_route,
		  snames[d->status],
		  snames[d->safety],
		  d->vitality);
	  if (d->lunchi != -1)
	    gprintf("... adjacent worm %m is lunch\n", d->lunchi, d->lunchj);
	}
	else {
	  gprintf("%m : cave of size %d and border color %s\n",
		  m,n,
		  d->size,
		  cnames[d->color]);
	  if (d->color == BLACK_BORDER || d->color == WHITE_BORDER) {
	    if (!worm[m][n].ko)
	      gprintf("... surrounded by dragon at %m\n",
		      d->borderi, d->borderj);
	    else
	      gprintf("... is a ko\n");
	  }
	}
      }
    }
}


  
/* The function dragon_ring(m, n, color, *di, *dj) amalgamates every 
 * dragon adjacent to the worm at (m, n) with the given color. The
 * amalgamated dragons then form a bigger dragon ringing the worm at
 * (m, n). 
 *
 * Amalgamation means that the individual dragons are to be combined
 * into the same dragon.  If color is not EMPTY, then p[m][n]==0 and
 * we have found an eye (possibly false). In this (*di, *dj) is set to
 * the origin of the surrounding dragon and dragon[*di][*dj].genus is
 * incremented.  
 */

static void
dragon_ring(int m, int n, int color, int *di, int *dj)
{
  int i, j;
  int dragoni=-1, dragonj=-1;

  DEBUG(DEBUG_DRAGONS, "amalgamate dragons around %m\n", m, n);
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((worm[i][j].origini==m) && (worm[i][j].originj==n)) {

	if ((i>0)&&(p[i-1][j]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i-1][j].origini;
	    dragonj=dragon[i-1][j].originj;
	  } else if ((dragoni != dragon[i-1][j].origini) 
		     || (dragonj != dragon[i-1][j].originj))
	    join_dragons(dragon[i-1][j].origini, dragon[i-1][j].originj, 
			 dragoni, dragonj);
	}

	if ((i<board_size-1)&&(p[i+1][j]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i+1][j].origini;
	    dragonj=dragon[i+1][j].originj;
	  } else if ((dragoni != dragon[i+1][j].origini) 
		     || (dragonj != dragon[i+1][j].originj))
	    join_dragons(dragon[i+1][j].origini, dragon[i+1][j].originj, 
			 dragoni, dragonj);
	}

	if ((j>0)&&(p[i][j-1]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i][j-1].origini;
	    dragonj=dragon[i][j-1].originj;
	  } else if ((dragoni != dragon[i][j-1].origini) 
		     || (dragonj != dragon[i][j-1].originj))
	      join_dragons(dragon[i][j-1].origini, dragon[i][j-1].originj, 
			   dragoni, dragonj);
	}

	if ((j<board_size-1)&&(p[i][j+1]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i][j+1].origini;
	    dragonj=dragon[i][j+1].originj;
	  } else if ((dragoni != dragon[i][j+1].origini) 
		     || (dragonj != dragon[i][j+1].originj))
	      join_dragons(dragon[i][j+1].origini, dragon[i][j+1].originj, 
			   dragoni, dragonj);
	}
      }
    }

  if (color) {
    ASSERT(p[m][n] == 0, m, n); /* (m, n) is a cave */

    /* record the fact that we found an eye (could be false) */
    (dragon[dragoni][dragonj].genus)++;
  }
  *di=dragoni;   /* (m, n) is an inessential string. */
  *dj=dragonj;
}


/* 
 * dragon_eye(m, n, *di, *dj) is invoked with (m, n) the origin of an
 * eyespace. It unites all the worms adjacent to non-marginal points of
 * the eyespace into a single dragon with orign (*di, *dj). In addition
 * to marginal eye space points, amalgamation is inhibited for points
 * with the INHIBIT_CONNECTION type set.
 *
 * This is based on the older function dragon_ring. Unlike dragon_ring
 * this function does not attempt to keep track of the genus.
 */

static void
dragon_eye(int m, int n, struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int i, j;
  int dragoni=-1, dragonj=-1;
  int color;

  /* don't amalgamate across ikken tobi */
  if ((eye[m][n].esize==3) && (eye[m][n].msize>1))
    return;

  DEBUG(DEBUG_DRAGONS, "amalgamate dragons around %m\n", m, n);
  if (eye[m][n].color==BLACK_BORDER)
    color=BLACK;
  else {
    assert(eye[m][n].color==WHITE_BORDER);
    color=WHITE;
  }

  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((eye[i][j].origini==m)
	  && (eye[i][j].originj==n)
	  && !eye[i][j].marginal
	  && !(eye[i][j].type & INHIBIT_CONNECTION)) 
      {
	if ((i>0)&&(p[i-1][j]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i-1][j].origini;
	    dragonj=dragon[i-1][j].originj;
	  } else if ((dragoni != dragon[i-1][j].origini) 
		     || (dragonj != dragon[i-1][j].originj))
	    join_dragons(dragon[i-1][j].origini, dragon[i-1][j].originj, 
			 dragoni, dragonj);
	}

	if ((i<board_size-1)&&(p[i+1][j]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i+1][j].origini;
	    dragonj=dragon[i+1][j].originj;
	  } else if ((dragoni != dragon[i+1][j].origini) 
		     || (dragonj != dragon[i+1][j].originj))
	    join_dragons(dragon[i+1][j].origini, dragon[i+1][j].originj, 
			 dragoni, dragonj);
	}

	if ((j>0)&&(p[i][j-1]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i][j-1].origini;
	    dragonj=dragon[i][j-1].originj;
	  } else if ((dragoni != dragon[i][j-1].origini) 
		     || (dragonj != dragon[i][j-1].originj))
	      join_dragons(dragon[i][j-1].origini, dragon[i][j-1].originj, 
			   dragoni, dragonj);
	}

	if ((j<board_size-1)&&(p[i][j+1]==color)) {
	  if (dragoni == -1) {
	    dragoni=dragon[i][j+1].origini;
	    dragonj=dragon[i][j+1].originj;
	  } else if ((dragoni != dragon[i][j+1].origini) 
		     || (dragonj != dragon[i][j+1].originj))
	      join_dragons(dragon[i][j+1].origini, dragon[i][j+1].originj, 
			   dragoni, dragonj);
	}
      }
    }

  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((eye[i][j].color == BLACK_BORDER 
	   || eye[i][j].color == WHITE_BORDER) 
	  && (eye[i][j].origini==m)
	  && (eye[i][j].originj==n)) 
      {
	eye[i][j].dragoni=dragoni;
	eye[i][j].dragonj=dragonj;
      }
    }
}


/* 
 * join_dragons amalgamates the dragon with origin at (i, j) to the
 * dragon with origin at (m, n). 
 */

void 
join_dragons(int i, int j, int m, int n)
{
  int t, u;
  
  assert(worm[i][j].color==worm[m][n].color);

  DEBUG(DEBUG_DRAGONS,
	"joining dragon at %m with escape route %d to dragon at %m with %d\n", 
	i, j, dragon[i][j].escape_route, m, n, dragon[m][n].escape_route);

  dragon[m][n].size  = dragon[m][n].size + dragon[i][j].size;
  dragon[m][n].genus = dragon[m][n].genus + dragon[i][j].genus;
  if (dragon[i][j].escape_route > dragon[m][n].escape_route)
    dragon[m][n].escape_route = dragon[i][j].escape_route;

  for (t=0; t<board_size; t++)
    for (u=0; u<board_size; u++){
      if ((dragon[t][u].origini == i) && (dragon[t][u].originj == j)) {
	dragon[t][u].origini = m;
	dragon[t][u].originj = n;
      }
    }
}



/*
 * dragon_status(i, j) tries to determine whether the dragon at (i, j) is
 * ALIVE, DEAD, or UNKNOWN. The algorithm is not perfect and can give
 * incorrect answers. 
 *
 * The dragon is judged alive if its genus is >1. It is judged dead if
 * the genus is <2, it has no escape route, and no adjoining string can
 * be easily captured. Otherwise it is judged UNKNOWN.
 */

int 
dragon_status(int i, int j)
{

  int true_genus = 2*dragon[i][j].genus + dragon[i][j].heyes;

  if (true_genus > 3)
    return ALIVE;

  if ((dragon[i][j].size==worm[i][j].size)
      && (worm[i][j].attacki != -1) 
      && (worm[i][j].defendi == -1)
      && (true_genus < 3))
    return DEAD;
  
  if ((dragon[i][j].lunchi != -1)
      && (true_genus < 3)
      && (worm[dragon[i][j].lunchi][dragon[i][j].lunchj].defendi != -1)
      && (dragon[i][j].escape_route <5))
    if ((true_genus == 2)
	|| (worm[dragon[i][j].lunchi][dragon[i][j].lunchj].size >2))
      return CRITICAL;

  if ((dragon[i][j].lunchi != -1)
      && (true_genus >= 3))
    return ALIVE;

  if ((dragon[i][j].lunchi == -1)
      || (worm[dragon[i][j].lunchi][dragon[i][j].lunchj].cutstone <2)) 
  {
    if ((true_genus < 3) && (dragon[i][j].escape_route==0))
      if ((worm[i][j].defendi == -1) ||
	  !worm[worm[i][j].defendi][worm[i][j].defendj].ko)
	return DEAD;

    if ((true_genus == 3) && (dragon[i][j].escape_route < 6))
      return CRITICAL;
  }

  return UNKNOWN;
}


/* When the dragons have been computed, we can revise the values of
 * the worms. First we degrade potential cutting stones to non-cutting
 * if the worm is surrounded entirely by alive dragons, and under the
 * same conditions cutting stones are downgraded to potentially
 * cutting. Then we revise the values of non-cutting stones being
 * under attack. The dragon values are revised accordingly.
 */

static void
revise_worm_values(void)
{
  int m, n;

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if (p[m][n]
	  && (worm[m][n].origini == m)
	  && (worm[m][n].originj == n)) 
      {
	/* Found the origin of a worm. Revise it's value. */
	if (worm[m][n].cutstone >= 2)
	  worm[m][n].value = 83 + 4*worm[m][n].size;
	else if (worm[m][n].cutstone == 1)
	  worm[m][n].value = 76 + 4*worm[m][n].size;
	else
	  worm[m][n].value = 70 + 4*worm[m][n].size;
      }
      propagate_worm(m, n);
    }
  /* If a worm is involved in a semeai (small enough to be
   * found by the reading code), add half the value of the
   * opposing string. 
   */
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if (p[m][n]
	  && (worm[m][n].origini == m)
	  && (worm[m][n].originj == n)) 
	{
	  if ((worm[m][n].attack_code == 1) 
	      && (worm[m][n].defend_code == 1)) {
	    int i, j;
	    int semeaii=-1, semeaij=-1;
	    int found_semeai=0;
	    int other=OTHER_COLOR(p[m][n]);
	    
	    for (i=0; i<board_size; i++)
	      for (j=0; j<board_size; j++) {
		if (found_semeai)
		  continue;
		if ((worm[i][j].color==other) 
		    && (worm[i][j].attack_code == 1)
		    && (worm[i][j].defend_code == 1))
		  if (((i>0) 
		       && (worm[i-1][j].origini==m) 
		       && (worm[i-1][j].originj==n))
		      || 
		      ((i<board_size-1) 
		       && (worm[i+1][j].origini==m) 
		       && (worm[i+1][j].originj==n))
		      || 
		      ((j>0)
		       && (worm[i][j-1].origini==m) 
		       && (worm[i][j-1].originj==n))
		      || 
		      ((j<board_size-1) 
		       && (worm[i][j+1].origini==m) 
		       && (worm[i][j+1].originj==n)))
		    if (trymove(worm[m][n].attacki,
				worm[m][n].attackj,
				p[i][j],"",-1,-1)) {
		      if (!attack(i, j, NULL, NULL)) {
			found_semeai=1;
			semeaii=i;
			semeaij=j;
		      }
		      popgo();
		    }
	      }
	    if (found_semeai) {
	      if (worm[semeaii][semeaij].cutstone >= 2)
		worm[m][n].value += (83 + 4*worm[semeaii][semeaij].size)/2;
	      if (worm[semeaii][semeaij].cutstone >= 2)
		worm[m][n].value += (76 + 4*worm[semeaii][semeaij].size)/2;
	      else
		worm[m][n].value += (70 + 4*worm[semeaii][semeaij].size)/2;
	      propagate_worm(m, n);
	    }
	  }
	}
    }
}


/* Evaluate an intersection which is diagonal to an eye space,
 * as described in the Texinfo documentation (Eyes/Eye Topology).
 */
static int 
evaluate_diagonal_intersection(int m, int n, int color,
			       int *vitali, int *vitalj)
{
  int value = 0;
  int other = OTHER_COLOR(color);

  /* Check whether intersection is off the board.*/
  if (m<0 || m>=board_size)
    value += 1;

  if (n<0 || n>=board_size)
    value += 1;

  if (value > 0)
    return value % 2; /* Must return 0 if both coordinates out of bounds. */

  /* Discard points within own eyespace, unless marginal. */
  if ((color == BLACK
       && black_eye[m][n].color == BLACK_BORDER
       && !black_eye[m][n].marginal)
      || (color == WHITE
	  && white_eye[m][n].color == WHITE_BORDER
	  && !white_eye[m][n].marginal))
    return 0;

  if ((p[m][n] == EMPTY) && safe_move(m, n, other))
    value = 1;
  else if (p[m][n] == other) {
    if (worm[m][n].attacki == -1)
      value = 2;
    else if (worm[m][n].defendi != -1)
      value = 1;
  }

  /* This is a bit too simplified. In the case where we have a halfeye
   * caused by three unsettled intersections, we should keep track of
   * all of them, not just the last one. */
  if (value == 1) {
    if (p[m][n] == EMPTY) {
      *vitali = m;
      *vitalj = n;
    }
    else {
      /* FIXME:
       * The vital point may be either the defense point or the attack
       * point, depending on who is in turn to play. If the points
       * happen to coincide we have no problem, otherwise we have to
       * choose. The correct choice can only be done if we know who is
       * in turn to play, but this is not and maybe should not be
       * known by make_dragons().
       *
       * A nicer way to do this would be to say that the worm to be
       * captured or defended is the vital point, but currently this
       * confuses the eye code badly.
       */
      *vitali = worm[m][n].defendi;
      *vitalj = worm[m][n].defendj;
    }
  }
  return value;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
