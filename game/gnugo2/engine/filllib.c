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
#include "liberty.h"

static int find_backfilling_move(int m, int n, int color, int *i, int *j);
static int does_capture_something(int m, int n, int color);

/* Determine whether a point is adjacent to at least one own string which
 * isn't dead.
 */
static int living_neighbor(int m, int n, int color)
{
  if (m > 0
      && p[m-1][n] == color
      && dragon[m-1][n].status != DEAD)
    return 1;

  if (m < board_size-1
      && p[m+1][n] == color
      && dragon[m+1][n].status != DEAD)
    return 1;

  if (n > 0
      && p[m][n-1] == color
      && dragon[m][n-1].status != DEAD)
    return 1;

  if (n < board_size-1
      && p[m][n+1] == color
      && dragon[m][n+1].status != DEAD)
    return 1;

  return 0;
}

/* Determine whether (m, n) effectively is a black or white point.
 * The test for inessentiality is to avoid filling the liberties
 * around a killing nakade string.
 */
static void
analyze_neighbor(int m, int n, int *found_black, int *found_white)
{
  switch (p[m][n]) {
    case EMPTY:
      if (safe_move(m, n, BLACK)
	  && !safe_move(m, n, WHITE)
	  && living_neighbor(m, n, BLACK))
	*found_black = 1;
      if (safe_move(m, n, WHITE)
	  && !safe_move(m, n, BLACK)
	  && living_neighbor(m, n, WHITE))
	*found_white = 1;
      break;

    case BLACK:
      if (!worm[m][n].inessential) {
	if (dragon[m][n].status == ALIVE)
	  *found_black = 1;
	else
	  *found_white = 1;
      }
      break;

    case WHITE:
      if (!worm[m][n].inessential) {
	if (dragon[m][n].status == ALIVE)
	  *found_white = 1;
	else
	  *found_black = 1;
      }
      break;
  }
}

/* If no move of value can be found to play, this seeks to fill a
 * common liberty, backfilling or back-capturing if necessary. When
 * backfilling we take care to start from the right end, in the case
 * that several backfilling moves are ultimately necessary.
 *
 * Returns the move in (*I, *J) with the value 1 in *VAL.
 * COLOR is the color for which the move is made.
 * */

int 
fill_liberty(int *i, int *j, int *val, int color)
{
  int m, n;
  int other=OTHER_COLOR(color);
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      /* It seems we can't trust an empty liberty to be gray-colored
       * either as a cave or as a cavity. Instead we look for empty
       * intersections with at least one neighbor of each color, where
       * dead stones count as enemy stones. We also count empty
       * neighbor's to either color if the other can't play there.
       */
      int found_white = 0;
      int found_black = 0;

      if (p[m][n] != EMPTY)
	continue;

      if (m > 0)
	analyze_neighbor(m-1, n, &found_black, &found_white);
      if (m < board_size-1)
	analyze_neighbor(m+1, n, &found_black, &found_white);
      if (n > 0)
	analyze_neighbor(m, n-1, &found_black, &found_white);
      if (n < board_size-1)
	analyze_neighbor(m, n+1, &found_black, &found_white);

      /* Do we have neighbors of both colors? */
      if (!(found_white && found_black))
	continue;

      /* Ok, we wish to play here, but maybe we can't. The following
       * cases may occur:
       * 1. Move is legal and safe.
       * 2. Move is legal but not safe because it's in the middle of a seki.
       * 3. Move is legal but not safe, can be played after backfilling.
       * 4. Move is an illegal ko recapture.
       * 5. Move is illegal but can be played after back-captures.
       */

      /* Legal and safe, just play it. */
      if (safe_move(m, n, color)) {
	*i = m;
	*j = n;
	*val = 1;
	return 1;
      }

      /* Try to play the move. */
      if (trymove(m, n, color, "fill_liberty", -1, -1)) {
	/* Legal, but not safe. Look for backfilling move. */
	popgo();
	if (find_backfilling_move(m, n, color, i, j)) {
	  /* In certain positions it may happen that an illegal move
           * is found. This probably only can happen if we try to play
           * a move inside a lost semeai. Anyway we should discard the
           * move.
	   */
	  if (!legal(*i, *j, color)) {
	    *i = -1;
	    *j = -1;
	    continue;
	  }
	  else {
	    *val = 1;
	    return 1;
	  }
	}
	else {
	  /* If we captured some stones, this move should be ok anyway. */
	  if (does_capture_something(m, n, color)) {
	    *i = m;
	    *j = n;
	    *val = 1;
	    return 1;
	  }
	}
      }
      else {
	/* Move is illegal. Look for an attack on one of the neighbor
         * worms. If found, return that move for back-capture.
	 */
	if (m>0
	    && p[m-1][n]==other
	    && worm[m-1][n].attack_code==1) {
	  *i = worm[m-1][n].attacki;
	  *j = worm[m-1][n].attackj;
	  *val = 1;
	  return 1;
	}
	if (m<board_size-1
	    && p[m+1][n]==other
	    && worm[m+1][n].attack_code==1) {
	  *i = worm[m+1][n].attacki;
	  *j = worm[m+1][n].attackj;
	  *val = 1;
	  return 1;
	}
	if (n>0
	    && p[m][n-1]==other
	    && worm[m][n-1].attack_code==1) {
	  *i = worm[m][n-1].attacki;
	  *j = worm[m][n-1].attackj;
	  *val = 1;
	  return 1;
	}
	if (n<board_size-1
	    && p[m][n+1]==other
	    && worm[m][n+1].attack_code==1) {
	  *i = worm[m][n+1].attacki;
	  *j = worm[m][n+1].attackj;
	  *val = 1;
	  return 1;
	}
      }
    }
  /* Nothing found. */
  return 0;
}

/* The strategy for finding a backfilling move is to first identify
 * moves that
 *
 * 1. defends the position obtained after playing (m,n).
 * 2. captures a stone adjacent to our neighbors to (m,n), before
 *    (m,n) is played.
 *
 * Then we check which of these are legal before (m,n) is played. If
 * there is at least one, we take one of these arbitrarily as a
 * backfilling move.
 *
 * Now it may happen that (m,n) still isn't a safe move. In that case
 * we recurse to find a new backfilling move. To do things really
 * correctly we should also give the opponent the opportunity to keep
 * up the balance of the position by letting him do a backfilling move
 * of his own. Maybe this could also be arranged by recursing this
 * function. Currently we only do a half-hearted attempt to find
 * opponent moves.
 */
static int adji[MAXCHAIN];
static int adjj[MAXCHAIN];
static int adjsize[MAXCHAIN];
static int adjlib[MAXCHAIN];

static int mylibi[MAXLIBS]; /* libi and libj are occupied by globals */
static int mylibj[MAXLIBS];

static int
find_backfilling_move(int m, int n, int color, int *i, int *j)
{
  int k;
  int libs;
  int neighbors;
  int found_one = 0;
  int ai = -1;
  int aj = -1;
  int extra_pop = 0;
  int success = 0;
  int acode;
  
  /* Play (m,n) and identify all liberties and adjacent strings. */
  if (!trymove(m, n, color, "find_backfilling_move", m, n))
    return 0; /* This shouldn't happen, I believe. */

  /* The move wasn't safe, so there must be an attack for the
   * opponent. Save it for later use.
   */
  acode = attack(m, n, &ai, &aj);
  assert(acode != 0 && ai>=0 && aj>=0);
  
  /* Find liberties. */
  libs = countlib(m, n, color);
  /* Copy the list of liberties from globals, they may become lost in
   * later reading.
   */
  for (k=0; k<libs; k++) {
    mylibi[k] = libi[k];
    mylibj[k] = libj[k];
  }

  /* Find neighbors. */
  chainlinks(m, n, &neighbors, adji, adjj, adjsize, adjlib);

  /* Remove (m,n) again. */
  popgo();
  
  /* It's most fun to capture stones. Start by trying to take some
   *  neighbor off the board.
   *
   * FIXME: Maybe we should take care to find the neighbor with the
   * fewest liberties, since that string probably can be removed
   * fastest. For the moment we assume this to be nonimportant.
   *
   * FIXME: We rely on attack() not modifying *i, *j unless an attack
   * is found. I believe this should be safe, but otherwise we must do
   * things more carefully.
   *
   * FIXME: It seems we have to return immediately when we find an
   * attacking move, because recursing for further backfilling might
   * lead to moves which complete the capture but cannot be played
   * before the attacking move itself. This is not ideal but probably
   * good enough.
   */
  for (k=0; k<neighbors; k++) {
    if (attack(adji[k], adjj[k], i, j)) {
#if 0
      found_one = 1;
      break;
#endif
      return 1;
    }
  }
  
  /* Otherwise look for a safe move at a liberty. */
  if (!found_one) {
    for (k=0; k<libs; k++) {
      if (safe_move(mylibi[k], mylibj[k], color)) {
	*i = mylibi[k];
	*j = mylibj[k];
	found_one = 1;
	break;
      }
    }
  }

  if (!found_one)
    return 0;
  
  if (!trymove(*i, *j, color, "find_backfilling_move", m, n))
    return 0; /* This really shouldn't happen. */

  /* Allow opponent to get a move in here. */
  if (trymove(ai, aj, OTHER_COLOR(color), "find_backfilling_move", m, n))
    extra_pop = 1;

  /* If still not safe, recurse to find a new backfilling move. */
  if (!safe_move(m, n, color))
    success = find_backfilling_move(m, n, color, i, j);
  else
    success = 1;

  /* Pop move(s) and return. */
  if (extra_pop)
    popgo();
  popgo();
  return success;
}
    

/* This could probably be done nicer, maybe there should be a util
 * function for this purpose. Notice that this implementation only
 * works for stackp==0.
 */
static int
does_capture_something(int m, int n, int color)
{
  int other = OTHER_COLOR(color);
  if (m>0 && p[m-1][n]==other && worm[m-1][n].liberties==1)
    return 1;
  if (m<board_size-1 && p[m+1][n]==other && worm[m+1][n].liberties==1)
    return 1;
  if (n>0 && p[m][n-1]==other && worm[m][n-1].liberties==1)
    return 1;
  if (n<board_size-1 && p[m][n+1]==other && worm[m][n+1].liberties==1)
    return 1;
  return 0;
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
