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
#define VERSION "2.6"



#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define BUILDING_GNUGO_ENGINE  /* bodge to access private fns and variables */
#include "../engine/liberty.h"

#include "../engine/main.h"
#include "sgf.h"
#include "ttsgf.h"
#include "../interface/interface.h"



/* The SGF file while a game is played. */
static FILE *sgfout = NULL;


/*
 * A move has been made; Write out the move and the potential moves
 * that were also considered.
 */

void 
sgf_move_made(int i, int j, int who, int value)
{
  int m,n;
  int done_label=0;
  
  if (!sgfout)
    return;

  for (m=0; m < board_size; ++m) {
    for (n=0; n<board_size; ++n) {
      if (potential_moves[m][n] > 0) {
	if (!done_label) {
	  fprintf(sgfout, "\nLB");
	  done_label=1;
	}
	fprintf(sgfout, "[%c%c:%d]", 'a'+n, 'a'+m, potential_moves[m][n]);
      }
    }
  }

  if (value)
    fprintf(sgfout, "\nC[Value of move: %d]", value);

  /* If it is a pass move */
  if ((i==j) && (i==board_size)) {
    if (board_size>19)
      fprintf(sgfout, "\n;%c[]\n", who==WHITE ? 'W' : 'B');
    else
      fprintf(sgfout, "\n;%c[tt]\n", who==WHITE ? 'W' : 'B');
  }
  else
    fprintf(sgfout, "\n;%c[%c%c]\n", who==WHITE ? 'W' : 'B', 'a'+j, 'a'+i);

  fflush(sgfout);  /* in case cgoban terminates us without notice */
}  


/*
 * Mark dead and critical dragons in the sgf file.
 */

void 
sgf_dragon_status(int i, int j, int status)
{
  if (sgfout) {
    switch(status) {
      case DEAD:
	fprintf(sgfout, "LB[%c%c:X]\n", 'a'+j, 'a'+i);
	break;
      case CRITICAL:
	fprintf(sgfout, "LB[%c%c:!]\n", 'a'+j, 'a'+i);
	break;
    }
  }
}


/*
 * Write a line to the sgf file.
 */

int 
sgf_write_line(const char * line, ...)
{
  va_list ap;
  if (!sgfout)
    return 0;

  va_start(ap, line);
  vfprintf(sgfout, line, ap);
  va_end(ap);

  return sgf_flush_file();
}


/* 
 * Write header information to the sgf file.
 */

int 
sgf_write_game_info(int bsize, int handicap, float komi, int seed, 
		    const char *gametype)
{
   if (!sgfout) 
     return 0;

   fprintf(sgfout, "(;GM[1]FF[4]");
   fprintf(sgfout, "RU[%s]", "Japanese");
   fprintf(sgfout, "SZ[%d]", bsize);
   fprintf(sgfout, "\n");

   fprintf(sgfout, "PW[GNU Go]PB[GNU Go %s %s]", VERSION, gametype);
   fprintf(sgfout, "HA[%d]", handicap);
   fprintf(sgfout, "KM[%.1f]", komi);
   fprintf(sgfout, "GN[GNU Go %s %s ", VERSION, gametype);
   fprintf(sgfout, "Random Seed %d", seed);
   fprintf(sgfout, "] ");
   fprintf(sgfout, "\n");

   return sgf_flush_file();
}


/*
 * Open the sgf file for output.  The filename "-" means stdout.
 */

int 
sgf_open_file(const char *sgf_filename)
{
   if (strcmp(sgf_filename, "-")==0)
     sgfout = stdout;
   else
     sgfout = fopen(sgf_filename, "w");

   if (!sgfout)
     return 0;
   else
     return 1;
}


/*
 * Flush buffered output to the sgf file.
 */

int 
sgf_flush_file()
{
   if (!sgfout) 
     return 0;

   fflush(sgfout);
   return 1;
}


/*
 * Close the sgf file for output.
 */

int 
sgf_close_file()
{
   if (!sgfout)
     return 0;

   fprintf(sgfout, ")\n");
   fclose(sgfout);
   sgfout=NULL;

   return 1;
}

/*
 * begin_sgfdump begins outputting all moves considered by
 * trymove and trysafe to an sgf file.
 */

void 
begin_sgfdump(const char *filename)
{
  if (!sgf_open_file(filename))
    return;

  fprintf(sgfout,"(;SZ[%d];", get_boardsize() );
  sgf_printboard(-1);
  fprintf(sgfout, "\n(;");
  sgf_dump=1;
}


/*
 * end_sgfdump ends the dump and closes the sgf file.
 */

void 
end_sgfdump()
{
  while (stackp>0)
    popgo();

  fprintf(sgfout,")\n)\n");
  fclose(sgfout);
  count_variations=0;
  sgf_dump=0;
  sgfout=NULL;
}


/* 
 * sgf_decidestring tries to attack and defend the string at (m, n),
 * and then writes the number of variations considered in the attack
 * and defence to the sgf file.
 */

void
sgf_decidestring(int m, int n, const char *sgf_output)
{
  int i, j, acode, dcode;

  if (sgf_output)
    begin_sgfdump(sgf_output);

  count_variations=1;
  acode=attack(m, n, &i, &j);
  if (acode) {
    if (acode==1)
      gprintf("%m can be attacked at %m (%d variations)\n", 
	    m, n, i, j, count_variations);
    else
      gprintf("%m can be attacked with ko (code %d) at %m (%d variations)\n", 
	    m, n, acode, i, j, count_variations);

    count_variations=1;
    dcode=find_defense(m, n, &i, &j);
    if (dcode) {
      if (dcode==1)
	gprintf("%m can be defended at %m (%d variations)\n", 
	      m, n, i, j, count_variations);
      else
	gprintf("%m can be defended with ko (code %d) at %m (%d variations)\n", 
	      m, n, dcode, i, j, count_variations);
    }
    else
      gprintf("%m cannot be defended (%d variations)\n", 
	    m, n, count_variations);
  }
  else 
    gprintf("%m cannot be attacked (%d variations)\n", 
	  m, n, count_variations);

  if (sgf_output)
    end_sgfdump();
}


/*
 * Write a comment to the SGF file.
 */

void
sgf_write_comment(const char *comment)
{
   if (!sgfout)
     return ;

   sgfAddComment(NULL, comment);
   fprintf(sgfout,"C[%s]",comment);
}


/*
 * Add a stone to the SGF file.
 */

void
sgf_set_stone(int i, int j, int who)
{
  if (sgfout)
    fprintf(sgfout, "A%c[%c%c]", who==WHITE ? 'W' : 'B', 'a'+j, 'a'+i);
}


/*
 * sgf_printboard writes the current board position to the output file.
 * The parameter next, tells whose turn it is to move.
 */

void
sgf_printboard(int next) 
{
  int i,j;
  int start = 0;

  if (!sgfout)
    return;

  /* Write the white stones to the file. */
  for (i=0; i<board_size; i++) {
    for (j=0; j< board_size; j++) {
      if (p[i][j] == WHITE) {
	if (!start) {
	  fprintf(sgfout, "AW");
	  start = 1;
	}
	fprintf(sgfout, "[%c%c]", j+'a', i+'a');
      }
    }
  }
  fprintf(sgfout, "\n");

  /* Write the black stones to the file. */
  start = 0;
  for (i=0; i<board_size; i++) {
    for (j=0; j< board_size; j++) {
      if (p[i][j] == BLACK) {
	if (!start) {
	  fprintf(sgfout, "AB");
	  start = 1;
	}
	fprintf(sgfout, "[%c%c]", j+'a', i+'a');
      }
    }
  }
  fprintf(sgfout, "\n");

  /* If no game is going on, then return. */
  if (next != WHITE && next != BLACK) 
    return;

  /* Write whose turn it is to move. */
  if (next == WHITE) 
    fprintf(sgfout, "PL[W]\n"); 
  else if (next == BLACK)
    fprintf(sgfout, "PL[B]\n"); 

  /* Mark the intersections where it is illegal to move. */
  start = 0;
  for (i=0; i<board_size; i++) {
    for (j=0; j< board_size; j++) {
      if (p[i][j] == EMPTY && !legal(i,j,next)) {
	if (!start) {
	  fprintf(sgfout, "IL");
	  start = 1;
	}
	fprintf(sgfout, "[%c%c]", j+'a', i+'a');
      }
    }
  }
  fprintf(sgfout, "\n");
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
