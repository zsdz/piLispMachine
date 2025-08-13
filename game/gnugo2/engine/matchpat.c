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
#include "../patterns/patterns.h"


/* define this to see how each phase of pattern rejection is performing */
/* #define PROFILE_MATCHER */


/* In the current implementation, the edge constraints depend on
 * the board size, because we pad width or height out to the
 * board size. (This is because it is easy to find the corners
 * of the rotated pattern, but it is harder to transform the
 * bitmask of edge constraints.)
 *
 * But since version 1.103, board size is variable. Thus we
 * make a first pass through the table once we know the board
 * size.
 *
 * This should be called once for each pattern database
 * (currently pat and conn).
 */

static void
fixup_patterns_for_board_size(struct pattern *pattern)
{
  for ( ; pattern->patn; ++pattern )
    if (pattern->edge_constraints != 0) {
      /* we extend the pattern in the direction opposite the constraint,
       * such that maxi (+ve) - mini (-ve) = board_size-1
       * Note : the pattern may be wider than the board, so
       * we need to be a bit careful !
       */
      
      if (pattern->edge_constraints & NORTH)
	if (pattern->maxi < (board_size-1) + pattern->mini)
	  pattern->maxi = (board_size-1) + pattern->mini;
      
      if (pattern->edge_constraints & SOUTH)
	if (pattern->mini > pattern->maxi - (board_size-1))
	  pattern->mini = pattern->maxi - (board_size-1);
      
      if (pattern->edge_constraints & WEST)
	if (pattern->maxj <  (board_size-1) + pattern->minj)
	  pattern->maxj = (board_size-1) + pattern->minj;
      
      if (pattern->edge_constraints & EAST)
	if (pattern->minj > pattern->maxj - (board_size-1))
	  pattern->minj = pattern->maxj - (board_size-1);
    } 
}  


#ifdef PROFILE_MATCHER
static int totals[6];
#endif


/* [i][j] contains merged entries from p[][] around i,j */
static uint32 merged_board[2][MAX_BOARD][MAX_BOARD];

/* This combines the values in p[][] around each point, to allow 
 * rapid rejection of patterns. Must be called once per board position,
 * before matchpat is invoked.
 */

void 
compile_for_match()
{
  int i,j,color;
  static int been_here=0;

  if (!been_here) {
    fixup_patterns_for_board_size(pat);
    fixup_patterns_for_board_size(conn);
    been_here=1;
  }


#ifdef PROFILE_MATCHER
  fprintf(stderr, 
	  "total %d, anchor=%d, grid=%d, edge=%d, matched=%d, accepted=%d\n",
	  totals[0], totals[1], totals[2], totals[3], totals[4], totals[5]);
#endif


#if GRID_OPT > 0
  memset(merged_board, 0, sizeof(merged_board));
  for (color=0; color<2; ++color) {
    for (i=0; i<board_size; ++i) {
      for (j=0; j<board_size; ++j) {
	uint32 this = p[i][j];  /* colour here */
	int ii;
	int shift=0;

	if (!p[i][j])
	  continue;

	/* mkpat prepares all the grid entries of the pattern assuming
	 * color==1. If this is not true, we swap all the colors on the
	 * board as we merge.
	 */
	if (this != EMPTY && color != 0)
	  this = OTHER_COLOR(this);
	  
	for (shift=0, ii = i-2; ii <= i+1 ; ++ii, shift += 8) {
	  if (ii < 0 || ii >= board_size)
	    continue;
	    
	  /* Add this one into all the nearby merged_board[][] elements. */
	  if (j > 1)  merged_board[color][ii][j-2] |= this << (shift+0);
	  if (j > 0)  merged_board[color][ii][j-1] |= this << (shift+2);
	  merged_board[color][ii][j]               |= this << (shift+4);
	  if (j < board_size-1) 
	    merged_board[color][ii][j+1] |= this << (shift+6);
	}
      }
    }
  
    /* Now go over the positions near the edge, and write in illegal
     * 'color' %11 for each location off the board. This stops a pattern
     * requiring spaces from matching near the edge.
     */

    for (i=0; i<board_size; ++i) {
      merged_board[color][0][i]            |= 0xff000000;
      merged_board[color][board_size-2][i] |= 0x000000ff;
      merged_board[color][board_size-1][i] |= 0x0000ffff;
      
      merged_board[color][i][0]  |= 0xc0c0c0c0;
      merged_board[color][i][board_size-2] |= 0x03030303;
      merged_board[color][i][board_size-1] |= 0x0f0f0f0f;
    }
  }

#endif
}



/* Compute the transform of (i,j) under transformation number trans.
 * *ti and *tj point to the transformed coordinates.
 * ORDER MATTERS : see PATTERNS for details
 *
 * There is a copy of this table in mkpat.c
 */

const int transformations[8][2][2] = {
   {{ 1,  0}, { 0,  1}}, /* linear transformation matrix */
   {{ 0,  1}, {-1,  0}}, /* rotate 90 */
   {{-1,  0}, { 0, -1}}, /* rotate board_size-10 */
   {{ 0, -1}, { 1,  0}}, /* rotate 270 */
   {{ 0, -1}, {-1,  0}}, /* rotate 90 and invert */
   {{-1,  0}, { 0,  1}}, /* flip left */
   {{ 0,  1}, { 1,  0}}, /* rotate 90 and flip left */
   {{ 1,  0}, { 0, -1}}  /* invert */
};


/* Functional version for completeness. Prefer the TRANSFORM macro
 * in patterns.h
 */

void 
transform(int i,int j,int *ti,int *tj,int trans)
{
  TRANSFORM(i,j,ti,tj,trans);
}


/* Computes the point offset by (i,j), relative to a base point (basei,basej), 
   taking into account a transformation.
*/

void 
offset(int i, int j, int basei, int basej, int *ti, int *tj, int trans)
{
  int ui,uj;
  TRANSFORM(i,j,&ui,&uj,trans);
  (*ti)=basei+ui;
  (*tj)=basej+uj;
}


/* 
 * Try all the patterns in the given array at (m,n). Invoke the callback
 * for any that matches.  Classes X,O,x,o are checked here, as is the
 * autohelper. It is up to the callback to process the other classes,
 * and any helper functions.
 */

void
matchpat(int m, int n, matchpat_callback_fn_ptr callback, int color,
	 int minwt, struct pattern *pattern) 
{
  int other = OTHER_COLOR(color);
  int ll;   /* Iterate over transformations (rotations or reflections)  */
  int k;    /* Iterate over elements of pattern */

  /* Precomuted tables to allow rapid checks on the piece at
   * the board. This table relies on the fact that color is
   * 1 or 2.
   *
   * For pattern element i,  require  (p[m][n] & andmask[i]) == valmask[i]
   *
   * .XO) For i=0,1,2,  p[m][n] & 3 is a no-op, so we check p[][] == valmask
   * x)   For i=3, we are checking that p[][] is not color, so AND color and
   *      we get 0 for either empty or OTHER_COLOR, but color if it contains
   *      color
   * o)   Works the other way round for checking it is not X.
   *
   *
   *  gcc allows the entries to be computed at run-time, but that is not ANSI.
   */
 
  static const int and_mask[2][8] = {
    /*  .      X      O     x      o      h      a      !         color */ 
    {   3,     3,     3,  WHITE, BLACK, WHITE,   3,   WHITE }, /* BLACK */
    {   3,     3,     3,  BLACK, WHITE, BLACK,   3,   BLACK }  /* WHITE */
  };

  static const int val_mask[2][8] = {
    { EMPTY, BLACK, WHITE,  0,     0,     0,   EMPTY,   0  },  /* BLACK */ 
    { EMPTY, WHITE, BLACK,  0,     0,     0,   EMPTY,   0  }   /* WHITE */
  };


  /* and a table for checking classes quickly
   * class_mask[status][color] contains the mask to look for in class.
   * ie. if  pat[r].class & class_mask[dragon[x][y].status][p[x][y]]
   * is not zero then we reject it
   * Most elements if class_mask[] are zero - it is a sparse
   * matrix containing
   *  CLASS_O in [DEAD][color]
   *  CLASS_o in [ALIVE][color]
   *  CLASS_X in [DEAD][other]
   *  CLASS_x in [ALIVE][other]
   *
   * so eg. if we have a dead white dragon, and we
   * are checking a pattern for black, then
   *  class_mask[DEAD][other]  will contain CLASS_X
   * Then we reject any patterns which have CLASS_X
   * set in the class bits.
   *
   * Making it static guarantees that all fields are
   * initially set to 0, and we overwrite the ones
   * we care about each time.
   */
  
  static int class_mask[MAX_DRAGON_STATUS][3];
  
  class_mask[DEAD][color] = CLASS_O;
  class_mask[DEAD][other] = CLASS_X;
  class_mask[ALIVE][color] = CLASS_o;
  class_mask[ALIVE][other] = CLASS_x;



  assert(m>=0 && m<board_size && n>=0 && n<board_size);

  /* Try each pattern - NULL pattern marks end of list. */
  for ( ; pattern->patn; ++pattern) { 

    /* If allpats=0, only patterns potentially larger than the largest
     * yet found are considered. This speeds the program up.
     * Generating all patterns gives useful information while tuning
     * the pattern database. We use the maxwt field in the pattern
     * database to decide whether we need to consider a pattern. 
     * If there is a helper function involved, the maximum weight has
     * to be entered into the patterns.db file. The code will assert
     * that the weight returned is never higher than maxwt.
     */
    /* jd  I added a little hack to have more pattern considered when the
     *     style fearless is enabled, thus with this option, a pattern
     *     must have a maxwt 10 points lower than minwt to be discarded.
     */       
    if ( allpats || (pattern->maxwt >= minwt) ||
	 (pattern->class & (CLASS_D | CLASS_A)) ||
	 (pattern->class & (CLASS_B | CLASS_C)) ||
	 (pattern->class & (CLASS_L)) ||
         (style & STY_FEARLESS && minwt < 75 && pattern->maxwt + 10 >= minwt))
    {

#ifdef PROFILE_MATCHER
      totals[0] += pattern->trfno;
#endif

      /* We can check the color of the anchor stone now.
       * Roughly half the patterns are anchored at each
       * color, and since the anchor stone is invariant under
       * rotation, we can reject all rotations of a wrongly-anchored
       * pattern in one go.
       *
       * Patterns are always drawn from O perspective in .db,
       * so p[m][n] is 'color' if the pattern is anchored
       * at O, or 'other' for X.
       * Since we require that this flag contains 3 for
       * anchored_at_X, we can check that
       *   p[m][n] == (color ^ anchored_at_X)
       * which is equivalent to
       *           == anchored_at_X ? other : color
       */

      if ( p[m][n] != (pattern->anchored_at_X ^ color) )
	continue;  /* does not match the anchor */
    
      /* try each orientation transformation */
      for (ll = 0; ll < pattern->trfno; ll++) {
 
#ifdef PROFILE_MATCHER
	++totals[1];
#endif


#if GRID_OPT == 1

	/* We first perform the grid check : this checks up to 16
	 * elements in one go, and allows us to rapidly reject
	 * patterns which do not match.  While this check invokes a
	 * necessary condition, it is not a sufficient test, so more
	 * careful checks are still required, but this allows rapid
	 * rejection. merged_board[][] should contain a combination of
	 * 16 board positions around m,n.  The colours have been fixed
	 * up so that stones which are 'O' in the pattern are
	 * bit-pattern %01.  
	 */
	if ( (merged_board[color-1][m][n] & pattern->and_mask[ll])
	     != pattern->val_mask[ll])
	  continue;  /* large-scale match failed */

#endif /* GRID_OPT == 1 */


#ifdef PROFILE_MATCHER
	++totals[2];
#endif

	/* Next, we do the range check. This applies the edge
	 * constraints implicitly.
	 */
	{
	  int mi,mj,xi,xj;
	  
	  TRANSFORM(pattern->mini, pattern->minj, &mi, &mj, ll);
	  TRANSFORM(pattern->maxi, pattern->maxj, &xi, &xj, ll);

	  /* transformed {m,x}{i,j} are arbitrary corners - 
	     Find top-left and bot-right. */
	  if (xi < mi) { int xx = mi; mi = xi ; xi = xx; }
	  if (xj < mj) { int xx = mj; mj = xj ; xj = xx; }

	  DEBUG(DEBUG_MATCHER, "---\nconsidering pattern '%s', rotation %d at %m. Range %d,%d -> %d,%d\n",
		pattern->name, ll, m,n, mi, mj, xi, xj);

	  /* now do the range-check */
	  if (m + mi < 0
	      || m + xi > board_size-1
	      || n + mj < 0
	      || n + xj > board_size-1)
	    continue;  /* out of range */
	}

#ifdef PROFILE_MATCHER	 
	++totals[3];
#endif

	/* Now iterate over the elements of the pattern. */
	for (k = 0; k < pattern->patlen; ++k) { /* match each point */
	  int x, y; /* absolute (board) co-ords of (transformed) pattern element */
	  int att = pattern->patn[k].att;  /* what we are looking for */


	  /* Work out the position on the board of this pattern element. */

	  /* transform pattern real coordinate... */
	  TRANSFORM(pattern->patn[k].x,pattern->patn[k].y,&x,&y,ll);
	  x+=m;
	  y+=n;

	  assert(x>=0 && x < board_size && y >= 0 && y < board_size);

	  /* ...and check that p[x][y] matches (see above). */
	  if ( (p[x][y] & and_mask[color-1][att]) != val_mask[color-1][att])
	    goto match_failed;

	  /* Check out the class_X, class_O, class_x, class_o attributes - see
	   * patterns.db and above
	   */
	  if ((pattern->class & class_mask[dragon[x][y].status][p[x][y]]) != 0)
	    goto match_failed; 

	} /* loop over elements */


	/* Make it here ==> We have matched all the elements to the board. */

#ifdef PROFILE_MATCHER
	++totals[4];
#endif


#if GRID_OPT == 2

	/* Make sure the grid optimisation wouldn't have rejected this pattern */
	ASSERT( (merged_board[color-1][m][n] & pattern->and_mask[ll]) == pattern->val_mask[ll], m,n);

#endif /* we don't trust the grid optimisation */



	/* If the pattern has an autohelper, call it to see if the pattern */
	/* must be rejected. */

	if (pattern->autohelper) {
	  int ti,tj;

	  TRANSFORM(pattern->movei, pattern->movej, &ti, &tj, ll);
	  ti += m;
	  tj += n;

	  if (!pattern->autohelper(pattern, ll, ti, tj, color))
	    goto match_failed;
	}

#ifdef PROFILE_MATCHER
	++totals[5];
#endif

	/* A match!  - Call back to the invoker to let it know. */
	callback(m, n, color, pattern, ll);

	/* We jump to here as soon as we discover a pattern has failed. */
      match_failed:
	DEBUG(DEBUG_MATCHER, 
	      "end of pattern '%s', rotation %d at %m\n---\n", 
	      pattern->name, ll, m,n);
	 
      } /* ll loop over symmetries */
    } /* if not rejected by maxwt */
  } /* loop over patterns */
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
