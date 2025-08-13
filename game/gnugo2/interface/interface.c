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
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "../sgf/sgf.h"

/* FIXME : this should not need to include liberty.h ! */
/* actually, I am now thinking that this file should be part of the engine */

#define BUILDING_GNUGO_ENGINE  /* bodge to access private fns and variables */
#include "../engine/liberty.h"


#include "interface.h"
#include "../engine/main.h"


/* ---------------------------------------------------------------
 * the game information structure
 * --------------------------------------------------------------- */

struct game_info {

#if 0
  /* until certain things get resolved, we use the engine's
   * globals to store these settings, rather than private
   * variables
   */
  int boardsize;
  int move_number;
#endif

  int handicap;
  float komi;
  int to_move; /* whose move it currently is*/
  int seed;	/* random seed */
  int computer_player;	/* BLACK, WHITE, or EMPTY (used instead of BOTH) */
  char outfile[128];
};


/* ---------------------------------------------------------------
 * the game option structure
 * --------------------------------------------------------------- */

struct game_options {
  int quiet;
  int display_board;
};



static struct game_info gminfo;
static struct game_info *ginfo=&gminfo;

static struct game_options gmopt;
static struct game_options *gopt=&gmopt;


/* ---------------------------------------------------------------
 * These functions deal with the game information structure
 * --------------------------------------------------------------- */

	/* initialize the structure */
int init_ginfo()
{
  board_size = 19;
  movenum = 0;

  ginfo->handicap=0;
  ginfo->komi=5.5;
  ginfo->to_move=BLACK;
  ginfo->computer_player=WHITE;
  return 1;
}

	/* print it out */
int print_ginfo()
{
  printf("Board Size:   %d\n",board_size);
  printf("Handicap      %d\n",ginfo->handicap);
  printf("Komi:         %.1f\n",ginfo->komi);
  printf("Move Number:  %d\n",movenum);

  printf("To Move:      ");
  if(ginfo->to_move==WHITE)
    printf("White\n");
  else if(ginfo->to_move==BLACK)
    printf("Black\n");
  else
    printf("Nobody\n");

  if(ginfo->computer_player==WHITE)
    printf("White\n");
  else if(ginfo->computer_player==BLACK)
    printf("Black\n");
  else if(ginfo->computer_player==EMPTY)
    printf("Both (solo)\n");
  else
    printf("Nobody\n");

  return 1;
}

	/* board size */
int 
get_boardsize()
{ 
  return board_size; 
}

int set_boardsize(int num)
{
  if(num<MIN_BOARD || num>MAX_BOARD) return 0;
  board_size = num;
  return 1;
}


	/* handicap */
int 
get_handicap()
{ 
  return ginfo->handicap; 
}

int 
set_handicap(int num)
{
  if(num<0 || num> MAX_HANDICAP) return 0;
  ginfo->handicap = num;
  return 1;
}

	/* komi */
float 
get_komi()
{ 
  return ginfo->komi; 
}

int 
set_komi(float num)
{
  /* alow any komi... why not? */
  ginfo->komi = num;
  return 1;
}


	/* move number changing */
int 
get_movenumber()
{ 
  return movenum; 
}

int 
set_movenumber(int num)
{
  if(num<0) return 0;
  movenum = num;
  return 1;
}

int 
inc_movenumber()
{ 
  return ++movenum; 
}

int 
dec_movenumber()
{ 
  return movenum == 0 ? 0 : --movenum; 
}


	/* whose move? */
int 
get_tomove()
{ 
  return ginfo->to_move; 
}

int 
set_tomove(int num)
{
  if((num != BLACK) && (num != WHITE)) 
    {
      fprintf(stderr, "set_tomove: move set to neither black nor white: %d\n",num);
      exit(EXIT_FAILURE);
    }
  ginfo->to_move= num;
  return 1;
}

int 
switch_tomove()
{
  if(ginfo->to_move == BLACK) 
    {
      ginfo->to_move = WHITE;
      return 1;
    }
  if(ginfo->to_move == WHITE)
    {
      ginfo->to_move = BLACK;
      return 1;
    }
  return 0;
}

	/* random seed */
int 
get_seed()
{ 
  return ginfo->seed; 
}

int 
set_seed(int num)
{
  if(num<0) return 0;
  ginfo->seed = num;
  return 1;
}

const char * 
get_outfile()
{ 
  return ginfo->outfile; 
}

int 
set_outfile(const char *file)
{
  strcpy(ginfo->outfile, file);
  return 1;
}


/* ---------------------------------------------------------------
 * These functions deal with board manipulation
 * --------------------------------------------------------------- */
	/* allocate a new board */
board_t ** 
make_board()
{
  board_t **board=NULL;
  board = (board_t **)malloc(MAX_BOARD*MAX_BOARD*sizeof(board_t));
  if(board==NULL)
    {
      fprintf(stderr,"make_board: failed to malloc() board!\n");
      exit(EXIT_FAILURE);
    }
  memset(board,EMPTY,MAX_BOARD*MAX_BOARD*sizeof(board_t));
  return board;
}

	/* delete a board */
int 
kill_board(board_t **board)
{
  free(board);
  return 1;
}

	/* return a copy of the board to the caller */
int 
get_board(board_t **board)
{
  if(board==NULL)
    {
      fprintf(stderr,"get_board: got NULL board!\n");
      exit(EXIT_FAILURE);
    }
  memcpy(board, p, MAX_BOARD*MAX_BOARD*sizeof(board_t));
  return 1;
}

	/* make the board that is handed in from the caller the game board */
int 
put_board(board_t **board)
{
  if(board==NULL)
    {
      fprintf(stderr,"put_board: got NULL board!\n");
      exit(EXIT_FAILURE);
    }
  memcpy(p, board, MAX_BOARD*MAX_BOARD*sizeof(board_t));
  return 1;
}

int 
clear_board(board_t **board)
{
  if(board==NULL)
    {
      memset(p,EMPTY,sizeof(p));
    }
  else
    {
      memset(board,EMPTY,MAX_BOARD*MAX_BOARD*sizeof(board_t));
    }
  return 1;
}

int 
get_computer_player()
{ 
  return ginfo->computer_player; 
}

int 
set_computer_player(int num)
{
  if((num==WHITE) || (num==BLACK) || (num==EMPTY) || (num==NONE))
    {
      ginfo->computer_player=num;
      return 1;
    }
  fprintf(stderr,"set_computer_player: invalid setting: %d\n",num);
  return 0;
}

int 
is_computer_player(int num)
{
  /* if the computer is playing both or its the computers move */
  if((ginfo->computer_player==EMPTY) ||
     (num==ginfo->computer_player))
    return 1;
  /* otherwise, FASLE... */
  return 0;
}

int switch_computer_player()
{
  /* if the computer is playing both don't switch */
  if(ginfo->computer_player==EMPTY)
    return 0;
  /* if the computer is not playing don't switch */
  if(ginfo->computer_player==NONE)
    return 0;
  if(ginfo->computer_player==WHITE)
    set_computer_player(BLACK);
  else if(ginfo->computer_player==BLACK)
    set_computer_player(WHITE);
  return 1;
}


/* ---------------------------------------------------------------
 * These functions deal with move manipulation
 * --------------------------------------------------------------- */

/* return a move for the given color and return the move value */
int 
get_move(int *x, int *y, int move)
{
  int move_val;

  /* just in case we get the wrong information... */
  if((move != WHITE) && (move != BLACK)) 
  {
    fprintf(stderr,"get_move: Illegal move color specified: %d\n",move);
    move=get_tomove();
  }

  /* generate computer move */
  move_val=genmove(x, y, move);
  updateboard(*x, *y, move);

  sgf_move_made(*x, *y, move, move_val);
  inc_movenumber();
  
  /* a move */
  if (move_val >= 0) return 1;
  /* a pass */
  else return 0;
}

/* return a test move for the given color and return the move value 
 * This move does not get registered on the board */
int 
get_test_move(int *x, int *y, int move)
{
  int move_val;

  if((move != WHITE) && (move != BLACK))
  {
    fprintf(stderr,"get_test_move: Illegal move color specified: %d\n",move);
    move=get_tomove();
  }

  /* generate computer move */
  move_val=genmove(x, y, move);

  /* a value less than 0 is a pass */
  if (move_val >= 0) return 1;
  else return 0;
}

/* make a move with legality checking */
int 
put_move(int x, int y, int move)
{
  if(!legal(x,y, move))
  {
    gprintf("Illegal move: %m\n",x,y);
    return 0;
  }
  
  updateboard(x, y, move);
  /* send the move to the file, move_val=0 */
  sgf_move_made(x, y, move, 0);
  return 1;
}



/* no legality checking here... */
int 
put_stone(int x, int y, int move)
{
  p[x][y] = move;
  sgf_set_stone(x,y,move);
  return 1;
}



/* ---------------------------------------------------------------
 * These functions deal with the game option structure
 * --------------------------------------------------------------- */

int 
init_gopt()
{
  gopt->quiet=0;
  gopt->display_board=0;
  return 1;
}

/* turn off some informational messages */
int 
get_opt_quiet()
{
  return gopt->quiet;
}

int 
set_opt_quiet(int num)
{ 
  gopt->quiet=num;
  return 1;
}

/* turn off board displays */
int 
get_opt_display_board()
{
  return gopt->display_board;
}

int 
set_opt_display_board(int num)
{ 
  gopt->display_board=num;
  return 1;
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
