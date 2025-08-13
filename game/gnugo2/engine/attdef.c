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


/* ---------------------------------------------------
 *   attdef.c -- Find attacking and defending moves.
 * ----------------------------------------------------
 */

/*
 * This file contains move generators directly called by genmove().
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liberty.h"


static int active_defense(int m, int n, int color);


/* defender() looks for strings of my color which you can kill and
 * recommends defending them.
 */

int 
defender (int *i, int *j, int *val, int *equal_moves, int color,
	  int shapei, int shapej)
{
  int m, n, tval;
  int found_one=0;
  int ti, tj;
  
  TRACE("Defender is THINKING for %s!\n", color==WHITE ? "white" : "black");
  
  for (m=0;m<board_size;m++) 
    for (n=0;n<board_size;n++) {

      if (p[m][n] != color                 /* our piece */
	  || worm[m][n].origini != m       /* origin of ... */
	  || worm[m][n].originj != n       /* ... a worm */
	  || worm[m][n].attacki == -1      /* en prise   */
	  || worm[m][n].defendi == -1      /* defensible */
	  || worm[m][n].ko                /* ko left to ko_important_helper */
	  || worm[m][n].inessential
	  || !legal(worm[m][n].defendi, worm[m][n].defendj, color)) /* legal */
	continue;

      /* Considered move */
      ti = worm[m][n].defendi;
      tj = worm[m][n].defendj;
	  
      /* bonus if the string has fewer liberties, since capturing
       * leaves less aji
       */
      tval = worm[m][n].value + 3 - 2*worm[m][n].liberties;

      /* penalty if the string is nearly surrounded. */
      if (dragon[m][n].escape_route <= 4)
	tval -= 2;
      if (dragon[m][n].escape_route <= 2)
	tval -= 2;

      /* Severe penalty if we are passively running away inside
       * enemy territory.
       */
      if ((terri_color(ti, tj) == OTHER_COLOR(color))
	  && !active_defense(m, n, color)) {
	tval -= 15;
	/* And still more if the group is also classified as dead. */
	if (dragon[m][n].status == DEAD)
	  tval = tval/2;
      }
	  
      if (tval<0)
	tval=0;
	  
      if (!confirm_safety(ti, tj, color, worm[m][n].value))
	tval=0;

      TRACE("Defender found %m to defend %m for value %d\n", 
	    ti, tj, m, n, tval);
      if ((worm[m][n].size==1) 
	  && (snapback(worm[m][n].attacki, worm[m][n].attackj, 
		       worm[m][n].attacki, worm[m][n].attackj,
		       OTHER_COLOR(color)))) {
	TRACE("But it is a snapback\n");
      }
      else {
	if ((tval >= *val)
	    && (ti != shapei || tj != shapej)
	    && (shapei != -1)) {
	  /* Check if shape seer's move works too. To avoid horizon effect
	   * we must increase depth and backfill_depth so that the reading
	   * code is at the same depth as when we ran make_worms().
	   */
	  depth++;
	  backfill_depth++;
	  fourlib_depth++;
	  ko_depth++;
	  if (does_defend(shapei, shapej, m, n)) {
	    TRACE("But %m does too\n", shapei, shapej);
	    change_defense(m, n, shapei, shapej);
	  }
	  depth--;
	  backfill_depth--;
	  fourlib_depth--;
	  ko_depth--;
	}
	move_considered(worm[m][n].defendi, worm[m][n].defendj, tval);
	if (BETTER_MOVE(tval, *val, *equal_moves)) {
	  *i=worm[m][n].defendi;
	  *j=worm[m][n].defendj;
	  found_one=1;
	}
      }
    }

  return found_one;
}


/* attacker() looks for strings of your color which I can kill and
 * recommends attacking them.
 */

int 
attacker (int *i, int *j, int *val, int *equal_moves, int color,
	  int shapei, int shapej)
{
  int m, n, tval;
  int other = OTHER_COLOR(color);  /* color of the pieces we are attacking */
  int found_one=0;
  int ti, tj, acode, dcode;
  
  TRACE("Attacker is THINKING for %s!\n", color==WHITE ? "white" : "black");

  for (m=0; m<board_size; m++) 
    for (n=0; n<board_size; n++) {

      if (p[m][n] != other                 /* your piece */
	  || worm[m][n].origini != m       /* origin of ... */
	  || worm[m][n].originj != n       /* ... a worm */
	  || worm[m][n].attacki == -1      /* en prise   */
	  || worm[m][n].defendi == -1      /* defensible */
	  || worm[m][n].ko                 /* ko left to ko_important_helper */
	  || worm[m][n].inessential
	  || !legal(worm[m][n].attacki,    /* defensible */
		    worm[m][n].attackj, color))
	continue;

      /* Considered move */
      ti = worm[m][n].attacki;
      tj = worm[m][n].attackj;
      acode=worm[m][n].attack_code;
      dcode=worm[m][n].defend_code;
	  
      /* bonus if the string has fewer liberties, since capturing
       * leaves less aji.
       */
      tval = worm[m][n].value + 3 - 2*worm[m][n].liberties;

      /* penalty if the string is nearly surrounded. */
      if (dragon[m][n].escape_route <= 4)
	tval -= 2;
      if (dragon[m][n].escape_route <= 2)
	tval -= 2;

      /* Severe penalty if enemy would be passively running away
       * inside our territory.
       */
      if ((terri_color(worm[m][n].defendi, worm[m][n].defendj) == color)
	  && !active_defense(m, n, other)) {
	tval -= 15;

	/* And still more if the group is also classified as dead. */
	if (dragon[m][n].status == DEAD)
	  tval = tval/2;
      }
	  
      if (acode != 1) tval -= 10;
      if (dcode != 1) tval -= 10;

      if (tval<0)
	tval=0;

      if (!confirm_safety(ti, tj, color, worm[m][n].value))
	tval=0;

      TRACE("Attacker found %m to capture %m for value %d\n", 
	    ti, tj, m, n, tval);
      if ((worm[m][n].size==1) && snapback(ti, tj, ti, tj, color)) 
	TRACE("But it is a snapback\n");
      else {
	if ((tval >= *val)
	    && (ti != shapei || tj != shapej)
	    && (shapei != -1)) {

	  /* Check if shape seer's move works too. To avoid horizon effect
	   * we must increase depth and backfill_depth so that the reading
	   * code is at the same depth as when we ran make_worms().
	   */
	  depth++;
	  backfill_depth++;
	  fourlib_depth++;
	  ko_depth++;
	  if (does_attack(shapei, shapej, m, n)) {
	    TRACE("But %m does too\n", shapei, shapej);
	    change_attack(m, n, shapei, shapej);
	  }
	  depth--;
	  backfill_depth--;
	  fourlib_depth--;
	  ko_depth--;
	}
	  
	move_considered(worm[m][n].attacki, worm[m][n].attackj, tval);
	    
	if (BETTER_MOVE(tval, *val, *equal_moves)) {
	  *i=worm[m][n].attacki;
	  *j=worm[m][n].attackj;
	  found_one=1;
	}
      }
    }

  return found_one;
}


/* eye_finder() finds groups of either color which have 1-1/2 eyes and
 * recommends the vital point to make or kill the second eye.
 */

int
eye_finder(int *i, int *j, int *val, int *equal_moves, int color,
	   int shapei, int shapej)
{
  int tval=0;
  int m, n;
  int found_one=0;
  int ai, aj;
  int ti, tj;
  
  TRACE("Eye Finder is THINKING!\n");

  for (m=0; m<board_size; m++) 
    for (n=0; n<board_size; n++) {
      if ((dragon[m][n].origini != m)
	  || (dragon[m][n].originj != n) 
	  || (dragon[m][n].status != CRITICAL)) 
	continue;

      if ((2*dragon[m][n].genus+dragon[m][n].heyes == 3)
	  && (legal(dragon[m][n].heyei, dragon[m][n].heyej, color))) 
      {
	if ((dragon[m][n].lunchi == -1) 
	    || (worm[dragon[m][n].lunchi][dragon[m][n].lunchj].inessential))
	{
	  tval = dragon[m][n].value;
	  TRACE("move to destroy eye with value %d for dragon at %m found at %m\n", 
		tval, dragon[m][n].origini, dragon[m][n].originj,
		dragon[m][n].heyei, dragon[m][n].heyej);
	  move_considered(dragon[m][n].heyei,dragon[m][n].heyej, tval);

	  if (BETTER_MOVE(tval, *val, *equal_moves)) {
	    if ((dragon[m][n].friendi != -1)
		&& (dragon[dragon[m][n].friendi][dragon[m][n].friendj].genus>0)
		&& legal(dragon[m][n].bridgei, dragon[m][n].bridgej, color)) {
	      *i = dragon[m][n].bridgei;
	      *j = dragon[m][n].bridgej;
	      found_one = 1;
	    } else if (legal(dragon[m][n].heyei, dragon[m][n].heyej, color)) {
	      *i = dragon[m][n].heyei;
	      *j = dragon[m][n].heyej;
	      found_one = 1;
	    }
	  }
	}
	else if ((dragon[m][n].lunchi != -1) 
		 && (worm[dragon[m][n].lunchi][dragon[m][n].lunchj].defendi != -1))
	  {
	    ai = dragon[m][n].lunchi;
	    aj = dragon[m][n].lunchj;
	    tval = dragon[m][n].value + worm[ai][aj].value;
	    
	    if (dragon[m][n].color == OTHER_COLOR(color)) {
	      ti = worm[ai][aj].defendi;
	      tj = worm[ai][aj].defendj;
	      TRACE("defending %m at %m attacks %m with value %d\n", 
		    ai, aj, ti, tj, m, n, tval);
	      
	      if ((shapei != -1)
		  && ((shapei != ti) || (shapej != tj))) {

		/* Check if shape seer's move works too. To avoid
		 * horizon effect we must increase depth and
		 * backfill_depth so that the reading code is at the
		 * same depth as when we ran make_worms().
		 */
		depth++;
		backfill_depth++;
		fourlib_depth++;
		ko_depth++;
		if (does_defend(shapei, shapej, ai, aj)) {
		  TRACE("But %m does too\n", shapei, shapej);
		  ti = shapei;
		  tj = shapej;
		}
		depth--;
		backfill_depth--;
		fourlib_depth--;
		ko_depth--;
	      }

	      move_considered(ti, tj, tval);
	      if (BETTER_MOVE(tval, *val, *equal_moves)) {
		*i = ti;
		*j = tj;
		found_one = 1;
	      }
	    }
	    else {
	      ti = worm[ai][aj].attacki;
	      tj = worm[ai][aj].attackj;
	      TRACE("capturing %m at %m defends %m with value %d\n", 
		    ai, aj, ti, tj, m, n, tval);
	      
	      if ((shapei != -1)
		  && ((shapei != ti) || (shapej != tj))) {
		
		/* Check if shape seer's move works too. To avoid
		 * horizon effect we must increase depth and
		 * backfill_depth so that the reading code is at the
		 * same depth as when we ran make_worms().
		 */
		depth++;
		backfill_depth++;
		fourlib_depth++;
		ko_depth++;
		if (does_attack(shapei, shapej, ai, aj)) {
		  TRACE("But %m does too\n", shapei, shapej);
		  ti = shapei;
		  tj = shapej;
		}
		depth--;
		backfill_depth--;
		fourlib_depth--;
		ko_depth--;
	      }
	      move_considered(ti, tj, tval);
	      if (BETTER_MOVE(tval, *val, *equal_moves)) {
		*i = ti;
		*j = tj;
		found_one = 1;
	      }
	    }
	  }
      }
    }

  /* 
   *
   * Now we look for eyeshapes in the form:
   * 
   *          .                     .
   *         ..!        or         ..!
   *                                .
   *
   * Playing in the center (m,n) kills one or two eyes with sente.
   *
   */
   			    
  for (m=0; m<board_size; m++) 
    for (n=0; n<board_size; n++) {
      if ((white_eye[m][n].esize==4) 
	  && (white_eye[m][n].msize==1)
	  && (white_eye[m][n].marginal_neighbors==1)
	  && (p[m][n]==EMPTY))
	if ((dragon[white_eye[m][n].dragoni][white_eye[m][n].dragonj].genus == 1)
	    && (dragon[white_eye[m][n].dragoni][white_eye[m][n].dragonj].heyes == 2)
	    && (dragon[white_eye[m][n].dragoni][white_eye[m][n].dragonj].friendi == -1)
	    && (dragon[white_eye[m][n].dragoni][white_eye[m][n].dragonj].escape_route < 5)) {
	  TRACE("move to destroy eye with value %d for dragon at %m found at %m\n", 
		tval, white_eye[m][n].dragoni, white_eye[m][n].dragonj, m, n);
	  tval = dragon[white_eye[m][n].dragoni][white_eye[m][n].dragonj].value;
	  if (BETTER_MOVE(tval, *val, *equal_moves)) {
	    *i = m;
	    *j = n;
	    found_one = 1;
	  }
	}
      if ((black_eye[m][n].esize==4) 
	  && (black_eye[m][n].msize==1)
	  && (black_eye[m][n].marginal_neighbors==1)
	  && (p[m][n]==EMPTY))
	if ((dragon[black_eye[m][n].dragoni][black_eye[m][n].dragonj].genus == 1)
	    && (dragon[black_eye[m][n].dragoni][black_eye[m][n].dragonj].heyes == 2)
	    && (dragon[black_eye[m][n].dragoni][black_eye[m][n].dragonj].friendi == -1)
	    && (dragon[black_eye[m][n].dragoni][black_eye[m][n].dragonj].escape_route < 5)) {
	  TRACE("move to destroy eye with value %d for dragon at %m found at %m\n", 
		tval, black_eye[m][n].dragoni, black_eye[m][n].dragonj, m, n);
	  tval = dragon[black_eye[m][n].dragoni][black_eye[m][n].dragonj].value;
	  if (BETTER_MOVE(tval, *val, *equal_moves)) {
	    *i = m;
	    *j = n;
	    found_one = 1;
	  }
	}
    }
  return found_one;
}


/* Determine whether a defense move is active or passive. By active we
 * mean a move that defends by connecting to a non-dead dragon or by
 * capturing some enemy stones. By passive we mean a move that only
 * runs away.
 *
 * Awaiting suitable support from the reading code, we assume that if
 * the defense move threatens to capture some of the surrounding
 * strings, it's active and otherwise it's passive. In any case the
 * criterion should mean it's sente.
 */

static int active_defense(int m, int n, int color)
{ 
  int adj;
  int adji[MAXCHAIN];
  int adjj[MAXCHAIN];
  int adjsize[MAXCHAIN];
  int adjlib[MAXCHAIN];
  int k;
  int ti = worm[m][n].defendi;
  int tj = worm[m][n].defendj;

  if (trymove(ti, tj, color, "passive_defense", -1, -1)) {
    chainlinks(m, n, &adj, adji, adjj, adjsize, adjlib);
    for (k=0; k<adj; k++) {
      if (attack(adji[k], adjj[k], NULL, NULL)) {
	popgo();

	return 1;
      }
    }
    popgo();
  }

  return 0;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
