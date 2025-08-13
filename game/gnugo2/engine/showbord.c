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





/*-------------------------------------------------------------
  showbord.c -- Show current go board and playing information
-------------------------------------------------------------*/

/* 
 * NOTE : this is no longer intended as the main user interface
 * as it was in GNU Go 1.2. It is now a debugging aid, showing
 * the internal state of dragons, and things. But with
 * color enabled, it should be easy enough to see the state
 * of play at a glance.
 *
 * Note : the dragons must have been calculated before this is called
 */

/* define CURSES or ANSI_COLOR to enable coloring of pieces
 * define JUST_USE_X_AND_O to use X,O instead of A,B,C,...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "liberty.h"

#ifdef CURSES

# ifdef _AIX
#  define _TPARM_COMPAT
# endif
#include <curses.h>
#include <term.h>
#endif

#define xor(x, y) (((x) && !(y)) || (!(x) && (y)))
#define unique_color(i, j) xor(black_domain[i][j], white_domain[i][j])

/*
 * Stuff to enumerate the dragons 
 */

/* Element at origin of each worm stores allocated worm number. */
static unsigned char dragon_num[MAX_BOARD][MAX_BOARD];

static int next_white;		/* next worm number to allocate */
static int next_black;


#ifdef CURSES
/* terminfo attributes */
static char *setaf;		/* terminfo string to set color */
static int   max_color;		/* terminfo max colour */


/* 
 * Initialize the curses code.
 */

static void
init_curses(void)
{
  static int initialized = 0;

  /* The compiler is set to make string literals  const char *,
   * but the system header files don't prototype things correctly.
   *
   * These are equivalent to a non-const string literals
   */
  static char setaf_literal[]="setaf";
  static char colors_literal[]="colors";
  static char empty_literal[]="";

  if (initialized)
    return;
  initialized = 1;

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
  fprintf(stderr, " %s%c", tparm(setaf,(c), 0, 0, 0, 0, 0, 0, 0, 0, 0),(x)); \
  fputs(tparm(setaf,max_color, 0, 0, 0, 0, 0, 0, 0, 0, 0), stderr); \
} while(0)

#elif defined(ANSI_COLOR)

#define max_color 7

#define DRAW_COLOR_CHAR(c,x) \
     fprintf(stderr, " \033[%dm%c\033[0m", 30+(c), (x))

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

static const int colors[3][4] = {
  {0,0,0,0}, /*not used */
  {6,2,1,3}, /* WHITE : dead, alive, critical, unknown */
  {6,2,1,3}  /* BLACK : dead, alive, critical, unknown */
};

static const int domain_colors[4] = {7, 1, 2, 3}; /* gray, black, white, both */

/* 
 * Write one stone. Use 'empty' if the board is empty ('-' or '+')
 * We use capital letters A,B,... for black, lower case a,b,... for white.
 * This allows us to indicate up to 26 dragons uniquely, and more with
 * low risk of ambiguity.
 */

/* The variable xo=1 if running gnugo -T, >1 if running gnugo -E or -T -T */

static void 
showchar(int i, int j, int empty, int xo)
{
  struct dragon_data *d = &(dragon[i][j]);  /* dragon data at i,j */
  int x = p[i][j];

  if (x == EMPTY) {
    if (xo <= 1)
      fprintf(stderr, " %c", empty);
    else {
      int  empty_color;
      char empty_char;
      
      if (black_eye[i][j].color==BLACK_BORDER) {
	if (white_eye[i][j].color==WHITE_BORDER)
	  empty_color=domain_colors[3];
	else {
	  empty_color=domain_colors[1];
	}
	if (black_eye[i][j].marginal)
	  empty_char='!';
	else
	  empty_char='x';
      }
      else if (white_eye[i][j].color==WHITE_BORDER) {
	empty_color=domain_colors[2];
	if (white_eye[i][j].marginal)
	  empty_char='!';
	else empty_char='o';
      }
      else {
	empty_color=domain_colors[0];
	empty_char='.';
      }

      DRAW_COLOR_CHAR(empty_color, empty_char);
    }
  }
  else {
    /* Figure out ascii character for this dragon. This is the
     * dragon number allocated to the origin of this worm. */
    int w = dragon_num[d->origini][d->originj];

    if (!w) {
      /* Not yet allocated - allocate next one. */
      /* Count upwards for black, downwards for white to reduce confusion. */
      if (p[i][j] == BLACK)
	w = dragon_num[d->origini][d->originj] = next_black++;
      else
	w = dragon_num[d->origini][d->originj] = next_white--; 
    }

    w = w%26 + (p[i][j] == BLACK ? 'A' : 'a');
    
    /* Now draw it. */
    if (xo>1) {
      if (p[i][j]==BLACK)
	DRAW_COLOR_CHAR(domain_colors[1], 'X');
      else
	DRAW_COLOR_CHAR(domain_colors[2], 'O');
    }
    else {
      if (d->safety == DEAD) {
	if (0) fprintf(stderr, " %c", w);
	DRAW_COLOR_CHAR( colors[p[i][j]][0], w);
      }
      else
	DRAW_COLOR_CHAR( colors[p[i][j]][d->safety], w);
    }
  }
}




/*
 * Show go board.
 */

void
showboard(int xo)
{
   int i, j, ii;

#ifdef CURSES
   init_curses();
#endif

   /* Set all dragon numbers to 0. */
   memset(dragon_num, 0, sizeof(dragon_num));

   next_white = (259 - 26);
   next_black = 26;



   if (board_size==9)
     fprintf(stderr,"   A B C D E F G H J\n");
   else if (board_size==13)
     fprintf(stderr,"   A B C D E F G H J K L M N\n");
   else
     fprintf(stderr,"   A B C D E F G H J K L M N O P Q R S T\n");

   for (i = 0; i < board_size; i++) {
     ii = board_size - i;
     fprintf(stderr,"%2d",ii);

     for (j = 0; j < board_size; j++)
       showchar(i,j, (i-3)%6 == 0 && (j-3)%6 == 0 ? '+' : '.', xo);

     fprintf(stderr," %d",ii);

     if ((board_size < 10 && i == board_size - 2) 
	 || (board_size >= 10 && i == 8))
       fprintf(stderr,"     WHITE has captured %d pieces", white_captured);

     if ((board_size < 10 && i == board_size - 1)
	 || (board_size >= 10 && i == 9))
       fprintf(stderr,"     BLACK has captured %d pieces", black_captured);

     fprintf(stderr, "\n");
   }

   if (board_size==9)
     fprintf(stderr,"   A B C D E F G H J\n");
   else if (board_size==13)
     fprintf(stderr,"   A B C D E F G H J K L M N\n");
   else
     fprintf(stderr,"   A B C D E F G H J K L M N O P Q R S T\n");

}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
