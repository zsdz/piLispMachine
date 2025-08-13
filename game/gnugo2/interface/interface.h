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





/*-------------------------------------------------------------------------*/
/* interface.h
	This file contains all headers for interfaces
*/
/*-------------------------------------------------------------------------*/

#ifndef _PLAY_INTERFACE_H
#define _PLAY_INTERFACE_H

#include "../engine/liberty.h"

enum testmode {UNKNOWN_TESTMODE=0, MOVE_ONLY, ANNOTATION_ONLY, BOTH, GAME};

struct SGFNode;  /* forward decl to keep gcc happy */

void play_ascii(char * filename);
void play_ascii_emacs(char * filename);
void play_gmp(void);
void play_solo(int);
void play_test(struct SGFNode *, enum testmode);
int load_sgf_file(struct SGFNode *, const char *untilstr);
void load_sgf_header(struct SGFNode *);
void load_and_analyze_sgf_file(struct SGFNode *, const char *untilstr, int benchmark);
void load_and_score_sgf_file(struct SGFNode *, const char *untilstr);


/* ---------------------------------------------------------------
 * These functions deal with the game information structure
 * --------------------------------------------------------------- */
int init_ginfo(void);
int print_ginfo(void);
int get_boardsize(void);
int set_boardsize(int);
int get_handicap(void);
int set_handicap(int);
int set_komi(float);
int get_movenumber(void);
int set_movenumber(int);
int dec_movenumber(void);
int get_tomove(void);
int set_tomove(int);
int switch_tomove(void);
int get_computer_player(void);
int set_computer_player(int);
int is_computer_player(int);
int switch_computer_player(void);
int get_seed(void);
int set_seed(int);
const char * get_outfile(void);
int set_outfile(const char *);


/* ---------------------------------------------------------------
 * These functions deal with board manipulation
 * --------------------------------------------------------------- */
board_t ** make_board(void);
int kill_board(board_t **);
int get_board(board_t **); /* size MAX_BOARD x MAX_BOARD */
int put_board(board_t **);
int clear_board(board_t **);

/* ---------------------------------------------------------------
 * These functions deal with move manipulation
 * --------------------------------------------------------------- */
int get_move(int *, int *, int);
int get_test_move(int *, int *, int);
int put_move(int, int, int);
int put_stone(int, int, int);


/* ---------------------------------------------------------------
 * These functions deal with the game option structure
 * --------------------------------------------------------------- */
int init_gopt(void);
int get_opt_quiet(void);
int set_opt_quiet(int);
int get_opt_display_board(void);
int set_opt_display_board(int);



#endif


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
