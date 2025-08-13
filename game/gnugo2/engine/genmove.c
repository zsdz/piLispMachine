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
#include <string.h>

#include "liberty.h"

#include "../sgf/ttsgf.h"
#include "../sgf/sgfana.h"

#define MAXTRY 400

void sgfShowConsideredMoves(void);

/* 
 * Generate computer move for COLOR.
 *
 * Return the generated move in (*i, *j).
 */
  
int
genmove(int *i, int *j, int color)
{
  int val;
  int equal_moves;
  int m,n;
  int shapei, shapej;

  /* prepare our table of moves considered */
  memset(potential_moves, 0, sizeof(potential_moves));

  /* Reset all the statistics for each move. */
  stats.nodes = 0;

  /* prepare matchpat for use */
  compile_for_match();

#if HASHING
  /* Initialize things for hashing of positions. */
  hashtable_clear(movehash);
  board_to_position(p, ko_i, ko_j, /*color,*/ &(hashdata.hashpos));
  hashdata.hashval = board_hash(p, ko_i, ko_j/*, color*/);
#endif

  /* no move is found yet. */
  *i = -1;  
  *j = -1;  
  val = -1; 

  /* Used to give every equal move the same chance to be chosen. */
  equal_moves = 2;

  {
    /* Don't print reading traces during make_worms and make_dragons unless 
       the user really wants it (verbose == 3). */

    int save_verbose=verbose;
    if ((verbose==1) || (verbose==2)) --verbose;
    if (make_worms()) {
      make_dragons();
    }
    verbose=save_verbose;
  }

  /* Map the moyos for both sides. */
  make_moyo(color);
  
  if (printworms) {
    show_dragons();
  }
  else if (verbose) {
    for (m=0;m<board_size;m++)
      for (n=0;n<board_size;n++)
	if ((p[m][n])
	    && (worm[m][n].origini==m) 
	    && (worm[m][n].originj==n) 
	    && (worm[m][n].cutstone)) 
	{
	  TRACE("%scutting stones found at %m\n",
		worm[m][n].cutstone==2 ? "potential " : "", m,n); 
	}
  }	  
  
  if (printmoyo) {
    print_moyo(color);
  }
   
  if (printboard)
    showboard(printboard);
  
  assert(stackp == 0);  /* stack empty */

  /* Try to find nice fuseki moves. */

  if (fuseki(i, j, &val, &equal_moves, color))
  {
      TRACE("Fuseki Player likes %m with value %d\n", *i, *j, val);
      if(analyzerflag&ANALYZE_RECOMMENDED)
          sgfBoardText(0,*i,*j,"F");
  }
  assert(stackp == 0);

  /* Try to catch your stones in semeai. */
  if (semeai(i, j, &val, &equal_moves, color))
  {
     TRACE("Semeai Player likes %m with value %d\n", *i, *j, val);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"S");
  }
  assert(stackp == 0);  /* stack empty */
  
  /* pattern matcher */
  if (shapes(i, j, &val, &equal_moves, color))
  {
     TRACE("Shape Seer likes %m with value %d\n", *i, *j, val);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"P");
  }
  shapei = *i;
  shapej = *j;

  assert(stackp == 0);  /* stack empty */

  /* Try to catch your stones in a regular fight (semeai already done above). */
  if (attacker(i, j, &val, &equal_moves, color, shapei, shapej))
  {
     TRACE("Attacker likes %m with value %d\n", *i, *j, val);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"A");
  }
  assert(stackp == 0);  /* stack empty */
  
  /* Try to save my stones if you can attack them. */
  if (defender(i, j, &val, &equal_moves, color, shapei, shapej))
  {  TRACE("Defender likes %m with value %d\n", *i, *j, val);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"D");
  }
  assert(stackp == 0);  /* stack empty */

  /* Look for eye stealing moves. */
  if (eye_finder(i, j, &val, &equal_moves, color, shapei, shapej))
  {  TRACE("Eye Finder  likes %m with value %d\n", *i, *j, val);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"E");
  }
  assert(stackp == 0);  /* stack empty */
  
  /* If no move found yet, revisit any semeai and change the
   * status of the opponent group from DEAD to UNKNOWN, then 
   * run shapes again. This may turn up a move.
   */
  if ((val < 0) 
      && revise_semeai(color) 
      && shapes(i, j, &val, &equal_moves, color))
    {
       TRACE("Upon reconsideration Shape Seer likes %m with value %d\n",
	  *i, *j, val); 
       if(analyzerflag&ANALYZE_RECOMMENDED)
         sgfBoardText(0,*i,*j,"p");
    }
  if ((val < 0) && fill_liberty(i, j, &val, color))
  {
     TRACE("Filling a liberty at %m\n", *i, *j);
     if(analyzerflag&ANALYZE_RECOMMENDED)
       sgfBoardText(0,*i,*j,"L");
  }
  if (style & STY_FEARLESS) {

    /* If there is a big move
          and the temperature is low (i.e. no fighting)
          and the recommended move is somewhere near the big move
       then do the big move instead. */
    if (very_big_move[0] 
	&& val < min(75, very_big_move[0]) 
	&& (max(abs(*i - very_big_move[1]), 
		abs(*j - very_big_move[2])) >= 3)) 
    {
      if (printmoyo & 128)
        gprintf("genmove() recommended %m val %d ", *i, *j, val);
      else
        TRACE("genmove() recommended %m val %d ", *i, *j, val);

      *i = very_big_move[1];
      *j = very_big_move[2];
      val = very_big_move[0];
      if (printmoyo & 128)
        gprintf("but we play %m with new val %d, t %d\n",
		*i, *j, val,terri_eval[0]);
      else
        TRACE("but we play %m with new val %d, t %d\n",
	      *i, *j, val,terri_eval[0]);
      if(analyzerflag&ANALYZE_RECOMMENDED)
        sgfBoardText(0,*i,*j,"B");
    }
  }

  /* If no move is found then pass. */
  if (val < 0) {
    TRACE("I pass.\n");
    *i = board_size;
    *j = board_size;
  } else
    TRACE("genmove() recommends %m with value %d\n", *i, *j, val);
  
  /* If statistics is turned on, this is the place to show it. */
  if (showstatistics) {
    char text[120];
    sprintf(text,"Nodes: %d\n", stats.nodes);
    sgfAddComment(0,text);
    gprintf("Nodes: %d\n", stats.nodes);
      
  }

  return val;
  
}  /* end genmove */




/* This is called for each move which has been considered. For
 * debugging purposes, we keep a table of all the moves we
 * have considered.
 */

void 
move_considered(int i, int j, int val)
{
  if ((val > potential_moves[i][j])) {
    potential_moves[i][j] = val;
  }
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

