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



/* This file describes the compiled form of the pattern database.
 * mkpat is used to compile the source file patterns.db into
 * an intermediate file patterns.c which defines data structures
 * describing the patterns.
 * This is also used to store a half-eye pattern, which does not
 * use all the fields, but that doesn't matter since there are only
 * a few of them
 */

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
 *  *    Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define GRID_OPT 0
#endif

#ifndef GRID_OPT
#error GRID_OPT should be defined as 0, 1 or 2
#endif


/* a 32-bit unsigned int */
typedef unsigned int uint32;

/* this trick forces a compile error if ints are not at least 32-bit */
struct _unused_patterns_h {
  int unused[ sizeof(uint32) >= 4 ? 1 : -1];
};

/* transformation stuff */
extern const int transformations[8][2][2];

/* a macro (inline) version of the transform function */

#define TRANSFORM(i,j,ti,tj,trans) \
do { \
  *ti = transformations[trans][0][0] * (i) + transformations[trans][0][1] * (j); \
  *tj = transformations[trans][1][0] * (i) + transformations[trans][1][1] * (j); \
} while(0)


struct pattern; /* forward reference to keep gcc happy */

/* this is the type of a function which the matcher can
 * call to evaluate the score of a move.
 * parameters :
 *   pattern and rotation are the current pattern being considered
 *   ti, tj : IN = posn of the 7,8 or 9 marker
 *            OUT = recommended move
 * return value : weight of move, or 0 if match failed            
 */
 
typedef int (*pattern_helper_fn_ptr)(struct pattern *, int rotation, int ti, int tj, int color);


/* each pattern is compiled into a sequence of these elements.
 * Each describes a relative x,y from the pattern origin,
 * and a description of what should be there.
 * Current attributes are
 *  0 = .
 *  1 = X
 *  2 = O
 *  3 = x
 *  4 = o
 *  5 = h  (half-eye only)
 *  6 = a  (half-eye only)
 *  7 = !  (connection only)
 */

#define ATT_dot 0
#define ATT_X   1
#define ATT_O   2
#define ATT_x   3
#define ATT_o   4
#define ATT_h   5
#define ATT_a   6
#define ATT_not 7

/* and pattern classes */
#define CLASS_s 0x1   /* move is a sacrifice */
#define CLASS_O 0x2   /* O stones must be alive or unknown */
#define CLASS_o 0x4   /* O stones must be dead or unknown */
#define CLASS_X 0x8   /* X stones must be alive or unknown */
#define CLASS_x 0x10  /* X stones must be dead or unknown */
#define CLASS_D 0x20  /* move is defensive : update worm data */
#define CLASS_C 0x40  /* move connects two worms */ 
#define CLASS_n 0x80  /* X could also make this move if we do not */
#define CLASS_B 0x100 /* move breaks connection between enemy worms */
#define CLASS_A 0x200 /* change attack point of a worm */
#define CLASS_L 0x400 /* run pattern regardless of weight */

/* methods of assistance */

#define NO_ASSISTANCE 0
#define WIND_ASSISTANCE 1
#define MOYO_ASSISTANCE 2

/* directions for applying edge-constraints */

#define NORTH 1
#define SOUTH 2
#define EAST 4
#define WEST 8


struct patval {int x, y, att;};


/* each pattern as a whole is compiled to an instance of this structure. */

struct pattern {
  struct patval *patn;  /* array of elements */
  int patlen;           /* number of elements */
  int trfno;            /* either number of transformations (rotations and reflections) */
                        /* or transformation number of this instance */
  const char *name;     /* short description of pattern (optional) */

  int mini, minj;       /* min and max (relative to anchor) extent of ... */
  int maxi, maxj;       /* ...the pattern */
  int edge_constraints; /* and combinations of NORTH,EAST etc. for edges */

  int movei, movej;     /* position of the suggested move (relative to anchor) */

#ifdef GRID_OPT
  uint32 and_mask[8], val_mask[8];  /* for each rotation, masks for a 4x4 grid around anchor */
#endif

  /* the following apply to patterns only, not half-eyes */

  int patwt;            /* score for pattern, if matched */
  int maxwt;            /* max weight this pattern can generate */
  int assistance;       /* method of assistance */
  int *assist_params;   /* parameters used by assistance */
  int class;            /* classification of pattern */
  int obonus;           /* bonus to be applied if O weak */
  int xbonus;           /* bonus to be applied if X weak */
  int splitbonus;       /* bonus to be applied if move splits or
			 * connects in a larger sense */
  int minrand;          /* minimum random adjustment */
  int maxrand;          /* maximum random adjustment */
  
  pattern_helper_fn_ptr helper;  /* helper function, or NULL */
  pattern_helper_fn_ptr autohelper;  /* automatically generated helper */
                                     /* function, or NULL */

  int anchored_at_X;    /* 3 if the pattern has 'X' at the anchor posn */

};

int compute_score(int i,int j,int color,struct pattern *pat);
int compute_score_sub(int i,int j,int color,struct pattern *pat,int *upower,int *mypower);




/* helper fns */

#define DECLARE(x) int x(struct pattern *pattern, int transformation, int ti, int tj, int color)

DECLARE(double_atari_helper);
DECLARE(break_out_helper);
DECLARE(basic_cut_helper);
DECLARE(diag_miai_helper);
DECLARE(diag_miai2_helper);
DECLARE(connect_under_helper);
DECLARE(save_on_second_helper);
DECLARE(hane_on_4_helper);
DECLARE(alive11_helper);
DECLARE(ko_important_helper);
DECLARE(edge_ko_important_helper);
DECLARE(double_attack_helper);
DECLARE(tobi_connect_helper);
DECLARE(attach_connect_helper);
DECLARE(take_valuable_helper);
DECLARE(common_geta_helper);
DECLARE(atari_attack_helper);
DECLARE(eye_stealing_helper);
DECLARE(sente_hane_helper);
DECLARE(fight_ko_helper);
DECLARE(fight_ko2_helper);
DECLARE(try_to_rescue_helper);
DECLARE(clamp1_helper);
DECLARE(clamp2_helper);
DECLARE(jump_out_helper);
DECLARE(jump_out_far_helper);
DECLARE(double_connection_helper);
DECLARE(double_does_break_helper);
DECLARE(double_break_helper);
DECLARE(peep_connect_helper);
DECLARE(break_hane_helper);
DECLARE(two_half_helper);
DECLARE(trap_helper);
DECLARE(tiger_defense_helper);
DECLARE(backfire_helper);
DECLARE(two_bird_helper);
DECLARE(kosumi_tesuji_helper);
DECLARE(break_connection_helper);
DECLARE(cap3_helper);
DECLARE(knight_helper);
DECLARE(defend_solid_helper);
DECLARE(defend_bamboo_helper);
DECLARE(high_handicap_helper);
DECLARE(draw_back_helper);
DECLARE(threaten_helper);
DECLARE(no_capture_helper);
DECLARE(safe_connection_helper);
DECLARE(threaten_capture_helper);
DECLARE(wedge_helper);
DECLARE(wedge2_helper);
DECLARE(weak_connection_helper);
DECLARE(sente_cut_helper);
DECLARE(wide_break_helper);
DECLARE(double_attack2_helper);
DECLARE(only_atari_helper);
DECLARE(setup_double_atari_helper);
DECLARE(urgent_connect_helper);
DECLARE(urgent_connect2_helper);
DECLARE(detect_trap_helper);
DECLARE(double_threat_connection_helper);
DECLARE(sacrifice_another_stone_tesuji_helper);
DECLARE(throw_in_atari_helper);
DECLARE(sente_extend_helper);
DECLARE(straight_connect_helper);
DECLARE(block_to_kill_helper);
DECLARE(extend_to_kill_helper);
DECLARE(revise_value_helper);
DECLARE(push_to_capture_helper);
DECLARE(special_half_eye_helper);
DECLARE(block_attack_defend_helper);
DECLARE(pull_back_catch_helper);
DECLARE(second_line_block_helper);
DECLARE(not_lunch_helper);
DECLARE(invade3sp_helper);
DECLARE(indirect_helper);
DECLARE(dont_attack_helper);
DECLARE(atari_ladder_helper);
DECLARE(safe_cut_helper);
DECLARE(waist_helper);
DECLARE(ugly_cutstone_helper);
DECLARE(cutstone2_helper);
DECLARE(real_connection_helper);
DECLARE(connect_to_live_helper);
DECLARE(stupid_defense_helper);
DECLARE(threaten_connect_helper);
DECLARE(threaten_connect1_helper);
DECLARE(make_eye_helper);
DECLARE(make_eye1_helper);
DECLARE(make_eye2_helper);
DECLARE(make_eye3_helper);
DECLARE(expand_eyespace_helper);
DECLARE(cap_kills_helper);
DECLARE(chimera_helper);
DECLARE(semimarginal_helper);
DECLARE(eye_on_edge_helper);
DECLARE(make_trouble_helper);
DECLARE(move_half_eye_helper);

/* autohelper fns */
int seki_helper(int ai, int aj);

/* pattern arrays themselves */
extern struct pattern pat[];
extern struct pattern conn[];

/* not sure if this is the best way of doing this, but... */
#define UNUSED(x)  x=x

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
