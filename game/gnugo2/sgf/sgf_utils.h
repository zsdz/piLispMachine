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
/* sgf_utils.h
	This file contains all headers for the sgf utilities
*/
/*-------------------------------------------------------------------------*/

#include "ttsgf.h"

#ifndef _SGF_UTILS_H
#define _SGF_UTILS_H

short str2short(char *);
int get_moveX(SGFPropertyP);
int get_moveY(SGFPropertyP);
int get_moveX_a(SGFPropertyP);
int get_moveY_a(SGFPropertyP);
int is_pass(int, int);
int is_pass_a(char, char);
int show_sgf_properties(SGFNodeP);
int show_sgf_tree(SGFNodeP);
int is_markup_node(SGFNodeP);
int is_move_node(SGFNodeP);
int find_move(SGFNodeP);
enum testmode guess_mode_from_sgf_comment(SGFNodeP);

#endif
