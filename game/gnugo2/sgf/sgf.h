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
/* sgf.h
	This file contains all headers for the sgf interface
*/
/*-------------------------------------------------------------------------*/

#ifndef _SGF_H
#define _SGF_H

int sgf_open_file(const char *);
int sgf_write_game_info(int, int, float, int, const char *);
int sgf_write_line(const char *, ...);
int sgf_close_file(void);
int sgf_flush_file(void);

void begin_sgfdump(const char *filename);
void end_sgfdump(void);
void sgf_decidestring(int i, int j, const char *sgf_output);

void sgf_move_made(int i, int j, int who, int value);
void sgf_set_stone(int i, int j, int who);

void sgf_write_comment(const char *comment);
void sgf_printboard(int next);

#endif
