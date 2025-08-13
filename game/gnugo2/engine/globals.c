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

#include "../sgf/ttsgf.h"
#include "liberty.h"


/* 
 * Define all global variables used within the engine.
 */


/* The go board and position. */
board_t  p[MAX_BOARD][MAX_BOARD];
int      ko_i;
int      ko_j;

/* Hashing of positions. */
Hashtable  * movehash;
Hash_data    hashdata;
/* 
 * A study showed that this is the optimal setting for when hashing only 
 * a few functions.  Hashing all functions saves time, but wastes table
 * space.  This is bad when the reading is complicated.  This is a 
 * compromise.
 */
int hashflags = HASH_ATTACK3 | HASH_ATTACK2 | HASH_DEFEND4;

int last_move_i;           /* The position of the last move */
int last_move_j;           /* -""-                          */

board_t potential_moves[MAX_BOARD][MAX_BOARD];

int  black_captured;
int  white_captured;                /* num. of black and white stones captured */
int  black_captured_backup;
int  white_captured_backup; 

/* These variables are used by count(). */
int  lib;               /* current stone liberty written by count() */
int  libi[MAXLIBS];     /* array of liberties found : filled by count() */
int  libj[MAXLIBS];
int  size;              /* cardinality of a group : written by count() */

/* Used by reading. */
int stackp;             /* stack pointer */
int movenum;            /* movenumber */
int depth;              /* deep reading cut off */
int backfill_depth;     /* deep reading cut off */
int fourlib_depth;      /* deep reading cut off */
int ko_depth;           /* deep reading cut off */

/* Miscellaneous. */
int showstack;          /* debug stack pointer */
int showstatistics;     /* print statistics */
int allpats;            /* generate all patterns, even small ones */
int printworms;         /* print full data on each string */
int printmoyo;          /* print moyo board each move*/
int printboard;         /* print board each move */
int board_size=19;      /* board size */
int count_variations=0; /* used by decide_string */
int sgf_dump=0;         /* used by decide_string*/
int loading=0;          /* TRUE indicates last loaded move comes from file*/
int style=STY_DEFAULT;  /* style of play */

/* Various statistics are collected here. */
struct stats_data stats;

struct worm_data      worm[MAX_BOARD][MAX_BOARD];
struct dragon_data    dragon[MAX_BOARD][MAX_BOARD];
struct half_eye_data  half_eye[MAX_BOARD][MAX_BOARD];
struct eye_data       black_eye[MAX_BOARD][MAX_BOARD];
struct eye_data       white_eye[MAX_BOARD][MAX_BOARD];

int  distance_to_black[MAX_BOARD][MAX_BOARD];
int  distance_to_white[MAX_BOARD][MAX_BOARD];
int  strategic_distance_to_black[MAX_BOARD][MAX_BOARD];
int  strategic_distance_to_white[MAX_BOARD][MAX_BOARD];
int  black_domain[MAX_BOARD][MAX_BOARD];
int  white_domain[MAX_BOARD][MAX_BOARD];

int debug          = 0;
int verbose        = 0;  /* trace level                                     */
int showstack      = 0;  /* print the stack pointer (for debugging)         */
int showstatistics = 0;
int printworms     = 0;  /* print full data about each string on the board  */
int printmoyo      = 0;  /* print moyo board each move                      */
int allpats        = 0;  /* compute and print value of all patterns.        */
int color_has_played = 0;  /* whether color has placed a stone yet          */

SGFNodeP sgf_root = NULL;
char *analyzerfile = NULL;

