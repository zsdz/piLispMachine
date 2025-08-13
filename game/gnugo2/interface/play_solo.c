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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "interface.h"

#include "../sgf/sgf.h"
#include "../sgf/sgf_properties.h"
#include "../sgf/ttsgf_read.h"
#include "../sgf/sgfana.h"
#include "../engine/liberty.h"

double gg_gettimeofday(void);

double gg_gettimeofday(void)
{
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 1.e-6*tv.tv_usec;
#else
  return time(NULL);
#endif
}

void play_solo(int moves)
{
  int pass=0; /* num. consecutive passes */
  int who=BLACK;
  int moval;
  int mymove;
  double t1, t2;
  int benchmark = moves;
  
  /* It tends not to be very imaginative in the opening,
   * so we scatter a few stones randomly to start with.
   * We add two random numbers to reduce the probability
   * of playing stones near the edge.
   */
  
  int n = 6 + 2*rand()%5;
  int i,j;

  int bsize, umove, handicap, seed;
  float komi;
  
  bsize=get_boardsize();
  komi=get_komi();
  seed=get_seed();
  handicap=get_handicap();
  mymove=get_computer_player();
  umove=OTHER_COLOR(mymove);
  
  mymove=OTHER_COLOR(umove); /* not needed, but kills an error message */
  sgf_write_game_info(bsize, handicap, komi, seed, "solo");
  
  /* for the randomly generated moves... */
  moval=0;
  
  if (board_size>6)
    do {
      do {
	i = (rand() % 4) + (rand() % (board_size-4));
	j = (rand() % 4) + (rand() % (board_size-4));
      } while (p[i][j] != EMPTY);
      
      updateboard(i,j,who);
      
      /*    if (sgfout)
	    fprintf(sgfout, "A%c[%c%c]", who==WHITE ? 'W' : 'B', 'a'+j, 'a'+i);
      */
      sgf_move_made(i, j, who, moval);
      sgfAddPlay(0,who,i, j);
      sgfAnalyzer(moval);
      who = OTHER_COLOR(who);
    } while (--n > 0);
  
  /*    if (sgfout)
	putc('\n', sgfout);
  */
  
  t1 = gg_gettimeofday();
  
  while (pass < 2 && --moves >= 0 && !time_to_die)
    {
      moval=genmove(&i, &j, who);
      updateboard(i,j,who);
      if (moval < 0) {
	++pass;
	printf("%s(%d): Pass\n", who==BLACK ? "Black" : "White",
	       get_movenumber());
      } else {
	pass=0;
	inc_movenumber();
	gprintf("%s(%d): %m\n", who==BLACK ? "Black" : "White",
		get_movenumber(), i,j);
      }
      sgf_move_made(i, j, who, moval);
      sgfAddPlay(0,who,i, j);
      sgfAnalyzer(moval);
      who=OTHER_COLOR(who);
    }
  t2 = gg_gettimeofday();
  
  /* two passes and it's over */
  
  who_wins(mymove, komi, stdout);
  if (t2 == t1) {
    printf("%.3f moves played\n", (double) (benchmark-moves));
  } else {
    printf("%.3f moves/sec\n", (benchmark-moves)/(t2-t1));
  }
  
}





/* FIXME : this should be in a separate source file, but it
 * is easier to send diffs when there are no new source
 * files to add.
 */

/* fills i,j from the property value, in GNU Go co-ords.
 * Note that GNU Go
 * uses different conventions from sgf for co-ordinates
 * been called. Returns 1 for a move, 0 for a pass
 */

static int get_moveXY(SGFPropertyP property, int *i, int *j)
{
  if (!property->value[0])
    {
      *i=board_size;
      *j=board_size;
      return 0;  /* [] is pass */
    }
  *i = property->value[1] - 'a';
  *j = property->value[0] - 'a';
  
  return (*i < 19 && *j < 19);
}



/* this used to read the sgf file character by character,
 * but now we have a full sgf parser available. Variations
 * are not supported.
 *
 * The name is historical : the file has already been read into a tree. Walk the main
 * variation, actioning the properties into the playing
 * board.
 *
 * Returns the colour of the next move to be made.
 *
 * head is a sgf tree (already read from file). until is an optional string of the form
 * either 'L12' or '120' which tells it to stop loading at that move
 * or move-number - when debugging, this
 * will be a particularly bad move, and we want to know why
 */

int load_sgf_file(SGFNodeP head, const char *untilstr)
{
  int bs, handicap;
  int next=BLACK;
  
  int untilm = -1, untiln = -1;
  int until = 9999;
  int addstone=0;          /*handicap stone detector*/
  
  SGFNodeP node;
  
  if (sgfGetIntProperty(head, "SZ", &bs))
    set_boardsize(bs);
  else
    set_boardsize(19);
  
  /* now we can safely parse the until string (which depends on board size) */
  
  if (untilstr)
    {
      if (*untilstr > '0' && *untilstr <= '9')
	{
	  until = atoi(untilstr);
	  DEBUG(DEBUG_LOADSGF, "Loading until move %d\n", until);
	}
      else
	{
	  untiln = *untilstr - 'A';
	  if (*untilstr >= 'I')
	    --untiln;
	  
	  untilm = board_size - atoi(untilstr+1);
	  DEBUG(DEBUG_LOADSGF,"Loading until move at %d,%d (%m)\n", untilm, untiln, untilm, untiln);
	}
    }
  
  if (sgfGetIntProperty(head, "HA", &handicap) && handicap > 0)
    {
      set_handicap(handicap);
      next = WHITE;
    }
  
  
  /* finally, we iterate over all the properties of all the
   * nodes, actioning them. We follow only the 'child' pointers,
   * as we have no interest in variations.
   * The sgf routines map AB[aa][bb][cc] into AB[aa]AB[bb]AB[cc]
   */
  for (node=head; node; node=node->child)
    {
      SGFPropertyP prop;
      int i,j;
      
      sgfSetLastNode(node);

      for (prop=node->prop; prop; prop=prop->next)
	{
	  DEBUG(DEBUG_LOADSGF, "%c%c[%s]\n", prop->name & 0xff, (prop->name >> 8), prop->value);
	  switch(prop->name)
	    {
	    case SGFAB:
	      get_moveXY(prop, &i, &j);
	      put_stone(i,j,BLACK);
	      addstone=1;
	      break;
	      
	    case SGFAW:
	      get_moveXY(prop, &i, &j);
	      put_stone(i,j,WHITE);
	      addstone=1;
	      break;
	      
	    case SGFPL:
	      if (!get_movenumber() && !addstone)
		sethand(get_handicap());
	      
	      if (prop->value[0] == 'w' || prop->value[0] == 'W')
		next = WHITE;
	      else
		next = BLACK;
	      break;
	      
	    case SGFW:
	    case SGFB:
	      next = prop->name == SGFW ? WHITE : BLACK;
	      if (!get_movenumber() && !addstone)
		sethand(get_handicap());
	      inc_movenumber();
	      if (get_movenumber() == until)
		return next;
	      
	      if (get_moveXY(prop, &i, &j))
		{
		  if (i == untilm && j == untiln)
		    return next;
		  
		  put_move(i,j,next);
		  next = OTHER_COLOR(next);
		  /* send the move to the file, move_val=0 */
		  /*  sgf_move_made(i, j, next, 0);        */
		  /* updateboard(i,j,next);                */
		}
	      
	      break;
	    }
	}
    }
  return next;
}

/* reads header info from sgf structure and sets the appropriate variables*/
void
load_sgf_header(SGFNodeP head)
{
  int bs;
  int handicap;
  float komi;
  
  if (sgfGetIntProperty(head, "SZ", &bs))
    set_boardsize(bs);
  else
    set_boardsize(19);
  
  if (sgfGetIntProperty(head, "HA", &handicap) && handicap > 0)
    /*    set_handicap(handicap=sethand(handicap));*/
    set_handicap(handicap);
  /*handicap stones should appear as AW,AB properties in the sgf file*/
  if (sgfGetFloatProperty(head,"KM",&komi))
    set_komi(komi);
}


/* load sgf file and run genmove() */


void 
load_and_analyze_sgf_file(SGFNodeP head, const char *untilstr, int benchmark)
{
  int i,j;
  int next;
  
  /* first we load the header to get correct board_size, 
     komi and handicap for writing */
  
  load_sgf_header(head); 
  sgf_write_game_info(board_size, get_handicap(),
		      get_komi(),get_seed(), "load and analyze");
  next = load_sgf_file(head, untilstr);
  
  if (benchmark)
    {
      int z;
      for (z=0; z < benchmark; ++z)
	{
	  genmove(&i, &j, next);
	  next = OTHER_COLOR(next);
	}
    }
  else
    {
      genmove(&i, &j, next);
      
      if (i != board_size) 
	{
	  gprintf("%s move %m\n", next == WHITE ? "white (o)" : "black (X)", i,j);
	  put_move(i, j, next);
	}
      else
	{
	  gprintf("%s move : PASS!\n", next == WHITE ? "white (o)" : "black (X)");
	  sgf_move_made(i, j, next, 0);
	}
    }
}

/* load sgf file and score the game
   untilstr:
   end  - finish the game by selfplaying from the end of the file until two passes
   last - estimate territorial balance at the end of the of the file
   move - load file until move is reached and estimate territorial balance
*/


void 
load_and_score_sgf_file(SGFNodeP head, const char *untilstr)
{
  int i, j, moval, pass, wt, bt;
  int until;
  float score;
  float komi;
  float result;
  char *tempc=NULL;
  char dummy;
  char text[250];
  char winner;
  int next;
  pass=0;

  assert(head);
  assert(sgf_root);
  load_sgf_header(head);
  sgf_write_game_info(board_size, get_handicap(),get_komi(),get_seed(), "load and score");
  next=load_sgf_file(head, untilstr);
  komi=get_komi();
  
  until=atoi(untilstr);
  if (!strcmp(untilstr,"end"))
  {
    until=9999;
    do {
      moval=genmove(&i, &j, next);
      updateboard(i,j,next);
      if (moval>=0) {
	pass=0;
	gprintf("%d %s move %m\n",get_movenumber(),
		next == WHITE ? "white (o)" : "black (X)", i,j);
      } else {
	++pass;
	gprintf("%d %s move : PASS!\n",get_movenumber(), 
		next == WHITE ? "white (o)" : "black (X)");
      }
      sgf_move_made(i, j, next, moval);
      sgfAddPlay(0,next,i, j);
      sgfAnalyzer(moval);
      inc_movenumber();
      next = OTHER_COLOR(next);
    } while ((get_movenumber()<=until)&(pass<2));
  if (pass>=2)
    {
      /*count actually owned territory*/
      evaluate_territory(&wt,&bt);
      score=white_captured+bt-black_captured-wt-komi;
      if (score>0)
	{
	  sprintf(text,"Black wins by %1.1f points\n", score);
	  winner='B';
	}
      else if (score<0)
	{
	  sprintf(text,"White wins by %1.1f points\n", -1.0 * score);
	  winner='W';
	}
      else
	{
	  sprintf(text,"Tie\n");
	  winner='0';
	}
      fputs(text,stdout);
      sgfShowTerritory(0);
      sgf_write_comment(text);
      if (sgfGetCharProperty(head, "RE", &tempc))
	{
	  if (sscanf(tempc,"%1c%f",&dummy,&result)==2)
	    {
	      fprintf(stdout,"Result from file: %1.1f\n",result);
	      fputs("GNU Go result and result from file are ",stdout);
	      if ((result==abs(score)) && (winner==dummy))
		fputs("identical\n",stdout);
	      else
		fputs("different\n",stdout);
	      
	    } else
	      if (tempc[2]=='R')
		{
		  fprintf(stdout,"Result from file: Resign\n");
		  fputs("GNU Go result and result from file are ",stdout);
		  if (tempc[0]==winner)
		    fputs("identical\n",stdout);
		  else
		    fputs("different\n",stdout);
		}
	}
      sgfWriteResult(score);
      return;
    }
  }
  moval=genmove(&i, &j, next);
  sgfAnalyzer(moval);
  score=terri_eval[BLACK]-terri_eval[WHITE]-komi;
  sprintf(text,
"Black moyo: %3i\n\
White moyo: %3i\n\
Black territory: %3i\n\
White territory: %3i\n\
Dead white stones: %3i\n\
Dead black stones: %3i\n\
%s seems to win by %1.1f points\n",
	  moyo_eval[BLACK],moyo_eval[WHITE],
	  terri_eval[BLACK],terri_eval[WHITE],
	  white_captured,
	  black_captured,
	  (score>0)?("BLACK"):((score<0)?("WHITE"):("NOBODY")),
	  (score<0)?(-1*score):(score)
	  );
  if (strcmp(untilstr,"last"))
      sgfSetLastNode(sgfStartVariant(0));
  sgfShowMoyo(SGF_SHOW_COLOR,0);
  sgf_write_comment(text);
  fputs(text,stdout);
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
