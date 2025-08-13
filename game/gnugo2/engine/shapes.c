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
#include "patterns.h"

/* to collect values across callbacks */
static int shapes_i;
static int shapes_j;
static int shapes_val;
static int shapes_equal_moves;


/* 
 * This callback is invoked for each matched pattern 
 */

static void
shapes_callback(int m, int n, int color, struct pattern *pattern, int ll)
{
  int bonus;         /* bonus awarded if dragon is weak */
  int ti, tj, tval;  /* trial move and its value */
  int bval = 0;      /* cut value */
  int cval = 0;      /* connection value */
  int bcval;         /* combined cut and connection value */
  int oweak = 0;
  int xweak = 0;

  /* for D class patterns, points to a (color) string under attack */
  int defi = -1;
  int defj = -1;

  /* for A class patterns, points to a (color) string under attack */
  int atti = -1;
  int attj = -1;

  /* Dragons for C class patterns */
  int first_c_dragoni = -1;
  int first_c_dragonj = -1;
  int second_c_dragoni = -1;
  int second_c_dragonj = -1;
  
  /* Dragons for B class patterns */
  int first_b_dragoni = -1;
  int first_b_dragonj = -1;
  int second_b_dragoni = -1;
  int second_b_dragonj = -1;
  
  /* Some classes of pattern need some extra stuff calculated at each
   * element.
   */
  if ((pattern->class & (CLASS_A | CLASS_B | CLASS_C | CLASS_D | CLASS_L)) != 0
      || pattern->obonus || pattern->xbonus)
  {
    int k;
    int other = OTHER_COLOR(color);

    /* Match each point. */
    for (k = 0; k < pattern->patlen; ++k) { 
      int x, y; /* absolute (board) co-ords of (transformed) pattern element */

      /* all the following stuff (currently) applies only at occupied cells */
      if (pattern->patn[k].att == ATT_dot)
	continue;

      /* transform pattern real coordinate */
      TRANSFORM(pattern->patn[k].x, pattern->patn[k].y, &x, &y, ll);
      x += m;
      y += n;

      if (pattern->obonus || pattern->xbonus) {
	if (p[x][y]
	    && (dragon[x][y].safety==CRITICAL)
	    && ((dragon[x][y].status==UNKNOWN)
		|| dragon[x][y].status==CRITICAL)) {
	  if (p[x][y]==color)
	    oweak = 1;
	  else
	    xweak = 1;
	}
      }

      if ((pattern->class & CLASS_D) && (p[x][y]==color) && (defi==-1))
	if (worm[x][y].attacki != -1) {
	  defi = worm[x][y].origini;
	  defj = worm[x][y].originj;
	}
	
      if ((pattern->class & CLASS_A) && (p[x][y]==other) && (atti==-1)) {
	if ((worm[x][y].attacki != -1) && (worm[x][y].defendi != -1)) {
	  atti = worm[x][y].origini;
	  attj = worm[x][y].originj;
	}
      }
	
      if ((pattern->class & CLASS_C) && (p[x][y]==color)) {
	if (first_c_dragoni==-1) {
	  first_c_dragoni = dragon[x][y].origini;
	  first_c_dragonj = dragon[x][y].originj;
	}
	else if ((second_c_dragoni==-1)
		 && ((dragon[x][y].origini != first_c_dragoni) 
		     || (dragon[x][y].originj != first_c_dragonj)))
	{
	  second_c_dragoni = dragon[x][y].origini;
	  second_c_dragonj = dragon[x][y].originj;
	}
	if (0)
	  TRACE("connect dragons: (%s) %m, %m\n", pattern->name,
		first_c_dragoni, first_c_dragonj,
		second_c_dragoni, second_c_dragonj);
      }

      if ((pattern->class & CLASS_B) && (p[x][y]==other)) {
	if (first_b_dragoni==-1) {
	  first_b_dragoni = dragon[x][y].origini;
	  first_b_dragonj = dragon[x][y].originj;
	}
	else if ((second_b_dragoni==-1) 
		 && ((dragon[x][y].origini != first_b_dragoni) 
		     || (dragon[x][y].originj != first_b_dragonj)))
	{
	  second_b_dragoni = dragon[x][y].origini;
	  second_b_dragonj = dragon[x][y].originj;
	}
	if (0) 
	  TRACE("cut dragons: (%s) %m, %m\n", pattern->name,
		first_b_dragoni, first_b_dragonj,
		second_b_dragoni, second_b_dragonj);
      }
    } /* loop over elements */
  } /* if we need to loop over the elements */



  /* Pick up the location of the move */
  TRANSFORM(pattern->movei, pattern->movej, &ti, &tj, ll);
  ti += m;
  tj += n;


  /* For sacrifice patterns, the survival of the stone to be played is
   * not checked (but it still needs to be legal). Otherwise we
   * discard moves which can be captured. 
   */
  if (! (pattern->class & CLASS_s)) {
    if (!safe_move(ti, tj, color)) {
      TRACE("  move at %m wasn't safe, discarded\n", ti, tj);
      goto match_failed;
    }
  }
  else {
    if (! legal(ti, tj, color)) {
      TRACE("  move at %m wasn't legal, discarded\n", ti, tj);
      goto match_failed;
    }
  }
  
  /* For class n patterns, the pattern is contingent on an opponent
   * move at * not being captured.
   */
  if (pattern->class & CLASS_n) {
    if (!safe_move(ti, tj, OTHER_COLOR(color))) {
      TRACE("  opponent can't play safely at %m, move discarded\n", ti, tj);
      goto match_failed;
    }
  }
  

  /* and work out the value of this move */
  if (pattern->helper) {
    /* ask helper function to consider the move */
    DEBUG(DEBUG_HELPER, "  asking helper to consider '%s'+%d at %m\n", 
	  pattern->name, ll, ti,tj);
    tval = pattern->helper(pattern, ll, ti, tj, color);
    
    if ((tval > 0) || CLASS_B || CLASS_C)
      {
	TRACE("helper likes pattern '%s' weight %d at %m\n",
	      pattern->name, tval, ti,tj);
	
	/* Check maxweight supplied in the patterns.db file. */
	if (tval > pattern->maxwt)
	  assert(tval <= pattern->maxwt);  
      }
    else {
      DEBUG(DEBUG_HELPER,"  helper does not like pattern '%s' at %m\n", 
	    pattern->name, ti,tj);
      goto match_failed;  /* pattern matcher does not like it */
    }
  }
      
  else { /* no helper */
    tval = compute_score(ti,tj,color,pattern);
  }

  if (pattern->class & CLASS_C) {
    if (second_c_dragoni == -1)
      cval = 0;
    else {
      cval = connection_value(first_c_dragoni, first_c_dragonj, 
			      second_c_dragoni, second_c_dragonj, ti, tj);
      if ((dragon[first_c_dragoni][first_c_dragonj].friendi == -1) 
	  || (dragon[second_c_dragoni][second_c_dragonj].genus >
	      dragon[first_c_dragoni][first_c_dragonj].genus)
	  || ((dragon[second_c_dragoni][second_c_dragonj].genus ==
	       dragon[first_c_dragoni][first_c_dragonj].genus) 
	      && (dragon[second_c_dragoni][second_c_dragonj].heyes >
		  dragon[first_c_dragoni][first_c_dragonj].heyes))) {
	int i, j;
	for (i=0; i<board_size; i++)
	  for (j=0; j<board_size; j++) 
	    if ((dragon[i][j].origini == first_c_dragoni) 
		&& (dragon[i][j].originj == first_c_dragonj)) {
	      dragon[i][j].friendi = second_c_dragoni;
	      dragon[i][j].friendj = second_c_dragonj;
	      dragon[i][j].bridgei = ti;
	      dragon[i][j].bridgej = tj;
	    }
      }
      if ((dragon[second_c_dragoni][second_c_dragonj].friendi == -1) 
	  || (dragon[first_c_dragoni][first_c_dragonj].genus >
	      dragon[second_c_dragoni][second_c_dragonj].genus)
	  || ((dragon[first_c_dragoni][first_c_dragonj].genus ==
	       dragon[second_c_dragoni][second_c_dragonj].genus) 
	      && (dragon[first_c_dragoni][first_c_dragonj].heyes >
		  dragon[second_c_dragoni][second_c_dragonj].heyes))) {
	int i, j;
	for (i=0; i<board_size; i++)
	  for (j=0; j<board_size; j++) 
	    if ((dragon[i][j].origini == second_c_dragoni) 
		&& (dragon[i][j].originj == second_c_dragonj)) {
	      dragon[i][j].friendi = first_c_dragoni;
	      dragon[i][j].friendj = first_c_dragonj;
	      dragon[i][j].bridgei = ti;
	      dragon[i][j].bridgej = tj;
	    }
      }
    }
    if (0) {
      TRACE("connect dragons: (%s) %m, %m, %d\n", pattern->name,
	    first_c_dragoni, first_c_dragonj,
	    second_c_dragoni, second_c_dragonj, cval);
    }
  }

  if (pattern->class & CLASS_B) {
    if (second_b_dragoni == -1)
      bval = 0;
    else {
      bval = connection_value(first_b_dragoni, first_b_dragonj, 
			      second_b_dragoni, second_b_dragonj, ti, tj);
      if ((dragon[first_b_dragoni][first_b_dragonj].friendi == -1) 
	  || (dragon[second_b_dragoni][second_b_dragonj].genus >
	      dragon[first_b_dragoni][first_b_dragonj].genus)
	  || ((dragon[second_b_dragoni][second_b_dragonj].genus ==
	       dragon[first_b_dragoni][first_b_dragonj].genus) 
	      && (dragon[second_b_dragoni][second_b_dragonj].heyes >
		  dragon[first_b_dragoni][first_b_dragonj].heyes))) {
	int i, j;
	for (i=0; i<board_size; i++)
	  for (j=0; j<board_size; j++) 
	    if ((dragon[i][j].origini == first_b_dragoni) 
		&& (dragon[i][j].originj == first_b_dragonj)) {
	      dragon[i][j].friendi = second_b_dragoni;
	      dragon[i][j].friendj = second_b_dragonj;
	      dragon[i][j].bridgei = ti;
	      dragon[i][j].bridgej = tj;
	    }
      }
      if ((dragon[second_b_dragoni][second_b_dragonj].friendi == -1) 
	  || (dragon[first_b_dragoni][first_b_dragonj].genus >
	      dragon[second_b_dragoni][second_b_dragonj].genus)
	  || ((dragon[first_b_dragoni][first_b_dragonj].genus ==
	       dragon[second_b_dragoni][second_b_dragonj].genus) 
	      && (dragon[first_b_dragoni][first_b_dragonj].heyes >
		  dragon[second_b_dragoni][second_b_dragonj].heyes))) {
	int i, j;
	for (i=0; i<board_size; i++)
	  for (j=0; j<board_size; j++) 
	    if ((dragon[i][j].origini == second_b_dragoni) 
		&& (dragon[i][j].originj == second_b_dragonj)) {
	      dragon[i][j].friendi = first_b_dragoni;
	      dragon[i][j].friendj = first_b_dragonj;
	      dragon[i][j].bridgei = ti;
	      dragon[i][j].bridgej = tj;
	    }
      }
    }
    if (0) {
      TRACE("cut dragons: (%s) %m, %m, %d\n", pattern->name,
	    first_b_dragoni, first_b_dragonj,
	    second_b_dragoni, second_b_dragonj, bval);
    }
  }
  
  if (pattern->class & (CLASS_B | CLASS_C)) {
    bcval = max(bval, cval);
    /* For a BC class pattern, give some bonus when it simultaneously
     * cuts and connects.
     */
    if (bval>0 && cval>0)
      bcval += max(0, (min(bval, cval) - 50) / 5);
    
    tval = min(tval, bcval);
  }

  /* Now apply bonuses. */
  bonus=0;
  if (oweak)
    bonus += pattern->obonus;
  if (xweak)
    bonus += pattern->xbonus;

  /* Add some bonus if meta_connect is ok. */
  if (meta_connect(ti, tj, color) > 0 )
    bonus += pattern->splitbonus;

  tval += bonus;

  /* If using -a, want to see all scores even if not -v */
  if (allpats || verbose) {
    int upower, mypower;
    int deltamoyo;

    switch (pattern->assistance) {
    case NO_ASSISTANCE:
      if (pattern->minrand == pattern->maxrand)
	TRACE("pattern '%s'+%d valued %d at %m; bonus=%d\n",
	      pattern->name, ll, tval + pattern->minrand, ti, tj, bonus);
      else
	TRACE("pattern '%s'+%d valued %d-%d at %m; bonus=%d\n",
	      pattern->name, ll, tval + pattern->minrand,
	      tval + pattern->maxrand, ti, tj, bonus);
      break;
    case WIND_ASSISTANCE:
      testwind(ti, tj, color, &upower, &mypower);
      if (pattern->minrand == pattern->maxrand)
	TRACE("pattern '%s'+%d valued %d at %m; bonus=%d, power=%d (mine), %d (yours) \n",
	      pattern->name, ll, tval + pattern->minrand, ti, tj, bonus,
	      mypower, upower);
      else
	TRACE("pattern '%s'+%d valued %d-%d at %m; bonus=%d, power=%d (mine), %d (yours) \n",
	      pattern->name, ll, tval + pattern->minrand,
	      tval + pattern->maxrand, ti, tj, bonus, mypower, upower);
      break;
    case MOYO_ASSISTANCE:
      deltamoyo = delta_moyo_simple(ti, tj, color);
      if (pattern->minrand == pattern->maxrand)
	TRACE("pattern '%s'+%d valued %d at %m; bonus=%d, deltamoyo=%d\n",
	      pattern->name, ll, tval + pattern->minrand, ti, tj, bonus,
	      deltamoyo);
      else
	TRACE("pattern '%s'+%d valued %d-%d at %m; bonus=%d, deltamoyo=%d\n",
	      pattern->name, ll, tval + pattern->minrand,
	      tval + pattern->maxrand, ti, tj, bonus, deltamoyo);
      break;
    }
  }


  /* If the pattern is class D we can move the point of defense
   * for a string of ours so that defender() will return the
   * recommended move instead of some other stupid one.
   */
  if ((pattern->class & CLASS_D) && (defi != -1)) {
    TRACE("pattern of type D found at %m\n", ti, tj);
    if (does_defend(ti, tj, defi, defj)) {
      change_defense(defi, defj, ti, tj);
      TRACE("Defence point of %m is %m due to pattern '%s'\n", defi, defj,
	    ti, tj, pattern->name);
    }
  }

  /* If the pattern is class A we can move the point of attack
   * for an enemy string so that attacker() will return the
   * recommended move instead of some other stupid one.
   */
  if (pattern->class & CLASS_A && (atti != -1)) {
    TRACE("pattern of type A found at %m\n", ti, tj);
    if (does_attack(ti, tj, atti, attj)) {
      change_attack(atti, attj, ti, tj);
      TRACE("Attack point of %m is %m due to pattern '%s'\n", atti, attj,
	    ti, tj, pattern->name);
    }
  }

  /* having made it here, we have made it through all the extra checks */

  /* Should we record the considered value with or without random bonus? */
  move_considered(ti, tj, tval);

  /* Add pattern dependent random variation. */
  tval += pattern->minrand;
  if (pattern->minrand != pattern->maxrand)
    tval += rand() % (pattern->maxrand - pattern->minrand + 1);
  
  if (tval >= shapes_val && confirm_safety(ti, tj, color, 0))
    if (BETTER_MOVE(tval, shapes_val, shapes_equal_moves)) {
      shapes_i = ti;
      shapes_j = tj;
    }
  
  if (style & STY_FEARLESS)
    search_big_move(ti,tj,color,tval);

match_failed:
  ;
}


/*
 * Match all patterns on all positions.  
 *
 * This function is one of the basic move generators called by genmove().
 */

int 
shapes(int *i, int *j, int *val, int *equal_moves, int color)
{
  int m, n; /* iterate over all board posns */

  TRACE("Shape Seer is THINKING of a move for %s!\n",
	color == WHITE ? "white" : "black");

  shapes_val = -1;
  shapes_equal_moves = 2;

  for (m = 0; m < board_size; m++)
    for (n = 0; n < board_size; n++)
      if (p[m][n])
	matchpat(m, n, shapes_callback, color, shapes_val, pat);

  if (shapes_val <= 0)
    return 0;

  if ((shapes_val > *val) 
      || ((shapes_val == *val)
	  && ((rand() % (shapes_equal_moves + *equal_moves -2)) 
	      < (shapes_equal_moves -1)))) 
  {
    if (shapes_val == *val)
      *equal_moves += shapes_equal_moves - 2;
    else {
      *val = shapes_val;
      *equal_moves = 2;
    }
    *i = shapes_i;
    *j = shapes_j;
    return 1;
  }

  return 0;
}
	





/* Computes the value of a move at i,j for (color) and pattern *pat,
 * taking assistance methods into account.
 */

int
compute_score(int i, int j, int color, struct pattern *patt)
{
  int result = patt->patwt;

  switch (patt->assistance) {
  case NO_ASSISTANCE:
    break;
  case WIND_ASSISTANCE:
    result += wind_assistance(i, j, color, patt->assist_params);
    break;
  case MOYO_ASSISTANCE:
    result += moyo_assistance(i, j, color, patt->assist_params);
    break;
  }
  return result;
}


int
wind_assistance(int i, int j, int color, int *params)
{
  int upower, mypower;
  int ucutoff  = abs(params[0]);
  int uvalue   = params[1];
  int mycutoff = abs(params[2]);
  int myvalue  = params[3];
  
  testwind(i, j, color, &upower, &mypower);
  if (upower > ucutoff)
    upower = ucutoff;
  if (mypower > mycutoff)
    mypower = mycutoff;
  return upower*uvalue + mypower*myvalue;
}


/* testwind(i,j,*upower,*mypower) is used to fine-tune the pattern database.
 * The idea is that the value of a pattern may depend on local strength of
 * each player. (i,j) are the coordinates of a potential move. *upower and
 * *mypower will point to values measuring the opponent's power and my power
 * in the vicinity of (i,j). This data may be used by matchpat to evaluate
 * the move.
 *
 * To calculate a color's power near (i,j), we sum (6-d) where d is the
 * distance of a stone to (i,j), where the sum is over all stones of the
 * given color with d<6;
 *
 */

static int cached_at_move[MAX_BOARD][MAX_BOARD];
static int cached_upower[MAX_BOARD][MAX_BOARD];
static int cached_mypower[MAX_BOARD][MAX_BOARD];

/* 
 * This should be called after backstepping to invalidate old values. 
 */
void
clear_wind_cache(void)
{
  int i,j;

  for (i=0; i<MAX_BOARD; i++)
    for (j=0; j<MAX_BOARD; j++)
      cached_at_move[i][j] = -1;
}


void 
testwind(int i, int j, int color, int *upower, int *mypower)
{
  int mini, maxi, minj, maxj;
  int m, n;
  int di, dj;

  if(cached_at_move[i][j] == movenum) {
    *upower = cached_upower[i][j];
    *mypower = cached_mypower[i][j];
    return;
  }
  
  DEBUG(DEBUG_WIND,"testing power of %d at %m\n", color, i, j);
  *upower = 0;
  *mypower = 0;
  mini = max(0, i-5);
  maxi = min(board_size-1, i+5);

  for (m=mini; m<=maxi; m++) {
    di = abs(i-m);
    minj = max(0, j-5+di);
    maxj = min(board_size-1, j+5-di);
    for (n=minj; n<=maxj; n++) {
      if (p[m][n] == EMPTY)
	continue;

      dj = abs(j-n);
      if (p[m][n] == color) {
	DEBUG(DEBUG_WIND,
	      "my power has contribution of %d from stone at %m\n",
	      6-di-dj, m, n);
	*mypower += (6-di-dj);
      }
      else {
	DEBUG(DEBUG_WIND,
	      "opponent's power has contribution of %d from stone at %m\n",
	      6-di-dj, m, n);
	*upower+=(6-di-dj);
      }
    }
  }

  cached_at_move[i][j] = movenum;
  cached_upower[i][j] = *upower;
  cached_mypower[i][j] = *mypower;
}


int
moyo_assistance(int i, int j, int color, int *params)
{
  int moyocutoff = params[0];
  int moyovalue  = params[1];
  int deltamoyo = delta_moyo_simple(i, j, color);
  
  if (deltamoyo > moyocutoff)
    deltamoyo = moyocutoff;
  return deltamoyo * moyovalue;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
