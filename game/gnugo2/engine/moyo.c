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




/*
 * Module to evaluate approximate territories and moyos.
 *
 * It uses a dilation/erosion algorithm with:
 * 5 dilations/21 erosions for potential territory
 * 5 dilations/10 erosions for moyo
 * 4 dilations/ 0 erosions for "area ownership"
 * 
 * These values have the following characteristics:
 * - to have accurate evaluation, dead stones must be "removed" 
 * - 5 dilations to be sure to have influence on very large territory
 * - 21 erosions induce that "isolated" stones don't make territory, 
 *   they just limit the expansion of other color influence.
 * - in the end of the game, the evaluated territory matches the actual score.
 *
 * See the Texinfo documentation (Moyo) for more information about the
 * functions in this file.
 */

/* -m [level]
 * use bits for printing these debug reports :
 * 0x001 = ascii printing of territorial evaluation (5/21)
 * 0x002 = table of delta_terri values
 * 0x004 = ascii printing of moyo evaluation (5/10)
 * 0x008 = table of delta_moyo values
 * 0x010 = ascii printing of area (weak groups?) (4/0)
 * 0x020 = list of area characteristics
 * 0x040 = table of meta_connect values
 *
 * (example: -m 0x9)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include <ctype.h>
#include <assert.h>
#include <string.h>		/* for memset */

#include "liberty.h"
#include "interface.h"
#include "ttsgf.h"
#include "sgfana.h"

/* enabling color, the same as in showbord.c  */
#ifdef CURSES

#ifdef _AIX
#define _TPARM_COMPAT
#endif
#include <curses.h>
#include <term.h>
#endif

#ifdef CURSES
/* terminfo attributes */
static char *setaf;		/* terminfo string to set color */
static int max_color;		/* terminfo max colour */

static void init_curses(void)
{
  static int init = 0;

  /* compiler is set to make string literals  const char *
   * But system header files dont prototype things correctly.
   * These are equivalent to a non-const string literals
   */
  static char setaf_literal[]="setaf";
  static char colors_literal[]="colors";
  static char empty_literal[]="";
  if (init)
    return;
  
  init = 1;

  setupterm(NULL, 2, NULL);
  setaf = tigetstr(setaf_literal);
  if (!setaf)
    setaf = empty_literal;
  max_color = tigetnum(colors_literal) - 1;
  if (max_color < 1)
    max_color = 1;
  else if (max_color > 30)
    max_color = 30;
}


#define DRAW_COLOR_CHAR(c,x) do {  \
  fprintf(stderr, " %s%c", tparm(setaf,(c), 0, 0, 0, 0, 0, 0, 0, 0),(x)); \
  fputs(tparm(setaf,max_color, 0, 0, 0, 0, 0, 0, 0, 0), stderr); \
} while(0)

#elif defined(ANSI_COLOR)

#define max_color 7

#define DRAW_COLOR_CHAR(c,x)  fprintf(stderr, " \033[%dm%c\033[0m", 30+(c), (x))

#else

#define DRAW_COLOR_CHAR(c,x)  fprintf(stderr, " %c", (x))

#endif


/* linux console :
 *  0=black
 *  1=red
 *  2=green
 *  3=yellow/brown
 *  4=blue
 *  5=magenta
 *  6=cyan
 *  7=white
 */


typedef int goban_t[MAX_BOARD][MAX_BOARD];

typedef unsigned long binmap_t[MAX_BOARD + 2];


/* Shadow is the state of the goban after a dilation, there is one
 * Shadow for black and one for white. shadow are made of : 
 *   left :            image of stones one intersection to the left.
 *   right, up, down : idem to the other directions 
 *   tot :             is the image of the current color influence
 *                     after dilation in all directions. in the stack,
 *                     mostack[WHITE-1][0].tot is the current position 
 *                     of white stones really on the goban
 *                     (before any dilation).  
 *   zone :            is the result of a crude dilation of one intersection 
 *                     in the four directions. Useful to get the limitation 
 *                     of influence when dilating the other color. 
 *                     It's different from "tot", "tot" doesn't dilate
 *                     against opposite color, "zone" does.
 */


typedef struct {
  binmap_t left;
  binmap_t right;
  binmap_t up;
  binmap_t down;
  binmap_t tot;
  binmap_t zone;
} Shadow;


/* Structure for evaluating area ownership (aka looking for weak groups).
 *
 * Currently, this is used as a sorted stack, and area_grid[19][19] 
 * contains the relevant reference of the stack
 *
 * FIXME : - using a [200] stack could cause an overflow ? I think
 * that the max group number is 190 on a goban.
 * - using a [361] stack use memory without real need
 * - using malloc to construct a real size stack is slow 
 */
#define GROUP_STACK_MAX 190

/* size of the stack for delta_moyo_color, delta_area_color, 
 * and delta_terri_color (this is intentionally left low to
 * minimise memory usage. When the stack is full, the older
 * values are supressed.
 */

#define COLOR_STACK_SIZE 80

static goban_t board_cache[COLOR_STACK_SIZE];
static int     board_cache_free;
static int     cache_movei[COLOR_STACK_SIZE];
static int     cache_movej[COLOR_STACK_SIZE];

/* number of dilation for area ownership, must be <= MAX_DILAT */
#define TERRI_DILATIONS  5
#define TERRI_EROSIONS  21

/* 4 or 5 */
/* must MOYO_EROSIONS <= TERRI_EROSIONS if MOYO_DILATIONS != TERRI_DILATIONS*/
#define MOYO_DILATIONS   5
#define MOYO_EROSIONS   10

#define AREA_DILATIONS   4
#define AREA_EROSIONS    0

/* number of bonus point for each group connected and opponent group cut */
#define GROUP_CONNECT_BONUS 15
#define GROUP_CUT_BONUS     10

typedef struct {
  int m, n;			/* origin point of the area */
  int color;			/* color of owner : WHITE or BLACK or EMPTY */
  int stone;			/* nb of stones of estimated group */
  int space;			/* nb of EMPTY around group */
  int tag;			/* for future usage : how the group is tagged ("weak", ...) */
  int d1m, d1n, d2m, d2n;       /* some dragons of the area */
  int d3m, d3n, d4m, d4n, d5m, d5n; 
} area_t;


static area_t   area_array[GROUP_STACK_MAX];
static int      areas_level;
static goban_t  area_grid;
static int      num_groups[2];  /* number of groups of each colors */
static int      nb_weak[2];     /* number of weak groups of each color */

/* for caching already calculated values of delta_area, in meta_connect() */
static goban_t area_cached_move[2];
static goban_t delta_area_cache[2];
static goban_t board_area_move[2];	/* cache board for delta_area_color */


static unsigned long mask, bord;	/* mask=011..10  bord=100..01 */
static Shadow empty_shadow;	/* to quickly initialize Shadow values */
static binmap_t mask_binmap;	/* full of stone, for some binary operators */

/* the stack of successive iterations of the dilation, it is useful just
 * before erosion for computing territorial weight of each intersection 
 */
static Shadow mostack[2][TERRI_DILATIONS + 1]; 

/* for caching already calculated values of delta_moyo */
static goban_t delta_moyo_move_cache[2];
static goban_t delta_moyo_value_cache[2];
static goban_t board_moyo_move[2];	/* cache board for delta_moyo_color */

/* for caching already calculated values of delta_terri */
static goban_t terri_cached_move[2];
static goban_t board_terri_move[2];	/* cache board for delta_terri_color */
static goban_t delta_terri_cache[2];

/* static int ikomi;               komi */

/* The boards in which the dilations/erosions are made. */
static goban_t empty_goban;
static goban_t start_goban;
static goban_t moyo_goban;
static goban_t terri_goban;
static goban_t d_moyo_goban;
static goban_t d_terri_goban;


int terri_eval[3];
int terri_test[3];
int moyo_eval[3];
int moyo_test[3];
int very_big_move[3];


/**********************************/
/* functions declared in liberty.h :
 * void init_moyo(void);
 * int make_moyo(int color);
 *
 * int delta_moyo(int ti, int tj, int color);
 * int delta_moyo_simple(int ti, int tj, int color);
 * int diff_moyo(int ti, int tj, int color);
 * int moyo_color(int m,int n);
 * int delta_moyo_color(int ti,int tj, int color, int m, int n);
 * int average_moyo_simple(int ti, int tj, int color);
 * void print_moyo(int color);
 *
 * int delta_terri(int ti, int tj, int color);
 * int diff_terri(int ti, int tj, int color);
 * int terri_color(int m,int n);
 * int delta_terri_color(int ti,int tj, int color, int m, int n);
 *
 * int area_stone(int m,int n);
 * int area_space(int m,int n);
 * int area_color(int m,int n);
 * int area_tag(int m,int n);
 * void set_area_tag(int m,int n,int tag);
 * int delta_area_color(int ti,int tj, int color, int m, int n);
 *
 * int meta_connect(int ti, int tj,int color);
 * void search_big_move(int ti, int tj,int color,int val);
 * int number_weak(int color);
 */

/* static int influence(binmap_t * s); */
static int compute_delta_moyo(int x, int y, int color, int b);
static int compute_delta_terri(int x, int y, int color);
static int compute_delta_area(int x, int y, int color);
static void dilate(goban_t goban, int dilations);
static void erode(goban_t gob);
static void count_goban(goban_t gob, int *score, int b);
static void compute_ownership_old(goban_t gob, goban_t grid,
				  area_t * area, int *alevel);
static void compute_ownership(goban_t gob, goban_t grid,
			      area_t * area, int *alevel);
static void compute_ownership_test(goban_t gob, goban_t grid,
				   area_t * area, int *alevel);
static void spread_area(goban_t goban, goban_t area_index, 
			area_t * area, int area_id,
			int color, int i, int j);
static void spread_area_test(goban_t goban, goban_t area_index, 
			     area_t * areas, int area_id,
			     int color, int i, int j);
static void combine_area(int area2, goban_t area_index, area_t * areas,
			 int area1, int i, int j);
static void combine_area2(int *cpt, int area2, goban_t area_index,
			  int area1, int i, int j);
static void count_groups(area_t areas[], int num_areas, int n_groups[2]);
static void test_weak(goban_t grid, area_t * area, int tw);
static int free_board(int color);

/* These will soon be obsolete */
static void clear_moyo(int i);
static void sum_shadow(Shadow * s);
static void stack2goban(int num, goban_t gob);
static void load_dragon_color_tot(Shadow * s, board_t what);
static void or_binmap(binmap_t v1, binmap_t v2);
static void inv_binmap(binmap_t v1);
static void zone_dilat(int ns);
static void dilation(int ns);
static void erosion(goban_t gob);
static void diffuse_area(goban_t gob, goban_t grid, 
                         area_t * area, int colval, int gridval, int i, int j);

/***************************************/
static void print_ascii_moyo(goban_t gob);
static void print_ascii_area(void);
static void print_txt_area(void);
static void print_delta_moyo(int color);
static void print_delta_terri(int color);
static void print_txt_connect(int color);
/* static void print_delta_terri_color(int ti,int tj,int color); */



/*
 * Initialize the moyo module.  This is called only once.
 */

/* FIXME: Make this a statis function. */
void
init_moyo(void)
{
  int i, j;
  binmap_t empty;

  for (i = 0; i < MAX_BOARD + 2; i++)
    empty[i] = 0;

  memcpy(empty_shadow.left, empty, sizeof(binmap_t));
  memcpy(empty_shadow.right, empty, sizeof(binmap_t));
  memcpy(empty_shadow.up, empty, sizeof(binmap_t));
  memcpy(empty_shadow.down, empty, sizeof(binmap_t));
  memcpy(empty_shadow.tot, empty, sizeof(binmap_t));
  memcpy(empty_shadow.zone, empty, sizeof(binmap_t));

  /* Clear the generic empty goban */
  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++)
      empty_goban[i][j] = 0;

  memset(area_array, 0, sizeof(area_array));

  /*
   *    mask = 011..10  bord = 100..01
   */
  mask = 0;
  for (i = 1; i <= board_size; i++)
    mask += (1 << i);
  bord = mask + 1 + 2;
  for (i = 1; i <= board_size; i++)
    mask_binmap[i] = mask;


  for (i = 0; i < MAX_BOARD ; i++)
    for (j = 0; j < MAX_BOARD ; j++) {
      board_moyo_move[0][i][j] = -1;
      board_moyo_move[1][i][j] = -1;
      board_terri_move[0][i][j] = -1;
      board_terri_move[1][i][j] = -1;
      board_area_move[0][i][j] = -1;
      board_area_move[1][i][j] = -1;
      area_cached_move[0][i][j] = 0;
      area_cached_move[1][i][j] = 0;
      delta_area_cache[0][i][j] = 0;
      delta_area_cache[1][i][j] = 0;
      delta_moyo_move_cache[0][i][j] = 0;
      delta_moyo_move_cache[1][i][j] = 0;
      delta_moyo_value_cache[0][i][j] = 0;	
      delta_moyo_value_cache[1][i][j] = 0;	
      terri_cached_move[0][i][j] = 0;
      terri_cached_move[1][i][j] = 0;
      delta_terri_cache[0][i][j] = 0;	
      delta_terri_cache[1][i][j] = 0;	
    }

  for (i = 0; i < COLOR_STACK_SIZE; i++) {
    cache_movei[i] = 0;
    cache_movej[i] = 0;
  }

  board_cache_free = 0;
}



/*
 * Main moyo function. Return the white-black territorial balance
 * and put values in :
 *
 * 5/21 : after 21 erosions + prisoners => territory approxim.
 * terri_eval[WHITE] : white territory
 * terri_eval[BLACK] : black territory
 * terri_eval[0] : difference in territory (color - other_color)
 *
 * these 3 are initialized to zero by make_moyo() :
 * terri_test[WHITE] : territory evaluation from delta_terri()
 * terri_test[BLACK] : ...
 * terri_test[0] :     Return of delta_terri(), difference in territorial
 *                     evaluation between terri_test and first call
 *                     to make_moyo the evaluation is for(color - other_color)
 *
 * 5/10 or 4/8 ...
 * after 10 erosions, no prisoners, but with count of DEAD stones 
 * => for moyo approximation & delta moyo
 * moyo_eval[WHITE] : first moyo evaluation from make_moyo()
 * moyo_eval[BLACK] : ...
 * moyo_eval[0] : difference in moyo (color - other_color)
 *
 * These 3 ones are initialized to zero by make_moyo() :
 * moyo_test[WHITE] : moyo evaluation from delta_moyo()
 * moyo_test[BLACK] : ...
 * moyo_test[0] :    return of delta_moyo(), difference in moyo between 
 *                   test moyo and first moyo evaluation (color - other_color)
 *
 * 4/0 : used for group analysis, see area_grid & area_array
 */

/*
 * make_moyo() returns terri_eval[0] and computes terri_eval & moyo_eval.
 *
 * It also computes the area_grid for area ownership & weak group search 
 */

int
make_moyo(int color)
{
  int i, j;

  /* FIXME: We don't have a good value for board_size in init_moyo() */
  mask = 0;
  for (i = 1; i <= board_size; i++)
    mask += (1 << i);
  bord = mask + 1 + 2;

  very_big_move[0]=0; 

  for (i = 0; i <= TERRI_DILATIONS; i++)
    clear_moyo(i);

  /*
   * if (board_cache_free>69) gprintf("bs %d\n",board_cache_free);
   */
  board_cache_free = 0;

  terri_eval[WHITE] = terri_eval[BLACK] = 0;
  terri_test[WHITE] = terri_test[BLACK] = terri_test[0] = 0;
  moyo_eval[WHITE] = moyo_eval[BLACK] = 0;
  moyo_test[WHITE] = moyo_test[BLACK] = moyo_test[0] = 0;

  /* OLD CODE */
  load_dragon_color_tot(&(mostack[WHITE - 1][0]), WHITE);
  load_dragon_color_tot(&(mostack[BLACK - 1][0]), BLACK);
  /* NEW CODE */
  /* Initialize the result. */
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      if (p[i][j] == BLACK && dragon[i][j].safety != DEAD)
	start_goban[i][j] = 128;
      else if (p[i][j] == WHITE && dragon[i][j].safety != DEAD)
	start_goban[i][j] = -128;
      else
	start_goban[i][j] = 0;
    }
  }

  /* If MOYO_DILATIONS != TERRI_DILATIONS, we need to compute a little
   * more (at least until some modifications are made in dilat(), and
   * change the function.  Since it's #define, gcc should optimize
   * function size.
   */
  if (MOYO_DILATIONS == TERRI_DILATIONS) {
    if (AREA_DILATIONS < TERRI_DILATIONS) {
      memcpy(moyo_goban, start_goban, sizeof(goban_t));
      dilate(moyo_goban, AREA_DILATIONS);

      /* Compute area_grid and the area_array. */
      compute_ownership_test(moyo_goban, area_grid, area_array, &areas_level);
      count_groups(area_array, areas_level, num_groups);
      for (i = 1; i <= TERRI_DILATIONS; i++)
	clear_moyo(i);
    }

    memcpy(moyo_goban, start_goban, sizeof(goban_t));
    dilate(moyo_goban, TERRI_DILATIONS);

    /* Compute area_grid and the area_array. */
    if (AREA_DILATIONS == TERRI_DILATIONS) {
      compute_ownership_test(moyo_goban, area_grid, area_array, &areas_level);
      count_groups(area_array, areas_level, num_groups);
    }
    for (i = 1; i <= MOYO_EROSIONS; i++)
      erode(moyo_goban);

    /* Compute moyo. */
    count_goban(moyo_goban, moyo_eval, -1);

    /* Make last part of erode. */
    memcpy(terri_goban, moyo_goban, sizeof(goban_t));
    for (i = MOYO_EROSIONS; i < TERRI_EROSIONS; i++)
      erode(terri_goban);

    /* Compute territory. */
    count_goban(terri_goban, terri_eval, -1);
  } else {
    /* MOYO_DILATIONS != TERRI_DILATIONS */

    if (AREA_DILATIONS != TERRI_DILATIONS
	&& AREA_DILATIONS != MOYO_DILATIONS) {

      /* Compute area_grid and the area_array */
      memcpy(moyo_goban, start_goban, sizeof(goban_t));
      dilate(moyo_goban, AREA_DILATIONS);

      compute_ownership_test(moyo_goban, area_grid, area_array, &areas_level);
      count_groups(area_array, areas_level, num_groups);

      for (i = 1; i <= TERRI_DILATIONS; i++)
	clear_moyo(i);
    }

    memcpy(terri_goban, start_goban, sizeof(goban_t));
    dilate(terri_goban, TERRI_DILATIONS);

    /* Compute area_grid and the area_array */
    if (AREA_DILATIONS == TERRI_DILATIONS) {
      compute_ownership_test(terri_goban, area_grid, area_array, &areas_level);
      count_groups(area_array, areas_level, num_groups);
    }
    for (i = 1; i <= TERRI_EROSIONS; i++)
      erode(terri_goban);

    /* Compute territory. */
    count_goban(terri_goban, terri_eval, -1);

    /* Now redo it for moyo_eval. */
    /* OLD CODE */
    for (i = 1; i <= MOYO_DILATIONS; i++)
      clear_moyo(i);

    memcpy(moyo_goban, start_goban, sizeof(goban_t));
    dilate(moyo_goban, MOYO_DILATIONS);

    /* Compute area_grid and the area_array. */
    if (AREA_DILATIONS == MOYO_DILATIONS) {
      compute_ownership_test(moyo_goban, area_grid, area_array, &areas_level);
      count_groups(area_array, areas_level, num_groups);
    }
    for (i = 1; i <= MOYO_EROSIONS; i++)
      erode(moyo_goban);

    /* Compute moyo. */
    count_goban(moyo_goban, moyo_eval, -1);
  }

  /* Look for weak groups. */
  nb_weak[0] = nb_weak[1] = 0;
  for (i = 1; i <= areas_level; i++) {
    test_weak(area_grid, area_array, i);
    if (area_array[i].tag >= 3)
      nb_weak[area_array[i].color - 1]++;
  }

  terri_eval[WHITE] += black_captured;	/* no komi added */
  terri_eval[BLACK] += white_captured;

  moyo_eval[0] = moyo_eval[color] - moyo_eval[OTHER_COLOR(color)];
  terri_eval[0] = terri_eval[color] - terri_eval[OTHER_COLOR(color)];

  return terri_eval[0];
}


/* 
 * delta_moyo returns the value of delta_moyo_simple + possible bonus
 * for a large scale cut/connect
 */

int
delta_moyo(int ti, int tj, int color)
{
  if (meta_connect(ti, tj, color) > 0)
    return delta_moyo_simple(ti, tj, color) + meta_connect(ti, tj, color);
  else
    return delta_moyo_simple(ti, tj, color);
}


/* 
 * The returned value is the difference in moyo evaluation induced by
 * the tested move. If the move creates an isolated group (number of 
 * stone <= 2 and  number of space around <= 10), the returned value
 * is zero.
 */

int
delta_moyo_simple(int ti, int tj, int color)
{
  if (delta_moyo_move_cache[color - 1][ti][tj] != movenum) {
    if (meta_connect(ti, tj, color) < 0)
      delta_moyo_value_cache[color - 1][ti][tj] = 0;
    else
      delta_moyo_value_cache[color - 1][ti][tj] = compute_delta_moyo(ti, tj, color, -1);
    delta_moyo_move_cache[color - 1][ti][tj] = movenum;
  }
  return delta_moyo_value_cache[color - 1][ti][tj];
}


/*
 * Look for territorial oriented moves, try to evaluate the value of
 * a move at (ti,tj) by this rule :
 *((terri. gain if color plays move
 *  + terri. gain if other_color plays move) * a + b + val*c)
 *
 * The a,b,c values are empiric, and can be modified for tuning, see below
 * for actual values.
 */

void
search_big_move(int ti, int tj, int color, int val)
{
  int dt;

  if (terri_color(ti, tj) == EMPTY ) {
    if (terri_eval[0] > -20)
      dt = diff_terri(ti, tj, color);
    else
      dt = diff_terri(ti, tj, color) / 2 + delta_moyo(ti, tj, color) / 2 ;

    if (dt >= 4) {
      dt = (int) (dt * 0.9 + 15 + val * 0.7);
      if (dt > very_big_move[0]) {
	very_big_move[0] = dt;
	very_big_move[1] = ti;
	very_big_move[2] = tj;
      }
    }
  }
}


/* ---------------------------------------------------------------- */


int
delta_terri(int ti, int tj, int color)
{
  /* delta_terri automatically computes the color board if necessary. */
  if (terri_cached_move[color-1][ti][tj] != movenum 
      || board_terri_move[color-1][ti][tj] == -1) 
  {
    delta_terri_cache[color-1][ti][tj] = compute_delta_terri(ti, tj, color);
    terri_cached_move[color-1][ti][tj] = movenum;
  }

  return delta_terri_cache[color-1][ti][tj];
}


/*
 * wrapper for delta_terri(..,color) + delta_terri(..,other_color)
 */

int
diff_terri(int ti, int tj, int color)
{
  return delta_terri(ti, tj, color) + delta_terri(ti, tj, OTHER_COLOR(color));
}


static int
compute_delta_terri(int m, int n, int color)
{
  /* This function uses TERRI_DILATIONS dilation and TERRI_EROSIONS erode. */
  int i, b;

  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  terri_test[WHITE] = 0;
  terri_test[BLACK] = 0;

  /* Find a free board in the stack. */
  b = free_board(color);
  board_terri_move[color-1][m][n] = b;
  cache_movei[b] = m;
  cache_movej[b] = n;

  /* OLD CODE */
  for (i = 1; i <= TERRI_DILATIONS; i++)
    clear_moyo(i);

  /* Add the stone to the binmap. */
  mostack[color - 1][0].tot[m + 1] |= (2 << n);

  /* Dilate and erode. */
  memcpy(d_terri_goban, start_goban, sizeof(goban_t));
  d_terri_goban[m][n] = (color == BLACK ? 128 : -128);
  dilate(d_terri_goban, TERRI_DILATIONS);

  for (i = 1; i <= TERRI_EROSIONS; i++)
    erode(d_terri_goban);

  /* Compute delta terri. */
  count_goban(d_terri_goban, terri_test, b);

  /* Remember the added stone is NOT in p[i][j]. */
  if (p[m][n] == EMPTY) {
    if (d_terri_goban[m][n] < 0)
      terri_test[WHITE]--;
    else if (d_terri_goban[m][n] > 0)
      terri_test[BLACK]--;
  }

  /* OLD CODE */
  /* Remove the added stone. */
  mostack[color - 1][0].tot[m + 1] &= (~(2 << n) & mask);

  terri_test[WHITE] += black_captured;	/* no komi added */
  terri_test[BLACK] += white_captured;

  terri_test[0] = terri_test[color] - terri_test[OTHER_COLOR(color)] -
      (terri_eval[color] - terri_eval[OTHER_COLOR(color)]);

  return terri_test[0];
}


/* ---------------------------------------------------------------- */


/*
 * wrapper for delta_moyo(..,color) + delta_moyo(..,other_color) 
 */

int
diff_moyo(int ti, int tj, int color)
{
  return delta_moyo(ti, tj, color) +
      delta_moyo(ti, tj, OTHER_COLOR(color));
}


/*
 * wrapper for average of delta_moyo_simple(..,color) and 
 * delta_moyo_simple(..,other_color) 
 */

int
average_moyo_simple(int ti, int tj, int color)
{
  return (delta_moyo_simple(ti, tj, color) +
	  delta_moyo_simple(ti, tj, OTHER_COLOR(color))) / 2;
}


int
meta_connect(int ti, int tj, int color)
{
/* returns a positive cut/connect bonus or -1 if tested move creates 
 * an isolated group (number of stone <= 2 and  number of space 
 * around <= 10) 
 */
  if (area_cached_move[color - 1][ti][tj] != movenum) {
    delta_area_cache[color - 1][ti][tj] = compute_delta_area(ti, tj, color);
    area_cached_move[color - 1][ti][tj] = movenum;
  }
  return delta_area_cache[color - 1][ti][tj];
}


/*
 * delta_terri computes the board in the stack if not already cached
 */

int
delta_terri_color(int ti, int tj, int color, int m, int n)
{

  delta_terri(ti, tj, color);

  return board_cache[board_terri_move[color-1][ti][tj]][m][n];
}



int
delta_moyo_color(int ti, int tj, int color, int m, int n)
{
  int b = board_moyo_move[color - 1][ti][tj];

  if (delta_moyo_move_cache[color - 1][ti][tj] != movenum || b == -1) {

    /* Find a free board in the stack & compute again delta moyo. */
    b = free_board(color);
    board_moyo_move[color - 1][ti][tj] = b;
    cache_movei[b] = ti;
    cache_movej[b] = tj;

    delta_moyo_value_cache[color - 1][ti][tj] = compute_delta_moyo(ti, tj, color, b);
    delta_moyo_move_cache[color - 1][ti][tj] = movenum;
  }

  return board_cache[b][m][n];
}


int
delta_area_color(int ti, int tj, int color, int m, int n)
{
  int b;
  int dummy3[3];

  b = board_area_move[color - 1][ti][tj];
  if (area_cached_move[color - 1][ti][tj] != movenum || b == -1) {

    /* find a free board in the stack & compute again delta moyo */
    b = free_board(color);
    board_area_move[color - 1][ti][tj] = b;
    cache_movei[b] = ti;
    cache_movej[b] = tj;
    area_cached_move[color - 1][ti][tj] = -1;
    meta_connect(ti, tj, color);
    count_goban(d_moyo_goban, dummy3, b);

  }

  return board_cache[b][m][n];
}


static int
free_board(int color)
{
  int b;

  b = board_cache_free;
  if (board_terri_move[color-1][cache_movei[b]][cache_movej[b]] == b)
    board_terri_move[color-1][cache_movei[b]][cache_movej[b]] = -1;

  if (board_moyo_move[color - 1][cache_movei[b]][cache_movej[b]] == b)
    board_moyo_move[color - 1][cache_movei[b]][cache_movej[b]] = -1;

  if (board_area_move[color - 1][cache_movei[b]][cache_movej[b]] == b)
    board_area_move[color - 1][cache_movei[b]][cache_movej[b]] = -1;

  /* Wrap around to the beginning of the cache. */
  board_cache_free++;
  if (board_cache_free == COLOR_STACK_SIZE)
    board_cache_free = 0;

  return b;
}


static int
compute_delta_moyo(int m, int n, int color, int b)
{
  int i;

  moyo_test[WHITE] = 0;
  moyo_test[BLACK] = 0;

  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  /* OLD CODE */
  for (i = 1; i <= MOYO_DILATIONS; i++)
    clear_moyo(i);

  /* Add the stone to the binmap. */
  /* OLD CODE */
  mostack[color - 1][0].tot[m + 1] |= (2 << n);

  /* Dilate and erode. */
  memcpy(d_moyo_goban, start_goban, sizeof(goban_t));
  d_moyo_goban[m][n] = (color == BLACK ? 128 : -128);
  dilate(d_moyo_goban, MOYO_DILATIONS);

  for (i = 1; i <= MOYO_EROSIONS; i++)
    erode(d_moyo_goban);

  /* Compute delta moyo. */
  count_goban(d_moyo_goban, moyo_test, b);

  /* Remember the added stone is NOT in p[i][j] */
  if (p[m][n] == EMPTY) {
    if (d_moyo_goban[m][n] < 0)
      moyo_test[WHITE]--;
    else if (d_moyo_goban[m][n] > 0)
      moyo_test[BLACK]--;
  }

  /* OLD CODE */
  /* Remove the added stone. */
  mostack[color - 1][0].tot[m + 1] &= (~(2 << n) & mask);

  moyo_test[0] = moyo_test[color] - moyo_test[OTHER_COLOR(color)] -
      (moyo_eval[color] - moyo_eval[OTHER_COLOR(color)]);

  return moyo_test[0];
}


/* 
 * This function provides a "meta" cut bonus for delta_moyo, and tests 
 * if the stone is isolated. If the stone is isolated, there is no bonus,
 * and delta_moyo[_simple]  will be set to zero.   
 */
static int
compute_delta_area(int m, int n, int color)
{
  static area_t   test_area_s[GROUP_STACK_MAX];
  static int      te_areas_level;
  static goban_t  te_area_grid;
  int i;
  int temp_num_groups[2];
  int cut_bonus;
  int other=OTHER_COLOR(color);

  /* OLD CODE */
  for (i = 1; i <= AREA_DILATIONS; i++)
    clear_moyo(i);

  /* OLD CODE */
  /* Add the stone to the binmap. */
  mostack[color - 1][0].tot[m + 1] |= (2 << n);

  memcpy(d_moyo_goban, start_goban, sizeof(goban_t));
  d_moyo_goban[m][n] = (color == BLACK ? 128 : -128);
  dilate(d_moyo_goban, AREA_DILATIONS);

  compute_ownership_test(d_moyo_goban, te_area_grid, 
		    test_area_s, &te_areas_level);
  count_groups(test_area_s, te_areas_level, temp_num_groups);

  /* Remove the added stone. */
  /* OLD CODE */
  mostack[color - 1][0].tot[m + 1] &= (~(2 << n) & mask);

  /* Compute the cut_bonus of the move :
   * If the tested move creates a new isolated group with :
   *   number of stone <= 2 and  number of space around <= 10
   * then 
   *   return -1 to set the delta_moyo value to zero, 
   * else
   *   if we don't create an isolated stone, 
   *   then
   *     return the difference of number of groups * bonus value.     
   */
  if (test_area_s[te_area_grid[m][n]].stone <= 2 
      && test_area_s[te_area_grid[m][n]].space <= 10) 
  {
    cut_bonus = -1;
  } else if (test_area_s[te_area_grid[m][n]].stone > 1) {
    cut_bonus = 0;
#if 0
    if (terri_eval[0] > -15) {
#endif
      cut_bonus += GROUP_CONNECT_BONUS * (num_groups[color - 1] 
					  - temp_num_groups[color - 1]);
      cut_bonus += GROUP_CUT_BONUS * (temp_num_groups[other - 1]
				      - num_groups[other - 1]);
#if 0
    } else if (terri_eval[0] > -30) {
      cut_bonus += GROUP_CONNECT_BONUS * (num_groups[color - 1]
					  - temp_num_groups[color - 1]);
      cut_bonus += 1.5 * GROUP_CUT_BONUS * (temp_num_groups[other - 1] 
					    -num_groups[other-1]);
    } else {
      cut_bonus += 1.5 * GROUP_CONNECT_BONUS * (num_groups[color - 1] 
						- temp_num_groups[color - 1]);
      cut_bonus += 2 * GROUP_CUT_BONUS * (temp_num_groups[other - 1]
					  - num_groups[other - 1]);
    }
#endif
  } else
    cut_bonus = 0;

  return cut_bonus;
}


int
terri_color(int m, int n)
{
  assert(m >= 0 && m < board_size && n >= 0 && n < board_size);

  if (terri_goban[m][n] > 0)
    return BLACK;
  else if (terri_goban[m][n] < 0)
    return WHITE;
  else
    return EMPTY;
}


int
moyo_color(int m, int n)
{
  assert(m >= 0 && m < board_size && n >= 0 && n < board_size);

  if (moyo_goban[m][n] > 0)
    return BLACK;
  else if (moyo_goban[m][n] < 0)
    return WHITE;
  else
    return EMPTY;
}


int
area_stone(int m, int n)
{
  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  return area_array[area_grid[m][n]].stone;
}


int
area_space(int m, int n)
{
  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  return area_array[area_grid[m][n]].space;
}


int
area_color(int m, int n)
{
  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  return area_array[area_grid[m][n]].color;
}


int
area_tag(int m, int n)
{
  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  return area_array[area_grid[m][n]].tag;
}


void
set_area_tag(int m, int n, int tag)
{
  ASSERT((m >= 0 && m < board_size && n >= 0 && n < board_size), m, n);

  area_array[area_grid[m][n]].tag = tag;
}


int
number_weak(int color)
{
  if (color == WHITE || color == BLACK)
    return nb_weak[color - 1];
  else
    return 0;
}


static void
count_groups(area_t areas[], int num_areas, int n_groups[2])
{
  int i;

  n_groups[0] = 0;
  n_groups[1] = 0;

  for (i = 1; i <= num_areas; i++)
    if ((areas[i].color == WHITE) || (areas[i].color == BLACK))
      n_groups[areas[i].color-1]++;
}


/* ---------------------------------------------------------------- */

static void
compute_ownership_old(goban_t gob, goban_t grid, 
		      area_t * area, int *alevel)
{
  int i, j;

#if 0
  int m, n;
  fprintf(stderr, "In compute_ownership()\n");
  for (m = 0; m < board_size; m++) {
    for (n = 0; n < board_size; n++) {
      printf("%6d", gob[m][n]);
    }
    printf("\n");
  }
  printf("\n");
#endif

  /* Some level of the area may be useless (color=-1). */
  *alevel = 0;
  memcpy(grid, empty_goban, sizeof(goban_t));

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++) {
      if (grid[i][j] == 0) {
	(*alevel)++;

	/* The empty space without clear ownership belong to EMPTY. */
	if (gob[i][j] < 0)
	  area[(*alevel)].color = WHITE;
	else if (gob[i][j] > 0)
	  area[(*alevel)].color = BLACK;
	else {
	  /* Look for first stage kosumi connection, not seen without this. */
	  if (j > 0 && gob[i][j - 1] != 0) {
	    if (((mostack[WHITE - 1][0].tot[i] & (2 << j))
		 && ((i + 1) < board_size)
		 && (mostack[WHITE - 1][0].tot[i + 1] & (4 << j))
		 && (gob[i + 1][j - 1] == 0)
		 && grid[i + 1][j] != grid[i][j - 1])
		|| ((mostack[BLACK - 1][0].tot[i] & (2 << j))
		    && ((i + 1) < board_size)
		    && (mostack[BLACK - 1][0].tot[i + 1] & (4 << j))
		    && (gob[i + 1][j - 1] == 0)
		    && grid[i + 1][j] != grid[i][j - 1]))
	    {
	      /* level to free */
	      if (grid[i + 1][j] != 0) {
		combine_area(grid[i + 1][j], grid, area,
			   grid[i][j - 1], i + 1, j);
	      } else {
		spread_area_test(gob, grid, area, grid[i][j - 1], 
				 area[grid[i][j - 1]].color, i + 1, j);
	      }
	    }

	    if (((mostack[WHITE - 1][0].tot[i] & (2 << j))
		 && (i > 0)
		 && (mostack[WHITE - 1][0].tot[i + 1] & (1 << j))
		 && (gob[i - 1][j - 1] == 0)
		 && grid[i - 1][j] != grid[i][j - 1])
		|| ((mostack[BLACK - 1][0].tot[i] & (2 << j))
		    && (i > 0)
		    && (mostack[BLACK - 1][0].tot[i + 1] & (1 << j))
		    && (gob[i - 1][j - 1] == 0)
		    && grid[i - 1][j] != grid[i][j - 1]))
	    {
	      /* level to free */
	      combine_area(grid[i - 1][j], grid, area,
			 grid[i][j - 1], i - 1, j);
	    }
	  }
	  area[(*alevel)].color = EMPTY;
	}

	area[(*alevel)].stone = 0;
	area[(*alevel)].space = 0;
	area[(*alevel)].tag = 0;
	area[(*alevel)].m = i;
	area[(*alevel)].n = j;
	area[(*alevel)].d1m = area[(*alevel)].d1n = -1;
	area[(*alevel)].d2m = area[(*alevel)].d2n = -1;
	area[(*alevel)].d3m = area[(*alevel)].d3n = -1;
	area[(*alevel)].d4m = area[(*alevel)].d4n = -1;
	area[(*alevel)].d5m = area[(*alevel)].d5n = -1;

	spread_area_test(gob, grid, area, *alevel, area[(*alevel)].color, i, j);
      }
    }

  /* compute bonus */
  /*
   *   for (i = 1; i <= (*alevel); i++) {
   *     if (area[i].color == EMPTY) {
   *       if (area[i].space < 20)
   *      area[i].tag = 10;
   *     } else {
   *       if (area[i].color >= 0 && area[i].stone < 10)
   *      area[i].tag += area[i].stone;
   *       if (area[i].color >= 0 && area[i].stone < 15
   *           && (2 * area[i].stone > area[i].space))
   *      area[i].tag += 10;
   *     }
   *   }
   */
}


/*
 * Goban is dilated and, sometimes, eroded goban.
 * Compute the areas owned by the different players and return a map
 * of the areas in area_index, the number of areas in num_areas,
 * and the description of them in areas.
 */

static void
compute_ownership(goban_t goban,
		  goban_t area_index, area_t * areas, int *num_areas)
{
  int i, j;

  /* Some level of the area may be useless (color=-1). */
  *num_areas = 0;
  memcpy(area_index, empty_goban, sizeof(goban_t));

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++) {
      if (area_index[i][j] == 0) {
	(*num_areas)++;

	/* The empty space without clear ownership belong to EMPTY. */
	if (goban[i][j] < 0)
	  areas[*num_areas].color = WHITE;
	else if (goban[i][j] > 0)
	  areas[*num_areas].color = BLACK;
	else {
	  /* Look for first stage kosumi connection, not seen without this. */
	  /*
	  
	      we are looking for situations like :
	      
	      ? ? ?     i,j in the center, owned by nobody, if the  
	      ? . O     lower right corner is empty, then the two	      
	      ? O .     stones belong to same area.
	    
	    (and this is not i/j symetric, this induced a bug)
	    
	    (nota: we at this one later    O .
	                                   . O  )
	  	  	  
	       f g h      . is at i,j position (we know it's empty)	                	       
	   j   d . e      points a,d,f and b already seen by the "for" loops
	 j-1   a b c
	  
	   0     i i+1       
	  */
	  
	  if (j > 0 && goban[i][j - 1] != 0) {
	  /*    line b exists and b ~ stone	  */
	  
	    /* if ((((i + 1) < board_size)
	     *	    && (goban[i + 1][j - 1] == 0)
	     *      && (mostack[WHITE - 1][0].tot[i] & (2 << j))       aka i-1,j
	     *	    && (mostack[WHITE - 1][0].tot[i + 1] & (4 << j))   aka i,j+1
	     *	    && area_index[i + 1][j] != area_index[i][j - 1]) 
	                                                             should be 
	                                                             i,j+1 != i-1,j but...
	     *	   || (((i + 1) < board_size)
	     *	       && (goban[i + 1][j - 1] == 0)
	     *	       && (mostack[BLACK - 1][0].tot[i] & (2 << j))
	     *	       && (mostack[BLACK - 1][0].tot[i + 1] & (4 << j))
	     *	       && area_index[i + 1][j] != area_index[i][j - 1]))
	     */
/* gnugo 2.4 code
 * 	  if (j > 0 && (*gob)[i][j - 1] != 0) {
 * 	    if (((mostack[WHITE - 1][0].tot[j] & (2 << i))         aka i,j-1
 * 		 && ((i - 1) < board_size)
 * 		 && (mostack[WHITE - 1][0].tot[j + 1] & (4 << i))   aka i+1,j
 * 		 && ((*gob)[i + 1][j - 1] == 0)
 * 		 && (*grid)[i + 1][j] != (*grid)[i][j - 1])
 * 		|| ((mostack[BLACK - 1][0].tot[j] & (2 << i))
 * 		    && ((i - 1) < board_size)
 * 		    && (mostack[BLACK - 1][0].tot[j + 1] & (4 << i))
 * 		    && ((*gob)[i + 1][j - 1] == 0)
 * 		    && (*grid)[i + 1][j] != (*grid)[i][j - 1]))
 * 	    {
 * 	       level to free 
 * 	      if ((*grid)[i + 1][j] != 0) {
 * 		clean_grid((*grid)[i + 1][j], grid, area,
 * 			   (*grid)[i][j - 1], i + 1, j);
 * 	      } else {
 * 		diffuse_area(gob, grid, area, 
 * 			     area[(*grid)[i][j - 1]].color,
 * 			     (*grid)[i][j - 1], i + 1, j);
 * 	      }
 * 	    }
 */
	    if ((((i + 1) < board_size)            /* line e,c exist  */
		 && (goban[i + 1][j - 1] == 0)  /* c ~ empty      */
/*		 && (goban[i-1][j] < -64)  */ 
		 && (goban[i][j-1] < -64)       /* b ~ white stone      */ 
/*		 && (goban[i][j+1] < -64)  */
		 && (goban[i+1][j] < -64)       /* e ~ white stone      */
		 /* 
		    now we got   . O
		                 O .   
		*/
		 && area_index[i + 1][j] != area_index[i][j - 1])
                         /*        e area != b area (remember, b was already explored in loop) */

		|| (((i + 1) < board_size)     /* the same for black (some optimization here ?) */
		    && (goban[i + 1][j - 1] == 0)
/*		    && (goban[i-1][j] > 64)   */
		    && (goban[i][j-1] > 64)
/*		    && (goban[i][j+1] > 64)   */
		    && (goban[i+1][j] > 64)
		    && area_index[i + 1][j] != area_index[i][j - 1]))

	    {
	      /* level to free */
	      if (area_index[i + 1][j] != 0) {/* e has already owned area ? if yes, than
	                                            we need to mix b and e areas */
		combine_area(area_index[i + 1][j], area_index, areas,
			   area_index[i][j - 1], i + 1, j);
			   
	      } else { /* apply already computed b group area to e group */
		spread_area_test(goban, area_index, areas, area_index[i][j - 1], 
				 areas[area_index[i][j - 1]].color, i + 1, j);
	      }
	    }

	    /* if (((i > 0)
	     *	    && (mostack[WHITE - 1][0].tot[i] & (2 << j))
	     *	    && (mostack[WHITE - 1][0].tot[i + 1] & (1 << j))
	     *	    && (goban[i - 1][j - 1] == 0)
	     *	    && area_index[i - 1][j] != area_index[i][j - 1])
	     *	   || ((i > 0)
	     *	       && (mostack[BLACK - 1][0].tot[i] & (2 << j))
	     *	       && (mostack[BLACK - 1][0].tot[i + 1] & (1 << j))
	     *	       && (goban[i - 1][j - 1] == 0)
	     *	       && area_index[i - 1][j] != area_index[i][j - 1]))
	     */
	     
	     
	     
	     /*
	     now we look for  O *  shape, where * is at i,j
	                      . O  
	     (nota: this one is i/j symetric  :-)		      
	     
	     */
	    if (((i > 0)
		 && (goban[i-1][j] < -64)   /* d */		 
		 && (goban[i][j-1] < -64)   /* b   lucky, j>0 */
		 && (goban[i - 1][j - 1] == 0) /* a empty */
		 && area_index[i - 1][j] != area_index[i][j - 1])
		|| ((i > 0)
		    && (goban[i-1][j] > 64)  /* the same for other color */
		    && (goban[i][j-1] > 64)
		    && (goban[i - 1][j - 1] == 0)
		    && area_index[i - 1][j] != area_index[i][j - 1]))
	    {
	      /* level to free */
	      /* no need test here, since we know that both b and d area are 
	         already computed, so, just mix them  */
	      combine_area(area_index[i - 1][j], area_index, areas,
			 area_index[i][j - 1], i - 1, j);
	    }
	  }
	  areas[*num_areas].color = EMPTY;
	}

	areas[*num_areas].stone = 0;
	areas[*num_areas].space = 0;
	areas[*num_areas].tag = 0;
	areas[*num_areas].m = i;
	areas[*num_areas].n = j;
	areas[*num_areas].d1m = areas[*num_areas].d1n = -1;
	areas[*num_areas].d2m = areas[*num_areas].d2n = -1;
	areas[*num_areas].d3m = areas[*num_areas].d3n = -1;
	areas[*num_areas].d4m = areas[*num_areas].d4n = -1;
	areas[*num_areas].d5m = areas[*num_areas].d5n = -1;

	spread_area_test(goban, area_index, areas, *num_areas, 
			 areas[*num_areas].color, i, j);
      }
    }

  /* compute bonus */
  /*
   *   for (i = 1; i <= (*num_areas); i++) {
   *     if (area[i].color == EMPTY) {
   *       if (area[i].space < 20)
   *      area[i].tag = 10;
   *     } else {
   *       if (area[i].color >= 0 && area[i].stone < 10)
   *      area[i].tag += area[i].stone;
   *       if (area[i].color >= 0 && area[i].stone < 15
   *           && (2 * area[i].stone > area[i].space))
   *      area[i].tag += 10;
   *     }
   *   }
   */
}

static void
compute_ownership_test(goban_t goban,
		       goban_t area_index, area_t * areas, int *num_areas)
{
  goban_t  goban2;
  area_t   areas2[GROUP_STACK_MAX];
  int      num_areas2;
  goban_t  area_index2;
  int      i, j;

  for (i = 0; i < board_size; ++i)
    for (j = 0; j < board_size; ++j) {
      goban2[i][j] = goban[i][j];
    }
  for (i = 0; i < GROUP_STACK_MAX; ++i)
    areas2[i] = areas[i];
  num_areas2 = *num_areas;
  for (i = 0; i < board_size; ++i)
    for (j = 0; j < board_size; ++j) {
      area_index2[i][j] = area_index[i][j];
    }

  compute_ownership_old(goban, area_index, areas, num_areas);
  compute_ownership(goban2, area_index2, &areas2[0], &num_areas2);

#if 0
  for (i = 0; i < board_size; ++i)
    for (j = 0; j < board_size; ++j) {
      assert(goban2[i][j] == goban[i][j]);
    }

  assert(num_areas2 = *num_areas);
  for (i = 0; i < *num_areas; ++i) {
    assert(areas2[i].m == areas[i].m);
    assert(areas2[i].n == areas[i].n);
    assert(areas2[i].color == areas[i].color);
    assert(areas2[i].stone == areas[i].stone);
    assert(areas2[i].space == areas[i].space);
    assert(areas2[i].tag == areas[i].tag);
    assert(areas2[i].d1m == areas[i].d1m);
    assert(areas2[i].d1n == areas[i].d1n);
    assert(areas2[i].d2m == areas[i].d2m);
    assert(areas2[i].d2n == areas[i].d2n);
    assert(areas2[i].d3m == areas[i].d3m);
    assert(areas2[i].d3n == areas[i].d3n);
    assert(areas2[i].d4m == areas[i].d4m);
    assert(areas2[i].d5n == areas[i].d5n);
    assert(areas2[i].d5m == areas[i].d5m);
    assert(areas2[i].d5n == areas[i].d5n);
  }
  for (i = 0; i < board_size; ++i)
    for (j = 0; j < board_size; ++j) {
      assert(area_index2[i][j] == area_index[i][j]);
    }
#endif
}
  

/* ---------------------------------------------------------------- */

/*
 * A string is defined to be WEAK if it has size 2 or more and its
 * space is between 0 and 20 points.
 */

static void
test_weak(goban_t grid, area_t * area, int tw)
{
  int i, j, m, n;
  int gen = 0;
  int nbdrag = 0;
  char cs;

  if (area[tw].color >= 1 && area[tw].space < 21) {
    area[tw].tag = 2;
    if (printmoyo & 16)
      gprintf("weak area %d  %m: color %c,  %d stone  %d spaces\n",
	      tw, area[tw].m, area_array[tw].n,
	      area[tw].color == WHITE ? 'W' : 'B',
	      area[tw].stone, area[tw].space);

    /* Find area. */
    for (i = 0; i < board_size; i++)
      for (j = 0; j < board_size; j++) {
	if (grid[i][j] == tw && p[i][j] && dragon[i][j].safety != DEAD) {
	  if (nbdrag < 6
	      && area[tw].d1m != dragon[i][j].origini
	      && area[tw].d1n != dragon[i][j].originj
	      && area[tw].d2m != dragon[i][j].origini
	      && area[tw].d2n != dragon[i][j].originj
	      && area[tw].d3m != dragon[i][j].origini
	      && area[tw].d3n != dragon[i][j].originj
	      && area[tw].d4m != dragon[i][j].origini
	      && area[tw].d4n != dragon[i][j].originj
	      && area[tw].d5m != dragon[i][j].origini
	      && area[tw].d5n != dragon[i][j].originj) 
	  {
	    nbdrag++;
	    gen += dragon[i][j].genus;

	    if (printmoyo & 16) {
	      switch (dragon[i][j].safety) {
	      case ALIVE:
		cs = 'A';
		break;
	      case DEAD:
		cs = 'D';
		break;
	      case UNKNOWN:
		cs = 'U';
		break;
	      case CRITICAL:
		cs = 'C';
		break;
	      default:
		cs = '?';
	      }
	      gprintf("drag %m  gen %d  weakness: %c ",
		      i, j, dragon[i][j].genus, cs);
	    }

	    if (dragon[i][j].safety == UNKNOWN) {
	      dragon[dragon[i][j].origini]
		    [dragon[i][j].originj].safety = CRITICAL;
	      for (m = 0; m < board_size; m++)
		for (n = 0; n < board_size; n++) {
		  struct dragon_data *d = &(dragon[m][n]);

		  dragon[m][n] = dragon[d->origini][d->originj];
		}

              area[tw].tag ++;
	      if (printmoyo & 16)
		gprintf("=> critical\n");
	    } else {
	      if (printmoyo & 16)
		gprintf("\n");
	    }

	    if (area[tw].d1m == -1) {
	      area[tw].d1m = dragon[i][j].origini;
	      area[tw].d1n = dragon[i][j].originj;
	    } else if (area[tw].d2m == -1) {
	      area[tw].d2m = dragon[i][j].origini;
	      area[tw].d2n = dragon[i][j].originj;
	    } else if (area[tw].d3m == -1) {
	      area[tw].d3m = dragon[i][j].origini;
	      area[tw].d3n = dragon[i][j].originj;
	    } else if (area[tw].d4m == -1) {
	      area[tw].d4m = dragon[i][j].origini;
	      area[tw].d4n = dragon[i][j].originj;
	    } else if (area[tw].d5m == -1) {
	      area[tw].d5m = dragon[i][j].origini;
	      area[tw].d5n = dragon[i][j].originj;
	    }
	  }
	}
      }
  }
}

/*
 * The old version.
 */
static void
diffuse_area(goban_t gob, goban_t grid, area_t * area, 
	     int colval, int gridval, int i, int j)
{

  grid[i][j] = gridval;

  /* If a neighbour is of the same color, apply recursively. */
  if (colval == WHITE) {
    if (mostack[WHITE - 1][0].tot[i + 1] & (2 << j))
      area[gridval].stone++;
    else
      area[gridval].space++;

    if (i > 0 && grid[i - 1][j] == 0 && gob[i - 1][j] < 0)
      diffuse_area(gob, grid, area, colval, gridval, i - 1, j);

    if (j > 0 && grid[i][j - 1] == 0 && gob[i][j - 1] < 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j - 1);

    if ((i + 1) < board_size && grid[i + 1][j] == 0 && gob[i + 1][j] < 0)
      diffuse_area(gob, grid, area, colval, gridval, i + 1, j);

    if ((j + 1) < board_size && grid[i][j + 1] == 0 && gob[i][j + 1] < 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j + 1);
  } else if (colval == BLACK) {
    if (mostack[BLACK - 1][0].tot[i + 1] & (2 << j))
      area[gridval].stone++;
    else
      area[gridval].space++;

    if (i > 0 && grid[i - 1][j] == 0 && gob[i - 1][j] > 0)
      diffuse_area(gob, grid, area, colval, gridval, i - 1, j);

    if (j > 0 && grid[i][j - 1] == 0 && gob[i][j - 1] > 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j - 1);

    if ((i + 1) < board_size && grid[i + 1][j] == 0 && gob[i + 1][j] > 0)
      diffuse_area(gob, grid, area, colval, gridval, i + 1, j);

    if ((j + 1) < board_size && grid[i][j + 1] == 0 && gob[i][j + 1] > 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j + 1);

  } else if (colval == EMPTY) {
    area[gridval].space++;

    if (i > 0
	&& grid[i - 1][j] == 0
	&& gob[i - 1][j] == 0)
      diffuse_area(gob, grid, area, colval, gridval, i - 1, j);

    if (j > 0
	&& grid[i][j - 1] == 0
	&& gob[i][j - 1] == 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j - 1);

    if ((i + 1) < board_size
	&& grid[i + 1][j] == 0
	&& gob[i + 1][j] == 0)
      diffuse_area(gob, grid, area, colval, gridval, i + 1, j);

    if ((j + 1) < board_size
	&& grid[i][j + 1] == 0
	&& gob[i][j + 1] == 0)
      diffuse_area(gob, grid, area, colval, gridval, i, j + 1);
  }
}


/*
 * The parameter goban contains a dilated goban. 
 * The parameter area_index contains for each location the id of the
 *    area covering that location. 
 *
 * Spread_area spreads the area with origin at (i, j) to all locations in
 *   goban which are neighbours to (i, j) and contains the same color
 *   recursively.
 */

static void
spread_area(goban_t goban, goban_t area_index, 
	    area_t * area, int area_id,
	    int color, int i, int j)
{
  area_index[i][j] = area_id;

  /* If a neighbour is of the same color, apply recursively. */
  if (color == WHITE) {

    /* See if the original goban contained a white stone at this position. 
       -64 should be a sufficient limit even if we have 20 erosions or more. */
    if (goban[i][j] < -64)
      area->stone++;
    else
      area->space++;

    if (i > 0
	&& area_index[i - 1][j] == 0
	&& goban[i - 1][j] < 0)
      spread_area(goban, area_index, area, area_id, color, i - 1, j);

    if (j > 0
	&& area_index[i][j - 1] == 0
	&& goban[i][j - 1] < 0)
      spread_area(goban, area_index, area, area_id, color, i, j - 1);

    if (i < board_size - 1
	&& area_index[i + 1][j] == 0
	&& goban[i + 1][j] < 0)
      spread_area(goban, area_index, area, area_id, color, i + 1, j);

    if (j < board_size - 1
	&& area_index[i][j + 1] == 0
	&& goban[i][j + 1] < 0)
      spread_area(goban, area_index, area, area_id, color, i, j + 1);

  } else if (color == BLACK) {
    /* See if the original goban contained a white stone at this position. 
       64 should be a sufficient limit even if we have 20 erosions or more. */
    if (goban[i][j] > 64)
      area->stone++;
    else
      area->space++;

    if (i > 0
	&& area_index[i - 1][j] == 0
	&& goban[i - 1][j] > 0)
      spread_area(goban, area_index, area, area_id, color, i - 1, j);

    if (j > 0
	&& area_index[i][j - 1] == 0
	&& goban[i][j - 1] > 0)
      spread_area(goban, area_index, area, area_id, color, i, j - 1);

    if (i < board_size - 1
	&& area_index[i + 1][j] == 0
	&& goban[i + 1][j] > 0)
      spread_area(goban, area_index, area, area_id, color, i + 1, j);

    if (j < board_size - 1
	&& area_index[i][j + 1] == 0
	&& goban[i][j + 1] > 0)
      spread_area(goban, area_index, area, area_id, color, i, j + 1);

  } else if (color == EMPTY) {
    area->space++;

    if (i > 0
	&& area_index[i - 1][j] == 0
	&& goban[i - 1][j] == 0)
      spread_area(goban, area_index, area, area_id, color, i - 1, j);

    if (j > 0
	&& area_index[i][j - 1] == 0
	&& goban[i][j - 1] == 0)
      spread_area(goban, area_index, area, area_id, color, i, j - 1);

    if (i < board_size - 1
	&& area_index[i + 1][j] == 0
	&& goban[i + 1][j] == 0)
      spread_area(goban, area_index, area, area_id, color, i + 1, j);

    if (j < board_size - 1
	&& area_index[i][j + 1] == 0
	&& goban[i][j + 1] == 0)
      spread_area(goban, area_index, area, area_id, color, i, j + 1);
  }
}


/*
 * Wrapper around spread_area.  This version calls both the new and the
 * old version and checks that the result is the same from both.
 *
 * FIXME: Remove when we are satisfied that it works.
 *        Remember to change all calls to spread_area to reflect the
 *        new parameters (pointer to area instead of an area array).
 */

static void
spread_area_test(goban_t goban, goban_t area_index, 
		 area_t * areas, int area_id,
		 int color, int i, int j)
{
  area_t test_area;
  goban_t test_index;
  int  m, n;
  int  error=0;

  assert(i >= 0 && i < board_size && j >= 0 && j < board_size);

  /* Test spread_area using copies of the area and the index. */
  memcpy(&test_area, &(areas[area_id]), sizeof(area_t));
  memcpy(test_index, area_index, sizeof(goban_t));
  spread_area(goban, test_index, &test_area, area_id, color, i, j);

  diffuse_area(goban, area_index, areas, color, area_id, i, j);

  /* Check for differences. */
  for (m = 0; m < board_size; m++) {
    for (n = 0; n < board_size; n++) {
      if (test_index[m][n] != area_index[m][n]) 
	error |= 1;
    }
  }

  if (!(test_area.m == areas[area_id].m)) error |= 2;
  if (!(test_area.n == areas[area_id].n)) error |= 2;
  if (!(test_area.color == areas[area_id].color)) error |= 2;
  if (!(test_area.stone == areas[area_id].stone)) error |= 2;
  if (!(test_area.space == areas[area_id].space)) error |= 2;
  if (!(test_area.tag == areas[area_id].tag)) error |= 2;

  if (error) {
    fprintf(stderr, "In spread_area(%d,%d)\n", i, j);
    for (m = 0; m < board_size; m++) {
      for (n = 0; n < board_size; n++) {
	printf("%5d", goban[m][n]);
      }
      printf("\n");
    }
    printf("\n");

    for (m = 0; m < board_size; m++) {
      for (n = 0; n < board_size; n++) {
	printf("%4d",test_index[m][n]);
      }
      if (error & 1) {
	printf("      ");
	for (n = 0; n < board_size; n++) {
	  printf("%4d", area_index[m][n]);
	}
      }
      printf("\n");
    }
  }
   
  if (error & 2) {
    printf("       test_area (ny)  areas[area_id]\n");
    printf("m:     %-16d%d\n", test_area.m,     areas[area_id].m);
    printf("n:     %-16d%d\n", test_area.n,     areas[area_id].n);
    printf("color: %-16d%d\n", test_area.color, areas[area_id].color);
    printf("stone: %-16d%d\n", test_area.stone, areas[area_id].stone);
    printf("space: %-16d%d\n", test_area.space, areas[area_id].space);
    printf("tag:   %-16d%d\n", test_area.tag,   areas[area_id].tag);
  }

  if (error)
    exit(1);
}


/* ---------------------------------------------------------------- */


/* 
 * combine_area adds the area "area2" at (i,j) to the area "area1"
 * using the recursive function combine_area2.
 */

static void
combine_area(int area2, goban_t area_index, area_t * areas, 
	     int area1, int i, int j)
{
  int cpt, ii, jj;

  areas[area1].stone += areas[area2].stone;
  areas[area1].space += areas[area2].space;
  areas[area2].color = -1;

  /* number of vertices to find. */
  cpt = areas[area2].stone + areas[area2].space;

  combine_area2(&cpt, area2, area_index, area1, i, j);

  if (cpt > 0) {
    /* Bad luck. There are second stage kosumi connection, 
     * and we need to apply a heavy method.
     */
    for (ii = 0; ii < board_size; ii++)
      for (jj = 0; jj < board_size; jj++) {
	if (area_index[ii][jj] == area2) {
	  area_index[ii][jj] = area1;
	  cpt--;
	  if (cpt == 0)
	    return;
	}
      }

    ASSERT(cpt == 0, i, j);
  }

  return;
}


/*
 * Worker function for combine_area().
 */

static void
combine_area2(int *cpt, int area2, goban_t area_index, 
	      int area1, int i, int j)
{
  area_index[i][j] = area1;
  (*cpt)--;

  /* If a neighbour is also of area2, apply recursively. */
  if (i > 0 && area_index[i - 1][j] == area2)
    combine_area2(cpt, area2, area_index, area1, i - 1, j);

  if (j > 0 && area_index[i][j - 1] == area2)
    combine_area2(cpt, area2, area_index, area1, i, j - 1);

  if ((i + 1) < board_size && area_index[i + 1][j] == area2)
    combine_area2(cpt, area2, area_index, area1, i + 1, j);

  if ((j + 1) < board_size && area_index[i][j + 1] == area2)
    combine_area2(cpt, area2, area_index, area1, i, j + 1);
}


/* ================================================================ */
/*                    low level functions                           */
/* ================================================================ */


/*
 * Count the score on the goban gob.  Return the value in score[3].
 * This function is used after a number of dilations/erosions.
 *
 * If b != -1, then enter the score points into board_cache[b].
 */

static void
count_goban(goban_t gob, int *score, int b)
{
  int i, j;

  if (b == -1) {

    for (i = 0; i < board_size; i++)
      for (j = 0; j < board_size; j++) {
	if (p[i][j] == EMPTY) {
	  if (gob[i][j] < 0)
	    score[WHITE]++;
	  else if (gob[i][j] > 0)
	    score[BLACK]++;
	} else {
	  if (dragon[i][j].safety == DEAD) {
	    if (gob[i][j] < 0)
	      score[WHITE] += 2;
	    else if (gob[i][j] > 0)
	      score[BLACK] += 2;
	  }
	}
      }
  } else {
    for (i = 0; i < board_size; i++)
      for (j = 0; j < board_size; j++) {
	if (p[i][j] == EMPTY) {
	  if (gob[i][j] < 0) {
	    score[WHITE]++;
	    board_cache[b][i][j] = WHITE;
	  } else if (gob[i][j] > 0) {
	    score[BLACK]++;
	    board_cache[b][i][j] = BLACK;
	  } else {
	    board_cache[b][i][j] = EMPTY;
	  }
	} else {
	  if (dragon[i][j].safety == DEAD) {
	    if (gob[i][j] < 0) {
	      score[WHITE] += 2;
	      board_cache[b][i][j] = WHITE;
	    } else if (gob[i][j] > 0) {
	      score[BLACK] += 2;
	      board_cache[b][i][j] = BLACK;
	    } else {
	      board_cache[b][i][j] = EMPTY;
	    }
	  } else {
	    board_cache[b][i][j] = p[i][j];
	  }
	}
      }
  }
}


/*
 * sum of the bits of a binmap, aka influence
 */

#if 0
static int
influence(binmap_t * s)
{
  int i, j, k;
 
  k = 0;
  for (i = 1; i <= board_size; i++)
    for (j = 0; j < board_size; j++)
      if ((*s)[i] & (1 << j))
	k++;
  return k;
}
#endif


/*
 * The stack need to be cleared at every cycle.
 */

static void
clear_moyo(int i)
{
  mostack[WHITE - 1][i] = empty_shadow;
  mostack[BLACK - 1][i] = empty_shadow;
}


/* 
 * Compute the "tot" of a shadow by adding the 4 directions
 * influences.
 */

static void
sum_shadow(Shadow * s)
{
  memcpy(s->tot, s->left, sizeof(binmap_t));
  or_binmap(s->tot, s->right);
  or_binmap(s->tot, s->up);
  or_binmap(s->tot, s->down);
}

/*
 * Sum of the levels of dilations from the moyo stack in a numeric
 * goban for later use of this goban in erode phase.
 * add from 1 to "int num" level of the stack.
 */

static void
stack2goban(int num, goban_t gob)
{
  int i, j, k;

  for (i = 1; i <= board_size; i++)
    for (j = 1; j <= board_size; j++) {
      if ((mostack[WHITE - 1][0].tot[i]) & (1 << j))
	gob[i - 1][j - 1] -= 128;
      if ((mostack[BLACK - 1][0].tot[i]) & (1 << j))
	gob[i - 1][j - 1] += 128;
      for (k = 1; k <= num; k++) {
	gob[i - 1][j - 1] -= (mostack[WHITE - 1][k].left[i] >> j) & 1;
	gob[i - 1][j - 1] -= (mostack[WHITE - 1][k].right[i] >> j) & 1;
	gob[i - 1][j - 1] -= (mostack[WHITE - 1][k].up[i] >> j) & 1;
	gob[i - 1][j - 1] -= (mostack[WHITE - 1][k].down[i] >> j) & 1;
	gob[i - 1][j - 1] += (mostack[BLACK - 1][k].left[i] >> j) & 1;
	gob[i - 1][j - 1] += (mostack[BLACK - 1][k].right[i] >> j) & 1;
	gob[i - 1][j - 1] += (mostack[BLACK - 1][k].up[i] >> j) & 1;
	gob[i - 1][j - 1] += (mostack[BLACK - 1][k].down[i] >> j) & 1;
      }
    }
}


/* 
 * Load the current live stones of one color from the global dragon.
 */

static void
load_dragon_color_tot(Shadow * s, board_t what)
{
  int i, j;

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++) {
      if (p[i][j] == what && dragon[i][j].safety != DEAD)
	s->tot[i + 1] |= (2 << j);
    }
}


/*
 * Make a bit to bit OR between 2 binmap_t
 */

static void
or_binmap(binmap_t v1, binmap_t v2)
{
  int i;

  /* No OR on 0 and 21 line */
  for (i = 1; i <= board_size; i++)
    v1[i] |= v2[i];
}

/* 
 * Invert the binary binmap. 
 * 0=>1 and 1=>0, except the border.
 */

static void
inv_binmap(binmap_t v1)
{
  int i;

  for (i = 1; i <= board_size; i++)
    v1[i] = ~v1[i] & mask;
}


/*
 * Make the crude dilatation in the shadow.zone binmap for one
 * moyo of the stack.
 */

static void
zone_dilat(int ns)
{
  int i;

  memcpy(mostack[WHITE - 1][ns].zone, 
	 mostack[WHITE - 1][ns].tot, sizeof(binmap_t));
  for (i = 1; i <= board_size; i++) {
    mostack[WHITE - 1][ns].zone[i] 
      |= ((mostack[WHITE - 1][ns].tot[i] << 1) 
	  | (mostack[WHITE - 1][ns].tot[i] >> 1)) & mask;
    mostack[WHITE - 1][ns].zone[i] |= mostack[WHITE - 1][ns].tot[i - 1];
    mostack[WHITE - 1][ns].zone[i] |= mostack[WHITE - 1][ns].tot[i + 1];
  }

  memcpy(mostack[BLACK - 1][ns].zone,
	 mostack[BLACK - 1][ns].tot, sizeof(binmap_t));
  for (i = 1; i <= board_size; i++) {
    mostack[BLACK - 1][ns].zone[i] 
      |= ((mostack[BLACK - 1][ns].tot[i] << 1) 
	  | (mostack[BLACK - 1][ns].tot[i] >> 1)) & mask;
    mostack[BLACK - 1][ns].zone[i] |= mostack[BLACK - 1][ns].tot[i - 1];
    mostack[BLACK - 1][ns].zone[i] |= mostack[BLACK - 1][ns].tot[i + 1];
  }
}


static void
dilate(goban_t goban, int dilations)
{
  int val, inc;
  int dil;
  int i, j;
  goban_t temp_goban;
  goban_t temp_goban2;

#if 0
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      printf("%4d ", goban[i][j]);
    }
    printf("\n");
  }
  printf("===================================================\n");
#endif

  for (dil = 0; dil < dilations; dil++) {
    for (i = 0; i < board_size; i++) {
      for (j = 0; j < board_size; j++) {
	temp_goban[i][j] = goban[i][j];

	val = goban[i][j];
	inc = 0;

	if (i > 0) {
	  if (goban[i-1][j] * inc < 0 || goban[i-1][j] * val < 0)
	    continue;
	  else if (goban[i-1][j] > 0)
	    inc++;
	  else if (goban[i-1][j] < 0)
	    inc--;
	}

	if (i < board_size - 1) {
	  if (goban[i+1][j] * inc < 0 || goban[i+1][j] * val < 0)
	    continue;
	  else if (goban[i+1][j] > 0)
	    inc++;
	  else if (goban[i+1][j] < 0)
	    inc--;
	}

	if (j > 0) {
	  if (goban[i][j-1] * inc < 0 || goban[i][j-1] * val < 0)
	    continue;
	  else if (goban[i][j-1] > 0)
	    inc++;
	  else if (goban[i][j-1] < 0)
	    inc--;
	}

	if (j < board_size - 1) {
	  if (goban[i][j+1] * inc < 0 || goban[i][j+1] * val < 0)
	    continue;
	  else if (goban[i][j+1] > 0)
	    inc++;
	  else if (goban[i][j+1] < 0)
	    inc--;
	}

	temp_goban[i][j] += inc;
      }
    }

    memcpy(goban, temp_goban, sizeof(goban_t));
#if 0
    for (i = 0; i < board_size; i++) {
      for (j = 0; j < board_size; j++) {
	printf("%4d ", goban[i][j]);
      }
      printf("\n");
    }
    printf("===================================================\n");
#endif
  }

  /* Check the result with the old implementation. */
  /* FIXME: Remove this when we are satisfied that it works. */
  {
    int  error = 0;

    dilation(dilations);
    memcpy(temp_goban2, empty_goban, sizeof(goban_t));
    stack2goban(dilations, temp_goban2);
    for (i = 0; i < board_size; i++) {
      for (j = 0; j < board_size; j++) {
	/*      assert(temp_goban[i][j] == temp_goban2[i][j]);*/
	if (temp_goban[i][j] != temp_goban2[i][j])
	  error = 1;
      }
    }

    if (error) {
      for (i = 0; i < board_size; i++) {
	for (j = 0; j < board_size; j++) {
	  fprintf(stderr,"%4d ", temp_goban2[i][j]);
	}
	fprintf(stderr,"\n");
      }
      fprintf(stderr,"\n");
      for (i = 0; i < board_size; i++) {
	for (j = 0; j < board_size; j++) {
	  fprintf(stderr,"%4d ", temp_goban[i][j]);
	}
	fprintf(stderr,"\n");
      }
      abort();
    }
  }
}


/*
 * Compute one dilation. Make the four unitary dilations and the sum in
 * shadow.tot, for both colors of a moyo of the stack.
 */

static void
dilation(int dilations)
{
  int i, j;
  binmap_t zone;

  for (i = 1; i <= dilations; i++) {
    zone_dilat(i - 1);

    memcpy(zone, mostack[BLACK - 1][i - 1].zone, sizeof(binmap_t));
    inv_binmap(zone);
    for (j = 1; j <= board_size; j++) {
      mostack[WHITE - 1][i].left[j] 
	= (mostack[WHITE - 1][i - 1].tot[j] << 1) & mask & zone[j];
      mostack[WHITE - 1][i].right[j] 
	= (mostack[WHITE - 1][i - 1].tot[j] >> 1) & mask & zone[j];
      mostack[WHITE - 1][i].up[j]
	= mostack[WHITE - 1][i - 1].tot[j + 1] & zone[j];
      mostack[WHITE - 1][i].down[j]
	= mostack[WHITE - 1][i - 1].tot[j - 1] & zone[j];
    }

    sum_shadow(&(mostack[WHITE - 1][i]));
    or_binmap(mostack[WHITE - 1][i].tot, mostack[WHITE - 1][i - 1].tot);

    memcpy(zone, mostack[WHITE - 1][i - 1].zone, sizeof(binmap_t));
    inv_binmap(zone);
    for (j = 1; j <= board_size; j++) {
      mostack[BLACK - 1][i].left[j] 
	= (mostack[BLACK - 1][i - 1].tot[j] << 1) & mask & zone[j];
      mostack[BLACK - 1][i].right[j] 
	= (mostack[BLACK - 1][i - 1].tot[j] >> 1) & mask & zone[j];
      mostack[BLACK - 1][i].up[j] 
	= mostack[BLACK - 1][i - 1].tot[j + 1] & zone[j];
      mostack[BLACK - 1][i].down[j] 
	= mostack[BLACK - 1][i - 1].tot[j - 1] & zone[j];
    }

    sum_shadow(&(mostack[BLACK - 1][i]));
    or_binmap(mostack[BLACK - 1][i].tot, mostack[BLACK - 1][i - 1].tot);
  }
}



/*
 * The erode function : don't use binary operators.
 */

static void
erosion(goban_t gob)
{
  int i, j, m, mb;
  goban_t outgob;

  /*** different color or null => erode 1 point ***/
  for (j = 0; j < board_size; j++) {
    for (i = 0; i < board_size; i++) {
      if ((mb = gob[i][j]) != 0) {
	m = 0;

	if (mb > 0) {
	  i++;
	  if (i < board_size)
	    if (gob[i][j] * mb <= 0)
	      m--;
	  i--;
	  j++;

	  if (j < board_size)
	    if (gob[i][j] * mb <= 0)
	      m--;
	  i--;
	  j--;

	  if (i >= 0)
	    if (gob[i][j] * mb <= 0)
	      m--;
	  i++;
	  j--;

	  if (j >= 0)
	    if (gob[i][j] * mb <= 0)
	      m--;
	  j++;

	  if (-m >= mb)
	    mb = 0;
	  else
	    mb += m;
	} else {
	  i++;
	  if (i < board_size)
	    if (gob[i][j] * mb <= 0)
	      m++;
	  i--;
	  j++;

	  if (j < board_size)
	    if (gob[i][j] * mb <= 0)
	      m++;
	  i--;
	  j--;

	  if (i >= 0)
	    if (gob[i][j] * mb <= 0)
	      m++;
	  i++;
	  j--;

	  if (j >= 0)
	    if (gob[i][j] * mb <= 0)
	      m++;
	  j++;

	  if (m >= -mb)
	    mb = 0;
	  else
	    mb += m;
	}
      }
      outgob[i][j] = mb;
    }
  }

  memcpy(gob, outgob, sizeof(goban_t));

  return;
}


static void
erode(goban_t gob)
{
  int m, mb;
  int i, j;
  goban_t temp_goban;
  goban_t temp_goban2;

  /*** different color or null => erode 1 point ***/
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      mb = gob[i][j];
      if (mb == 0) {
	temp_goban[i][j] = 0;
	continue;
      }

      m = 0;
      if (i > 0)
	if (gob[i-1][j] * mb <= 0)
	  m++;

      if (i < board_size - 1)
	if (gob[i+1][j] * mb <= 0)
	  m++;

      if (j > 0)
	if (gob[i][j-1] * mb <= 0)
	  m++;

      if (j < board_size - 1)
	if (gob[i][j+1] * mb <= 0)
	  m++;

      if (mb > 0) {
	if (m > mb)
	  mb = 0;
	else
	  mb -= m;
      } else {
	if (m > -mb)
	  mb = 0;
	else
	  mb += m;
      }
      temp_goban[i][j] = mb;
    }
  }

  /* Check the result with the old version. */
  /* FIXME: Remove this when we are sure the new version works OK. */
  memcpy(temp_goban2, gob, sizeof(goban_t));
  erosion(temp_goban2);
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      assert(temp_goban[i][j] == temp_goban2[i][j]);
    }
  }

  memcpy(gob, temp_goban, sizeof(goban_t));

  return;
}


/* ================================================================ */
/*                test & printing functions                         */
/* ================================================================ */


void
who_wins(int color, float fkomi, FILE * stdwhat)
{
  float white_score;
  float black_score;
  float result;
  int winner;

  if (color != BLACK && color != WHITE)
    color = BLACK;

  make_moyo(color);
  white_score = (float) terri_eval[WHITE] + fkomi;
  black_score = (float) terri_eval[BLACK];
  if (white_score > black_score) {
    winner = WHITE;
    result = white_score - black_score;
  } else {
    winner = BLACK;
    result = black_score - white_score;
  }

  fprintf(stdwhat, "Result: %c+%.1f   ",
	  (winner == WHITE) ? 'W' : 'B', result);
  if (color == winner)
    fprintf(stdwhat, "%c says \"I win!\"\n", (color == WHITE) ? 'W' : 'B');
  else
    fprintf(stdwhat, "%c says \"I lost!\"\n", (color == WHITE) ? 'W' : 'B');
}



void
print_moyo(int color)
{
  if (printmoyo & 1)
    print_ascii_moyo(terri_goban);

  if (printmoyo & 2)
    print_delta_terri(color);

  if (printmoyo & 4)
    print_ascii_moyo(moyo_goban);

  if (printmoyo & 8)
    print_delta_moyo(color);

  if (printmoyo & 16)
    print_ascii_area();

  if (printmoyo & 32)
    print_txt_area();

  if (printmoyo & 64)
    print_txt_connect(color);
}


static void
print_delta_moyo(int color)
{
  int i, j;

#ifdef CURSES
  init_curses();
#endif

  fprintf(stderr, "delta_moyo :\n");
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      switch (p[i][j]) {
      case EMPTY:
	fprintf(stderr, "%3d", delta_moyo(i, j, color));
	break;
      case BLACK:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(5, 'X');
	break;
      case WHITE:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(2, 'O');
	break;
      }
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n");
}


static void
print_delta_terri(int color)
{
  int i, j;

#ifdef CURSES
  init_curses();
#endif

  fprintf(stderr, "delta_terri :\n");
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      switch (p[i][j]) {
      case EMPTY:
	fprintf(stderr, "%3d", delta_terri(i, j, color));
	break;
      case BLACK:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(5, 'X');
	break;
      case WHITE:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(2, 'O');
	break;
      }
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n");
}


#if 0
static void
print_delta_terri_color(int ti,int tj,int color)
{
  int i, j;

  gprintf("delta_terri_color %m :\n",ti,tj);
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      fprintf(stderr, "%3d", delta_moyo_color(ti, tj, color,i,j));
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n");
}
#endif


static void
print_txt_connect(int color)
{
  int i, j, k;

#ifdef CURSES
  init_curses();
#endif

  fprintf(stderr, "meta_connect :\n");
  for (i = 0; i < board_size; i++) {
    for (j = 0; j < board_size; j++) {
      switch (p[i][j]) {
      case EMPTY:
	k = meta_connect(i, j, color);
	if (k == 0)
	  fprintf(stderr, "  .");
	else
	  fprintf(stderr, "%3d", meta_connect(i, j, color));
	break;
      case BLACK:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(5, 'X');
	break;
      case WHITE:
	fprintf(stderr, " ");
	DRAW_COLOR_CHAR(2, 'O');
	break;
      }
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n");
}


static void
print_ascii_moyo(goban_t gob)
{
  int i, j, k;

#ifdef CURSES
  init_curses();
#endif

  if (board_size == 9)
    fprintf(stderr, "   A B C D E F G H J\n");
  else if (board_size == 13)
    fprintf(stderr, "   A B C D E F G H J K L M N\n");
  else
    fprintf(stderr, "   A B C D E F G H J K L M N O P Q R S T\n");

  for (i = 0; i < board_size; i++) {
    fprintf(stderr, "%2d", board_size - i);
    for (j = 0; j < board_size; j++) {
      if (p[i][j] && dragon[i][j].safety != DEAD) {
	k = p[i][j];
      } else {
	k = EMPTY;
      }
      switch (k) {
      case EMPTY:
	if (gob[i][j] > 0)
	  DRAW_COLOR_CHAR(5, 'b');
	else if (gob[i][j] < 0)
	  DRAW_COLOR_CHAR(2, 'w');
	else
	  fprintf(stderr, " .");
	break;
      case BLACK:
	fprintf(stderr, " X");
	break;
      case WHITE:
	fprintf(stderr, " O");
	break;
      }
    }
    fprintf(stderr, " %d", board_size - i);

    if (i == 5)
      fprintf(stderr, "     White territory %d", terri_eval[WHITE]);

    if (i == 7)
      fprintf(stderr, "     Black territory %d", terri_eval[BLACK]);

    if (i == 9)
      fprintf(stderr, "   W/B moyo %d/%d : %d",
	      moyo_eval[WHITE], moyo_eval[BLACK], moyo_eval[0]);

    fprintf(stderr, "\n");
  }

  if (board_size == 9)
    fprintf(stderr, "   A B C D E F G H J\n");
  else if (board_size == 13)
    fprintf(stderr, "   A B C D E F G H J K L M N\n");
  else
    fprintf(stderr, "   A B C D E F G H J K L M N O P Q R S T\n");
}


/* ================================================================ */
/*                         some test functions                      */
/* ================================================================ */


static void
print_ascii_area(void)
{
  int i, j, k;

#ifdef CURSES
  init_curses();
#endif

  if (board_size == 9)
    fprintf(stderr, "   A B C D E F G H J\n");
  else if (board_size == 13)
    fprintf(stderr, "   A B C D E F G H J K L M N\n");
  else
    fprintf(stderr, "   A B C D E F G H J K L M N O P Q R S T\n");

  for (i = 0; i < board_size; i++) {
    fprintf(stderr, "%2d", board_size - i);
    for (j = 0; j < board_size; j++) {
      if (p[i][j] && dragon[i][j].status != DEAD) {
	k = p[i][j];
      } else {
	k = EMPTY;
      }
      switch (k) {
      case EMPTY:
	if (area_color(i, j) == BLACK)
	  DRAW_COLOR_CHAR(5, 'b');
	else if (area_color(i, j) == WHITE)
	  DRAW_COLOR_CHAR(2, 'w');
	else
	  fprintf(stderr, " .");
	break;
      case BLACK:
	DRAW_COLOR_CHAR(5, 'X');
	break;
      case WHITE:
	DRAW_COLOR_CHAR(2, 'O');
	break;
      }
    }
    fprintf(stderr, " %d", board_size - i);

    if (i == 5)
      fprintf(stderr, "     White territory %d", terri_eval[WHITE]);

    if (i == 7)
      fprintf(stderr, "     Black territory %d", terri_eval[BLACK]);

    if (i == 9)
      fprintf(stderr, "   W/B moyo %d/%d : %d",
	      moyo_eval[WHITE], moyo_eval[BLACK], moyo_eval[0]);

    fprintf(stderr, "\n");
  }

  if (board_size == 9)
    fprintf(stderr, "   A B C D E F G H J\n");
  else if (board_size == 13)
    fprintf(stderr, "   A B C D E F G H J K L M N\n");
  else
    fprintf(stderr, "   A B C D E F G H J K L M N O P Q R S T\n");
}


static void
print_txt_area(void)
{
  int i;

  for (i = 1; i <= areas_level; i++)
    if (area_array[i].color > EMPTY)
      gprintf("area %d  %m: color %c,  %d stone  %d spaces\n",
	      i, area_array[i].m, area_array[i].n,
	      area_array[i].color == WHITE ? 'W' : 'B',
	      area_array[i].stone, area_array[i].space);
}


void
sgfShowMoyo(int mode, int as_variant)
{
  int i,j;
  SGFNodeP startnode=0;
  SGFNodeP pr=startnode;

  if (as_variant) {
      startnode=sgfStartVariant(0);
      pr=sgfAddComment(startnode,"Moyo");
  }

  for (i=0; i < board_size; i++)
    for (j=0; j < board_size; j++) {
      switch(mode) {
      case SGF_SHOW_VALUE:
	sgfBoardNumber(pr,i,j,moyo_goban[i][j]);
	break;
      case SGF_SHOW_COLOR:
	if ((p[i][j]==EMPTY) ||(dragon[i][j].status==DEAD)){
	  if (moyo_goban[i][j] > 0)
	    sgfTerritory(pr,i,j,BLACK);
	  else if (moyo_goban[i][j] < 0)
	    sgfTerritory(pr,i,j,WHITE);
	}
	break;
      }
    }
}


void
sgfShowTerri(int mode, int as_variant)
{
  int i,j;
  SGFNodeP startnode=0;
  SGFNodeP pr=startnode;

  if (as_variant) {
    startnode=sgfStartVariant(0);
    pr=sgfAddComment(startnode,"Terri");
  }

  for (i=0; i < board_size; i++)
    for (j=0; j < board_size; j++) {
      switch (mode) {
      case SGF_SHOW_VALUE:
	sgfBoardNumber(pr,i,j,terri_goban[i][j]);
	break;
      case SGF_SHOW_COLOR:
	if ((p[i][j]==EMPTY) ||(dragon[i][j].status==DEAD)) {
	  if (terri_goban[i][j] > 0)
	    /*FIXME better moyo_goban?*/
	    sgfTerritory(pr,i,j,BLACK);
	  else if (terri_goban[i][j] < 0)
	    sgfTerritory(pr,i,j,WHITE);
	}
	break;
      }
    }
}


void
sgfShowArea(int mode, int as_variant)
{
  int i,j,col;
  SGFNodeP startnode=0;
  SGFNodeP pr=startnode;

  if (as_variant) {
    startnode=sgfStartVariant(0);
    pr=sgfAddComment(startnode,"Area");
  }

  for (i=0; i < board_size; i++)
    for (j=0; j < board_size; j++) {
      switch (mode) {
      case SGF_SHOW_VALUE:
	sgfBoardNumber(pr,i,j,area_grid[i][j]);
	break;
      case SGF_SHOW_COLOR:
	if ((p[i][j]==EMPTY) ||(dragon[i][j].status==DEAD)) {
	  col=area_color(i,j);
	  if((col==BLACK)||(col==WHITE))
	    sgfTerritory(pr,i,j,col);
	}
	break;
      }
    }
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
