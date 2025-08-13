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
#include <stdlib.h>
#include <assert.h>

#include "liberty.h"

#define UPPER_LEFT  1
#define UPPER_RIGHT 2
#define LOWER_LEFT  4
#define LOWER_RIGHT 8

#define V_FIRST  870		/* base value for first corner stone */
#define V_ENCLOS 840		/* base value for enclosure          */
#define V_KAKARI 825		/* base value for approach           */
#define V_OBA    760		/* base value for side big move      */


/* constants for easier tunning */
static const int no_stone = 0;	/* no stone in that corner */
static const int my_open = 1;	/* some known first corner move, not 4-4 or 3-3 */
static const int you_open = 2;
static const int my_hoshi = 3;	/* 4-4 or 3-3 */
static const int you_hoshi = 4;
static const int my_one = 5;	/* some unknown first corner move */
static const int you_one = 6;
/* 2 stones */
static const int my_shimari = 7;
static const int you_shimari = 8;
static const int my_kakari = 9;
static const int you_kakari = 10;
static const int my_gray = 11;
static const int you_gray = 12;
static const int my_2 = 27;
static const int you_2 = 28;
/* 3 stones */
static const int my_3 = 13;
static const int you_3 = 14;
static const int my_gray_shimari = 15;
static const int you_gray_shimari = 16;
static const int my_pincer = 17;
static const int you_pincer = 18;
static const int my_3_gray = 19;
static const int you_3_gray = 20;
/* 4 stones */
static const int my_4 = 21;
static const int you_4 = 22;
static const int my_4_gray = 23;
static const int you_4_gray = 24;
static const int gray_4 = 25;
/* 5 stones and more */
static const int played_5 = 26;


static int corner_done, fuseki_ended;
static int firstmove = 1;

typedef struct {
  int nb_stone;
  int color;
  int closed;
  int kado;
  int coord[4];
} corner_status;

static corner_status angle[4];	/*initialized here to 0 */
static corner_status short_angle[4];	/* initialized here to 0 */

/*****************************************************************/
static int where(int i);
static int right(int i);
static void relative(int *i, int *j, int corner);
static int composite(int i, int j, int corner);
static void decompose(int i, int corner, int *m, int *n);
static void absolute(int corner, int *m, int *n);
static void analyze_corner(corner_status * cs, int corner, int color);
static int readcorner(int color, int corner);
static int play_empty_corner(int *m, int *n, int *val, int *equal_moves, int color);
static int play_somewhere(int *m, int *n, int *val, int *equal_moves, int color);
static int choose_second(corner_status * ang, int corner, int *m, int *n, int color, int sym);

static int openregion(int i1, int i2, int j1, int j2);
static void absolute_coord_old(int corner, int *m, int *n);
static void analyze_sides(int corner, int *table, int color);
static void choose_corner_move(int corner, int *m, int *n);
static void maybe_oba(int *bestval, int *besti, int *bestj, int ti, int tj, int color);
static int search_oba(int *i, int *j, int color);
static int search_small_oba(int *i, int *j, int color);
static void maybe_extend(int *bestval, int *besti, int *bestj, int ti, int tj, int color);
static int search_extend(int *i, int *j, int color);
static int known_stone(int km);
static int known_stone_2(int km);
static int area_color_c(int compos, int corner);
int fuseki(int *i, int *j, int *val, int *equal_moves, int color);
static int fuseki_19(int *i, int *j, int *val, int *equal_moves, int color);
/*****************************************************************/

/* The corner moves. */

static int corners[][2] =
{
  {3,3},
  {3,4},
  {4,3},
  {4,4},
  {5,3},
  {3,5},
  {5,4},
  {4,5},
};

/* Relative weights for different corner moves at different board
   sizes. */

/* up to 11x11 */
static int small_board[] =
{
  50,       /* 3-3 */
  18,       /* 3-4 */
  17,       /* 4-3 */
  15,       /* 4-4 */
  0,        /* 5-3 */
  0,        /* 3-5 */
  0,        /* 5-4 */
  0,        /* 4-5 */
};

/* 12x12 to 15x15 */
static int medium_board[] =
{
  30,       /* 3-3 */
  20,       /* 3-4 */
  20,       /* 4-3 */
  22,       /* 4-4 */
  2,        /* 5-3 */
  2,        /* 3-5 */
  2,        /* 5-4 */
  2,        /* 4-5 */
};

/* 16x16 and larger */
static int large_board[] =
{
  15,       /* 3-3 */
  15,       /* 3-4 */
  15,       /* 4-3 */
  35,       /* 4-4 */
  5,        /* 5-3 */
  5,        /* 3-5 */
  5,        /* 5-4 */
  5,        /* 4-5 */
};


static int known_stone(int km)
{
  switch (km) {
  case 304:
  case 305:
  case 403:
  case 405:
  case 503:
  case 504:
  case 303:
  case 404:
    return 1;
  }
  return 0;
}

static int known_stone_2(int km)
{
  switch (km) {
  case 304:
  case 305:
  case 403:
  case 405:
  case 503:
  case 504:
  case 303:
  case 404:
  case 603:
  case 604:
  case 306:
  case 406:
    return 1;
  }
  return 0;
}


static int where(int i)
{
  switch (i) {
  case UPPER_LEFT:
    return 0;
  case UPPER_RIGHT:
    return 1;
  case LOWER_LEFT:
    return 2;
  case LOWER_RIGHT:
    return 3;
  }
  ASSERT((i == UPPER_LEFT || i == UPPER_RIGHT || i == LOWER_LEFT || i == LOWER_RIGHT), 1, 1);
  return -1;
}

static int right(int i)
{
  switch (i) {
  case UPPER_LEFT:
    return LOWER_LEFT;
  case UPPER_RIGHT:
    return UPPER_LEFT;
  case LOWER_LEFT:
    return LOWER_RIGHT;
  case LOWER_RIGHT:
    return UPPER_RIGHT;
  }
  ASSERT((i == UPPER_LEFT || i == UPPER_RIGHT || i == LOWER_LEFT || i == LOWER_RIGHT), 1, 1);
  return -1;
}



/* check if region from i1, j1 to i2, j2 is open */

static int openregion(int i1, int i2, int j1, int j2)
{
  int x, y;

  if (i1 > i2)
    return openregion(i2, i1, j1, j2);
  if (j1 > j2)
    return openregion(i1, i2, j2, j1);
  for (x = i1; x <= i2; x++)
    for (y = j1; y <= j2; y++)
      if (p[x][y] != EMPTY)
	return 0;
  return 1;
}

static void analyze_corner(corner_status * cs, int corner, int color)
{
  int other;
  other = OTHER_COLOR(color);

  switch (cs->nb_stone) {
  case 0:
    cs->kado = no_stone;
    break;
  case 1:
    if (known_stone(cs->coord[0])) {
      if (cs->coord[0] == 303 || cs->coord[0] == 404)
	cs->kado = my_hoshi;
      else
	cs->kado = my_open;
    } else {
      cs->kado = my_one;
    }
    if (cs->color == other)
      cs->kado++;
    break;
  case 2:
    if (known_stone_2(cs->coord[0]) && known_stone_2(cs->coord[1])) {
      if (cs->color == color) {
	cs->kado = my_shimari;
      } else if (cs->color == other) {
	cs->kado = you_shimari;
      } else {
	if (cs->coord[0] == 303 || cs->coord[1] == 303) {
	  if (area_color_c(303, corner) == color)
	    cs->kado = my_kakari;
	  else
	    cs->kado = you_kakari;
	} else if (cs->coord[0] == 304 || cs->coord[1] == 304) {
	  if (area_color_c(304, corner) == color)
	    cs->kado = my_kakari;
	  else
	    cs->kado = you_kakari;
	} else if (cs->coord[0] == 403 || cs->coord[1] == 403) {
	  if (area_color_c(403, corner) == color)
	    cs->kado = my_kakari;
	  else
	    cs->kado = you_kakari;
	} else if (cs->coord[0] == 305 || cs->coord[1] == 305) {
	  if (area_color_c(305, corner) == color)
	    cs->kado = my_kakari;
	  else
	    cs->kado = you_kakari;
	} else if (cs->coord[0] == 503 || cs->coord[1] == 503) {
	  if (area_color_c(503, corner) == color)
	    cs->kado = my_kakari;
	  else
	    cs->kado = you_kakari;
	} else if (area_color_c(303, corner) == color)
	  cs->kado = my_gray;
	else
	  cs->kado = you_gray;
      }
    } else {
      if (cs->color == color) {
	cs->kado = my_2;
      } else if (cs->color == other) {
	cs->kado = you_2;
      } else {
	if (area_color_c(303, corner) == color)
	  cs->kado = my_gray;
	else
	  cs->kado = you_gray;
      }
    }
    break;
  case 3:
    if (cs->color == color)
      cs->kado = my_3;
    else if (cs->color == other)
      cs->kado = you_3;
    else {
      int c1, c2, c3;
      c1 = area_color_c(cs->coord[0], corner);
      c2 = area_color_c(cs->coord[1], corner);
      c3 = area_color_c(cs->coord[2], corner);
      if ((c1 == color && (c2 == color || c3 == color)) || (c2 == color && c3 == color))
	cs->kado = my_3_gray;
      else
	cs->kado = you_3_gray;

/*look for pincer, or approched shimari */
      c1 = area_color_c(303, corner);
      c2 = area_color_c(306, corner);
      c3 = area_color_c(603, corner);

      if (c2 == c3 && c1 != c2) {	/* pincer ! */
	if (c1 == color)
	  cs->kado = my_pincer;
	else if (c1 == other)
	  cs->kado = you_pincer;
      } else if (c1 == color && cs->kado == my_gray && (c1 == c2 || c1 == c3)) {
	cs->kado = my_gray_shimari;	/* approched enclosure ! */
      } else if (c1 == other && cs->kado == you_gray && (c1 == c2 || c1 == c3)) {
	cs->kado = you_gray_shimari;	/* approched enclosure ! */
      }
    }
    break;
  case 4:
    if (cs->color == color)
      cs->kado = my_4;
    else if (cs->color == other)
      cs->kado = you_4;
    else {
      int c1, c2;
      c1 = c2 = 0;
      if (area_color_c(cs->coord[0], corner) == color)
	c1++;
      else
	c2++;
      if (area_color_c(cs->coord[1], corner) == color)
	c1++;
      else
	c2++;
      if (area_color_c(cs->coord[2], corner) == color)
	c1++;
      else
	c2++;
      if (area_color_c(cs->coord[3], corner) == color)
	c1++;
      else
	c2++;
      if (c1 == 3)
	cs->kado = my_4_gray;
      else if (c2 == 3)
	cs->kado = you_4_gray;
      else
	cs->kado = gray_4;
    }
    break;
  default:
    cs->kado = played_5;
  }				/*switch */
  return;
}

/* update angle[], return 1 if two last moves found */
static int readcorner(int color, int corner)
{
  int i1, i2, j1, j2, x, y, cmp;
  corner_status cs, scs;
  int nbmove = 0;

  i1 = i2 = j1 = j2 = 0;

  if (short_angle[where(corner)].closed)
    return 0;

  switch (corner) {
  case UPPER_LEFT:
    i1 = 0;
    i2 = 7;
    j1 = 0;
    j2 = 7;
    break;
  case UPPER_RIGHT:
    i1 = 0;
    i2 = 7;
    j1 = 11;
    j2 = 18;
    break;
  case LOWER_LEFT:
    i1 = 11;
    i2 = 18;
    j1 = 0;
    j2 = 7;
    break;
  case LOWER_RIGHT:
    i1 = 11;
    i2 = 18;
    j1 = 11;
    j2 = 18;
    break;
  }

  cs.nb_stone = 0;
  cs.color = EMPTY;
  cs.closed = 0;
  cs.kado = 0;
  cs.coord[0] = cs.coord[1] = cs.coord[2] = cs.coord[3] = 0;
  scs = cs;

  for (x = i1; x <= i2; x++)
    for (y = j1; y <= j2; y++)
      if (p[x][y] != EMPTY) {
	cs.nb_stone++;
	/* 0+1=1 0+2=2 1+1=1 1+2=gray 2+2=2 */
	cs.color |= p[x][y];
	cmp = composite(x, y, corner);
	if (cs.nb_stone <= 4)
	  cs.coord[cs.nb_stone - 1] = cmp;
	if (cmp < 700 && cmp % 100 < 7) {
	  scs.nb_stone++;
	  scs.color |= p[x][y];
	  if (scs.nb_stone <= 4)
	    scs.coord[scs.nb_stone - 1] = cmp;
	}
      }
  if (cs.nb_stone != angle[where(corner)].nb_stone) {

/* at least one last 2 moves is in that corner */
    if (cs.nb_stone == angle[where(corner)].nb_stone + 2)
      nbmove = 2;
    else
      nbmove = 1;

/* finding what is the situation in the corner */
    analyze_corner(&cs, corner, color);

    angle[where(corner)].nb_stone = cs.nb_stone;
    angle[where(corner)].color = cs.color;
    angle[where(corner)].kado = cs.kado;
    angle[where(corner)].coord[0] = cs.coord[0];
    angle[where(corner)].coord[1] = cs.coord[1];
    angle[where(corner)].coord[2] = cs.coord[2];
    angle[where(corner)].coord[3] = cs.coord[3];

    if (cs.nb_stone >= 5)
      angle[where(corner)].closed = 1;
    }
  if (scs.nb_stone != short_angle[where(corner)].nb_stone) {

/* finding what is the situation in the corner */
    analyze_corner(&scs, corner, color);

    short_angle[where(corner)].nb_stone = scs.nb_stone;
    short_angle[where(corner)].color = scs.color;
    short_angle[where(corner)].kado = scs.kado;
    short_angle[where(corner)].coord[0] = scs.coord[0];
    short_angle[where(corner)].coord[1] = scs.coord[1];
    short_angle[where(corner)].coord[2] = scs.coord[2];
    short_angle[where(corner)].coord[3] = scs.coord[3];

    if (scs.nb_stone >= 5)
      short_angle[where(corner)].closed = 1;
  }
  return nbmove;
}

/*********************************************************************

3 systems of coord :

- absolute : [0..18] [0..18], the same that is used in p[m][n]
- relative to a corner : [1-10] [1-10] + corner, usefull to talk about 
  point 4 4 in upper right corner. Beware symetry ! 
  Just consider we are in upper left corner, and trust conversion functions.
- "composite" : way to write coord relative to a corner : 4-4 is number 404, 
  point 10-4 is number 1004, point 4-10 is 410.

warning about symetry :

relative & composite : 

u_left          u_right
  34         43
43 +    +    + 34

   +    +    +
   
34 +    +    + 43
  43         34
l_left          l_right

***************************************************************************/

/*from relative to absolute, too maintain compatibility with old fuseki.c */
static void absolute_coord_old(int corner, int *m, int *n)
{
    switch (corner) {
    case UPPER_LEFT:
      *m = *m - 1;
      *n = *n - 1;
      break;
    case UPPER_RIGHT:
      *m = *m - 1;
      *n = board_size - *n;
      break;
    case LOWER_LEFT:
      *m = board_size - *m;
      *n = *n - 1;
      break;
    case LOWER_RIGHT:
      *m = board_size - *m;
      *n = board_size - *n;
      break;
    }
}

/*from relative to absolute */
static void absolute(int corner, int *m, int *n)
{
  int s;
  switch (corner) {
  case UPPER_LEFT:
    *m = *m - 1;
    *n = *n - 1;
    break;
  case UPPER_RIGHT:
    s = *m;
    *m = *n - 1;
    *n = board_size - s;
    break;
  case LOWER_LEFT:
    s = *m;
    *m = board_size - *n;
    *n = s - 1;
    break;
  case LOWER_RIGHT:
    *m = board_size - *m;
    *n = board_size - *n;
    break;
  }
}

/*from absolute to relative */
static void relative(int *i, int *j, int corner)
{
  int s;
  switch (corner) {
  case UPPER_LEFT:
    (*i)++;
    (*j)++;
    break;
  case UPPER_RIGHT:
    s = *j;
    *j = *i + 1;
    *i = board_size - s;
    break;
  case LOWER_LEFT:
    s = *i;
    *i = *j + 1;
    *j = board_size - s;
    break;
  case LOWER_RIGHT:
    *i = board_size - *i;
    *j = board_size - *j;
    break;
  }
}

/* from absolute to composite */
static int composite(int i, int j, int corner)
{
  relative(&i, &j, corner);
  return (100 * i + j);
}

/* from composite to absolute */
static void decompose(int i, int corner, int *m, int *n)
{
  *m = i / 100;
  *n = i % 100;
  absolute(corner, m, n);
}

/* to get color around a "composite" stone */
static int area_color_c(int compos, int corner)
{
  int i, j;
  decompose(compos, corner, &i, &j);
  return area_color(i, j);
}

/***********************************************************************
   propose a move in a corner where one stone is
   already played, return the value of the move 
   ***********************************************************************/
static int 
choose_second(corner_status * ang, int corner, int *m, int *n, int color, int sym)
{
  int enclo_val, appro_val;
  int cor;
  int upper, left, first;
  int kado, kado_right, kado_left, kado_opposite;
  int stone_left, stone_right, color_right, color_left;
  int other, nb_mycol, nb_othercol, nb_gray;

#define SYM(x)	(sym?((x)/100+((x)%100)*100):(x))


  /* the position of the already played stone */
  first = ang[where(corner)].coord[0];
  if (!sym) {
    switch (first) {		/* use symetry */
    case 403:
    case 503:
    case 504:
      return choose_second(ang, corner, m, n, color, 1);
    }
  } else {
    first = SYM(first);
  }
  
  other = OTHER_COLOR(color);

  /* the stones position in this ang */
  kado = ang[where(corner)].kado;
  cor = right(corner);
  if (!sym) {
    kado_right = ang[where(cor)].kado;
    stone_right = ang[where(cor)].nb_stone;
    color_right = ang[where(cor)].color;
    cor = right(cor);
    kado_opposite = ang[where(cor)].kado;
    cor = right(cor);
    kado_left = ang[where(cor)].kado;
    stone_left = ang[where(cor)].nb_stone;
    color_left = ang[where(cor)].color;
  } else {
    kado_left = ang[where(cor)].kado;
    stone_left = ang[where(cor)].nb_stone;
    color_left = ang[where(cor)].color;
    cor = right(cor);
    kado_opposite = ang[where(cor)].kado;
    cor = right(cor);
    kado_right = ang[where(cor)].kado;
    stone_right = ang[where(cor)].nb_stone;
    color_right = ang[where(cor)].color;
  }


  enclo_val = V_ENCLOS;		/*base value for enclosure */
  appro_val = V_KAKARI;		/*base value for approach */
  

  upper = area_color_c(SYM(409), corner);	/* upper side color */
  left = area_color_c(SYM(904), corner);	/* left side color */

/* count how many corner we got */
  nb_mycol = nb_othercol = nb_gray = 0;
  cor = UPPER_RIGHT;
  do {
    if (ang[where(cor)].color == color)
      nb_mycol++;
    else if (ang[where(cor)].color == other)
      nb_othercol++;
    else
      nb_gray++;
    cor = right(cor);
  } while (cor != UPPER_RIGHT);
  
  /* if we got weak groups, decrease return value of kakari */
  appro_val -= number_weak(color) * 10;
  
  /* if we need to approach other corner */
  if ((nb_mycol + nb_gray) < nb_othercol)
    appro_val += 10;
  if (nb_mycol == 0)
    appro_val += 10;
  if (nb_mycol > nb_othercol) {
    enclo_val += 10;
    appro_val -= 10;
  }
  if (ang[where(corner)].color == color) {

/**********************************************************************/
/* we played first in this corner, look for corner enclosure value    */
/**********************************************************************/
    switch (first) {
    case 303:
      /* not an urgent fuseki move here, if any, 
	 it will be find by joseki pattern */
      {
	int i, j, ii, jj;
	int v = enclo_val;
	v -= 90;
	if (upper == color || left == color) {
	  v += 15;
	  if (upper == color && left == color)
	    decompose(404, corner, m, n);
	  else if (upper == color)
	    decompose(405, corner, m, n);
	  else if (left == color)
	    decompose(504, corner, m, n);
	  
	  if (color_left == other && stone_left >= 2)
	    v += 15;
	  if (color_right == other && stone_right >= 2)
	    v += 15;
	} else if (upper == other && left == other) {
	  v += 55;
	  decompose(306, corner, &i, &j);
	  decompose(603, corner, &ii, &jj);
	  if (diff_moyo(i, j, color) > diff_moyo(ii, jj, color))
	    decompose(306, corner, m, n);
	  else
	    decompose(603, corner, m, n);
	} else if (upper == other || left == other) {
	  decompose(306, corner, &i, &j);
	  decompose(603, corner, &ii, &jj);
	  if (diff_moyo(i, j, color) > diff_moyo(ii, jj, color))
	    decompose(306, corner, m, n);
	  else
	    decompose(603, corner, m, n);
	  v += 15;
	} else
	  v = 0;
	return v;
      }				/*303 */
      
    case 304:			/* corner enclosure */
      
      {
	int v = enclo_val;
	if (upper == color) {	/* we got upper side, the 3 extension side */
	  if (left == color) {
	    /* play high enclosure, maybe a double wing ! */
	    decompose(SYM(504), corner, m, n);
	    return v + 10;
	  } else if (left == other) {
	    /* play low enclosure */
	    decompose(SYM(503), corner, m, n);
	    return v;
	  } else {		/*empty on left */
	    /* play enclosure, merely high one */
	    if ((3.0 * rand() / (RAND_MAX + 1.0)) == 1)
	      decompose(SYM(503), corner, m, n);
	    else
	      decompose(SYM(504), corner, m, n);
	    if (area_color_c(SYM(1404), corner) == color)
	      return v + 5;
	    else
	      return v;
	  }
	} else if (upper == other) {
	  /* opponent on upper side, the big extension side */
	  if (left == color) {
	    /* play low enclosure */
	    decompose(SYM(503), corner, m, n);
	    return v;
	  } else if (left == other) {
	    /* play urgent low enclosure, since we are surrounded */
	    decompose(SYM(503), corner, m, n);
	    v += 30;
	    return v;
	  } else {		/* empty on left */
	    /* play enclosure, merely high one */
	    if ((3.0 * rand() / (RAND_MAX + 1.0)) == 1)
	      decompose(SYM(503), corner, m, n);
	    else
	      decompose(SYM(504), corner, m, n);
	    v += 8;
	    return v;
	  }
	} else {		/* empty on upper side */
	  v += 5;
	  if (left == color) {
	    if (area_color_c(SYM(414), corner) == color) {
	      /* play high enclosure, more high */
	      if ((4.0 * rand() / (RAND_MAX + 1.0)) == 1)
		decompose(SYM(503), corner, m, n);
	      else
		decompose(SYM(504), corner, m, n);
	    } else
	      decompose(SYM(503), corner, m, n);
	    v += 5;
	    return v;
	  } else if (left == other) {
	    v += 6;
	    /* play low enclosure, maybe large one */
	    decompose(SYM(503), corner, m, n);
	    return v + 1;
	  } else {		/* empty on both sides */
	    if ((3.0 * rand() / (RAND_MAX + 1.0)) == 1)
	      decompose(SYM(503), corner, m, n);
	    else
	      decompose(SYM(504), corner, m, n);
	    return v;
	  }
	}
      }				/*304 & 403 by sym */
      
    case 404:{
      /* friendly hoshi, not an urgent fuseki move here */
      /* if any, it will be find by joseki pattern */
      int i, j, ii, jj;
      int v = enclo_val;
      if (upper == other && left == other) {
	v -= 40;
	decompose(306, corner, &i, &j);
	decompose(603, corner, &ii, &jj);
	if (diff_moyo(i, j, color) > diff_moyo(ii, jj, color))
	  decompose(306, corner, m, n);
	else
	  decompose(603, corner, m, n);
      } else {
	v = 0;
      }
      return v;
    }				/*404 */
    
    
    case 305:{
      /* if we got one extension side, play enclosure */
      if (left == color || upper == color) {
	decompose(SYM(403), corner, m, n);
	return enclo_val;
      } else {
	/* no urgent move by here ? */
	decompose(SYM(403), corner, m, n);
	return enclo_val - 10;
      }
    }				/* 305 & 503 by sym */
    
    case 405:{
      /* if we got one extension side, play enclosure */
      if (left == color || upper == color) {
	decompose(SYM(403), corner, m, n);
	return enclo_val + 4;
      } else {
	/* no urgent move by here ? */
	decompose(SYM(403), corner, m, n);
	return enclo_val - 10;
      }
    }				/* 405 & 504 by sym */
    
    default:
      /* I played first in the corner, 
	 it's a strange move : wait and see */
      return 0;
    }				/* switch */
  }
  /*if (ang[where(corner)].color == color) */
/****************************************************************************/
/* opponent play first in this corner, look for kakari                      */
/****************************************************************************/
  else {
    switch (first) {
    case 303:{			/* 4-4 approach upon 3-3 */
      int tcorner;
      int v = appro_val;
      
      /* don't cap 3-3 if other corner move is possible */
      tcorner = right(corner);
      if (ang[where(tcorner)].nb_stone < 2)
	return 0;
      tcorner = right(tcorner);
      if (ang[where(tcorner)].nb_stone < 2)
	return 0;
      tcorner = right(tcorner);
      if (ang[where(tcorner)].nb_stone < 2)
	return 0;
      if (style & STY_FEARLESS)
	v += 15;
      decompose(404, corner, m, n);
      if (upper == color && left == color)
	return v - 5;
      else if (upper == color || left == color)
	return v - 15;
      else
	return v - 25;
    }				/*303 */

    case 304:
      {				/* kakari against 3-4 stone */
	int v = appro_val;
	
	if (upper == color && left == color) {
	  /* we have both sides, maybe play high */
	  if ((int) (3.0 * rand() / (RAND_MAX + 1.0)) == 1)
	    decompose(SYM(504), corner, m, n);
	  else
	    decompose(SYM(503), corner, m, n);
	  return v + 11;
	} else if (upper == color && left == other) {
	  /* we have only upper side, play high */
	  if ((int) (3.0 * rand() / (RAND_MAX + 1.0)) == 1)
	    decompose(SYM(503), corner, m, n);
	  else
	    decompose(SYM(504), corner, m, n);
	  return v + 3;
	} else {
	  /* otherwise, play low kakari ... */
	  decompose(SYM(503), corner, m, n);
	  return v;
	}
      }				/*304 & 403 by sym */
      
    case 404:{			/* try to approach on the open side, or 
				   on the side of my color */
      if (upper == EMPTY) {
	if (left == color) {
	  /* approach on my side */
	  decompose(603, corner, m, n);
	  return appro_val + 5;
	} else if (left == other) {
	  /* approach on empty side */
	  decompose(306, corner, m, n);
	  return appro_val;
	} else {
	  /* both sides empty, approach move less urgent, 
	     if one corner is of my color, chose this side */
	  if (area_color_c(414, corner) == color && area_color_c(1404, corner) == other) {
	    decompose(306, corner, m, n);
	    return appro_val - 8;
	  } else if (area_color_c(414, corner) == other && area_color_c(1404, corner) == color) {
	    decompose(603, corner, m, n);
	    return appro_val - 8;
	  } else {		/* chose by random, move less urgent */
	    /* play sometimes high approach move */
	    if ((int) (2.0 * rand() / (RAND_MAX + 1.0)) == 1) {
	      if ((int) (5.0 * rand() / (RAND_MAX + 1.0)) == 1)
		decompose(604, corner, m, n);
	      else
		decompose(603, corner, m, n);
	      return appro_val - 20;
	    } else {
	      if ((int) (5.0 * rand() / (RAND_MAX + 1.0)) == 1)
		decompose(406, corner, m, n);
	      else
		decompose(306, corner, m, n);
	      return appro_val - 20;
	    }
	  }
	}
      } else if (upper == color) {
	if (left == color) {	/* got both sides ! */
	  int il, jl, iu, ju;
	  decompose(904, corner, &il, &jl);
	  decompose(409, corner, &iu, &ju);
	  /* approach move less urgent, maybe we could play san-san invasion ? */
	  if (area_stone(il, jl) >= 3 && area_stone(iu, ju) >= 3
	      && area_space(il, jl) >= 16 && area_space(iu, ju) >= 16) {
	    /* launch early 3-3 invasion */
	    decompose(303, corner, m, n);
	    return appro_val - 25;
	  } else
	    /* play the side we are less strong */
	    if (area_space(iu, ju) < area_space(il, jl)) {
	      /* play upper */
	      decompose(306, corner, m, n);
	      return appro_val - 30;
	    } else if (area_space(iu, ju) > area_space(il, jl)) {
	      /* play left */
	      decompose(603, corner, m, n);
	      return appro_val - 30;
	    } else if ((int) (2.0 * rand() / (RAND_MAX + 1.0)) == 1) {
	      decompose(306, corner, m, n);
	      return appro_val - 30;
	    } else {
	      decompose(603, corner, m, n);
	      return appro_val - 30;
	    }
	} else {		/* approach from upper side */
	  decompose(306, corner, m, n);
	  return appro_val;
	}
      } else {		/*if upper == other */
	if (left == other) {	/* opponent got both sides ! */
	  int il, jl, iu, ju;
	  decompose(904, corner, &il, &jl);
	  decompose(409, corner, &iu, &ju);

	  /* maybe 3-3 invasion if opponent very strong on both sides */
	  if (area_space(il, jl) < area_stone(iu, ju)) {
	    /* play the side where opponent is less strong */
	    decompose(306, corner, m, n);
	    return appro_val - 12;
	  } else if (area_stone(il, jl) > area_stone(iu, ju)) {
	    decompose(603, corner, m, n);
	    return appro_val - 12;
	  } else if ((int) (2.0 * rand() / (RAND_MAX + 1.0)) == 1) {
	    decompose(306, corner, m, n);
	    return appro_val - 13;
	  } else {
	    decompose(603, corner, m, n);
	    return appro_val - 13;
	  }
	} else {		/* approach empty side or my side */
	  decompose(603, corner, m, n);
	  return appro_val;
	}
      }
      break;
    }				/*404 */
    

    case 305:{
      /* if we got side &  corner, try special 4-5 kakari */
      if (left == color && area_color_c(SYM(1404), corner) == color)
	decompose(SYM(504), corner, m, n);
      else			/* standard kakari */
	decompose(SYM(403), corner, m, n);
      return appro_val;
    }				/*305 & 503 by sym */
    
    case 405:{
      /* standard kakari is 4-3, but 3-3 is possible */
      if ((int) (5.0 * rand() / (RAND_MAX + 1.0)) == 1)
	decompose(303, corner, m, n);
      else
	decompose(SYM(403), corner, m, n);
      return appro_val;
    }				/*405  & 504 by sym */
    
    default:			/* we got a strange first move in the corner */
      {
	/* if 4-4 or 3-3 seems to be not owned by the opponent, play
	   there, this should be better than nothing, but low priority */
	if (area_color_c(404, corner) == EMPTY) {
	  decompose(404, corner, m, n);
	  return appro_val - 40;
	} else {		/* look around 3-3 */
	  if (area_color_c(303, corner) == EMPTY) {
	    decompose(303, corner, m, n);
	    return appro_val - 45;
	  } else {
	    if (area_color_c(403, corner) == EMPTY
		|| area_color_c(304, corner) == EMPTY) {
	      decompose(303, corner, m, n);
	      return appro_val - 48;
	    }
	  }
	}
	
	return 0;		/* no proposed move */
	
      }				/*default */
    }				/*switch */
  }
  
  return 0;			/* should not be possible */
}


/* change the probability table for first corner move if a
   play already occured on a side */

static void 
analyze_sides(int corner, int *table, int color)
{
  int scol;
  int other;
  
  if (style & STY_NO_FUSEKI)
    return;
  other = OTHER_COLOR(color);
  scol = area_color_c(904, corner);
  
  if (scol == other) {
    table[1] = 0;		/* 3-4 */
  } else if (scol == color) {
    table[1] += 10;		/* 3-4 */
    table[3] += 5;		/* 4-4 */
  } else {
    if (area_color_c(1404, corner) == other)
      table[1] -= 10;		/* 3-4 */
  }
  
  scol = area_color_c(409, corner);
  
  if (scol == other) {
    table[2] = 0;		/* 4-3 */
  } else if (scol == color) {
    table[2] += 10;		/* 4-3 */
    table[3] += 5;		/* 4-4 */
  } else {
    if (area_color_c(414, corner) == other)
      table[2] -= 10;		/* 4-3 */
  }
}


static void 
choose_corner_move(int corner, int *m, int *n)
{
  int *table=0;
  int sum_of_weights=0;
  int i;
  int q;
  
  if (board_size<=11)
    table = small_board;
  else if (board_size<=15)
    table = medium_board;
  else 
    table = large_board;
  
  for (i=0; i<8 ;i++)
    sum_of_weights += table[i];
  
  q = rand() % sum_of_weights;
  for (i=0; i<8; i++) {
    q -= table[i];
    if (q<0)
      break;
  }
  *m = corners[i][0];
  *n = corners[i][1];
  absolute_coord_old(corner, m, n);
}


static void 
maybe_extend(int *bestval, int *besti, int *bestj, int ti, int tj, int color)
{
  int oval;
  int swapi;
  int ajust;
  
  ajust = 2;
  
  if ((area_color(ti - 1, tj) == color
       || area_color(ti + 1, tj) == color)
      && openregion(ti - 2, ti + 1, tj - 2, tj + 2)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti - 1, tj, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti - 1;
      *bestj = tj;
    }
  }
  ti = board_size - ti - 1;
  if ((area_color(ti - 1, tj) == color
       || area_color(ti + 1, tj) == color)
      && openregion(ti - 2, ti + 1, tj - 2, tj + 2)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti + 1, tj, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti + 1;
      *bestj = tj;
    }
  }
  swapi = board_size - ti - 1;
  ti = tj;
  tj = swapi;
  if ((area_color(ti, tj - 1) == color
       && area_color(ti, tj + 1) == color)
      && openregion(ti - 2, ti + 2, tj - 2, tj + 1)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti, tj - 1, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj - 1;
    }
  }
  tj = board_size - tj - 1;
  if ((area_color(ti, tj - 1) == color
       && area_color(ti, tj + 1) == color)
      && openregion(ti - 2, ti + 2, tj - 2, tj + 1)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti, tj + 1, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj + 1;
    }
  }
}

static void 
maybe_oba(int *bestval, int *besti, int *bestj, int ti, int tj, int color)
{
  int oval;
  int swapi;
  int ajust;

  if (style & STY_FEARLESS)	/* play more on the 4th line */
    ajust = 3;
  else
    ajust = 7;

  if (area_color(ti - 1, tj) == EMPTY
      && area_color(ti + 1, tj) == EMPTY
      && openregion(ti - 1, ti + 1, tj - 2, tj + 2)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti - 1, tj, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti - 1;
      *bestj = tj;
    }
  }
  ti = board_size - ti - 1;
  if (area_color(ti - 1, tj) == EMPTY
      && area_color(ti + 1, tj) == EMPTY
      && openregion(ti - 1, ti + 1, tj - 2, tj + 2)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti + 1, tj, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti + 1;
      *bestj = tj;
    }
  }
  swapi = board_size - ti - 1;
  ti = tj;
  tj = swapi;
  if (area_color(ti, tj - 1) == EMPTY
      && area_color(ti, tj + 1) == EMPTY
      && openregion(ti - 2, ti + 2, tj - 1, tj + 1)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti, tj - 1, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj - 1;
    }
  }
  tj = board_size - tj - 1;
  if (area_color(ti, tj - 1) == EMPTY
      && area_color(ti, tj + 1) == EMPTY
      && openregion(ti - 2, ti + 2, tj - 1, tj + 1)) {
    oval = diff_moyo(ti, tj, color);
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj;
    }
    oval = diff_moyo(ti, tj + 1, color) + ajust;
    if (oval > *bestval) {
      *bestval = oval;
      *besti = ti;
      *bestj = tj + 1;
    }
  }
}
    
/* look for a big point on a side, than return its value */

static int 
search_oba(int *i, int *j, int color)
{
  int besti, bestj, bestval;

  besti = bestj = bestval = 0;

  maybe_oba(&bestval, &besti, &bestj, 3, 7, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 8, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 9, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 10, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 11, color);

  if (bestval) {
    *i = besti;
    *j = bestj;
    return V_OBA;
  } else
    return 0;
}

/* look for a medium point on a side, than return its value */

static int 
search_small_oba(int *i, int *j, int color)
{
  int besti, bestj, bestval;

  besti = bestj = bestval = 0;

  maybe_oba(&bestval, &besti, &bestj, 3, 6, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 5, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 12, color);
  maybe_oba(&bestval, &besti, &bestj, 3, 13, color);

  if (bestval) {
    *i = besti;
    *j = bestj;

    return V_OBA - 5;
  } else
    return 0;
}

/* look for an extension on a side, than return its value */
static int 
search_extend(int *i, int *j, int color)
{
  int besti, bestj, bestval;

  besti = bestj = bestval = 0;

  maybe_extend(&bestval, &besti, &bestj, 3, 7, color);
  maybe_extend(&bestval, &besti, &bestj, 3, 8, color);
  maybe_extend(&bestval, &besti, &bestj, 3, 9, color);
  maybe_extend(&bestval, &besti, &bestj, 3, 10, color);
  maybe_extend(&bestval, &besti, &bestj, 3, 11, color);

  if (bestval) {
    *i = besti;
    *j = bestj;
    return V_OBA - 10;
  } else {
    besti = bestj = bestval = 0;

    maybe_extend(&bestval, &besti, &bestj, 3, 6, color);
    maybe_extend(&bestval, &besti, &bestj, 3, 5, color);
    maybe_extend(&bestval, &besti, &bestj, 3, 12, color);
    maybe_extend(&bestval, &besti, &bestj, 3, 13, color);

    if (bestval) {
      *i = besti;
      *j = bestj;
      return V_OBA - 10;
    }
  }

  return 0;
}



/*****************************************************************

 MAIN FUSEKI FUNCTION generate move in empty corners

 *****************************************************************/

int fuseki(int *i, int *j, int *val, int *equal_moves, int color)
{
  int found_one=0;
  int width;  /* Side of the open region required in the corner. */
  int x,y;
  
  if (fuseki_ended)		/* quick exit if fuseki is over */
    return 0;
  
  if (board_size == 19) {
    /* complete fuseki function for 19 x 19 board */
    found_one = fuseki_19(i, j, val, equal_moves, color);
  } else {
    if (board_size < 9) {
      *val=95;
      for(x=board_size/2;x<board_size;x++)
	for (y = board_size / 2; y < board_size; y++) {
	  *i=x;
	  *j=y;
	  if(legal(x,y,color))
	    return 1;
        }
      *i=*j=-1;
      *val=-1;
      return 0;
    }
    if (board_size<=11) {
      /* For boards of size 11x11 or smaller we first go for the center point. */
      int middle=board_size/2;
      if (openregion(middle-1, middle+1, middle-1, middle+1)) {
	if (BETTER_MOVE(90, *val, *equal_moves)) {
	  *i=middle;
	  *j=middle;
	  found_one=1;
	}
      }
    }
    if (board_size>=18)
      width = 8;
    else
      width = board_size/2;
    
    /* fuseki function if size != 19 */
    
    if (!(corner_done & UPPER_RIGHT)) {
      if (openregion(0, width-1, board_size-width, board_size-1)) {
	if (BETTER_MOVE(85, *val, *equal_moves)) {
	  choose_corner_move(UPPER_RIGHT, i, j);
	  found_one=1;
	}
      } else
	corner_done |= UPPER_RIGHT;
    }
    if (!(corner_done & LOWER_LEFT)) {
      if (openregion(board_size-width,board_size-1,0,width-1)) {
	if (BETTER_MOVE(85, *val, *equal_moves)) {
	  choose_corner_move(LOWER_LEFT, i, j);
	  found_one=1;
	}
      } else
	corner_done |= LOWER_LEFT;
    }
    if (!(corner_done & LOWER_RIGHT)) {
      if (openregion(board_size-width,board_size-1,board_size-width,board_size-1)) {
	if (BETTER_MOVE(85, *val, *equal_moves)) {
	  choose_corner_move(LOWER_RIGHT, i, j);
	  found_one=1;
	}
      } else
	corner_done |= LOWER_RIGHT;
    }
    if (!(corner_done & UPPER_LEFT)) {
      if (openregion(0,width-1,0,width-1)) {
	if (BETTER_MOVE(85, *val, *equal_moves)) {
	  choose_corner_move(UPPER_LEFT, i, j);
	  found_one=1;
	}
      } else
	corner_done |= UPPER_LEFT;
    }
    if (corner_done == UPPER_RIGHT + LOWER_LEFT + LOWER_RIGHT + UPPER_LEFT)
      fuseki_ended = 1;
  }

  /* Normally we prefer to rely on the patterns to produce a move
   * after the first one on very small boards, but if they fail we
   * should at least not pass.
   */
  if (found_one && board_size<=9 && movenum>1)
    *val=1;
  
  if (found_one)
    TRACE("fuseki found : %m with value %d\n", *i, *j, *val);
  if (fuseki_ended)
    TRACE("fuseki ended\n");
  
  return found_one;
}


/***********************************************************************

fuseki function, for board size 19, return 1 if found a move, else 0

************************************************************************/

static int 
fuseki_19(int *i, int *j, int *val, int *equal_moves, int color)
{
  int found_one = 0;
  int corner;
  int lm = 0;
  
  /* update corner information */
  
  corner = UPPER_RIGHT;
  do {
    lm += readcorner(color, corner);
    corner = right(corner);
  } while ((firstmove || lm < 2) && corner != UPPER_RIGHT);
  
  /* look for empty corner to play */
  found_one = play_empty_corner(i, j, val, equal_moves, color);
  
  /* if we don't want complex fuseki, stop there */
  if (style & STY_NO_FUSEKI) {
    if (found_one) {
      *val -= (V_FIRST - 850);	/* so val is always 85 */
    } else {
      fuseki_ended = 1;
    }
  } else {
    /* real fuseki begin here : look for an approch move, shimari or side move */
    if (!found_one)
      found_one = play_somewhere(i, j, val, equal_moves, color);
  }
  
  /* convert internal value of move to external */
  if (found_one)
    *val = *val / 10;
  
  return found_one;
}



/* return 1 if a move in an empty corner is found */

static int 
play_empty_corner(int *m, int *n, int *val, int *equal_moves, int color)
{
  int corner;
  int found_empty = 0;
  int table[8];			/* probability of each possible move */
  int sum_of_weights;
  int q, i;

/* find some empty corner */
  corner = UPPER_RIGHT;

  do {
    if (angle[where(corner)].nb_stone == 0) {

      if (BETTER_MOVE((firstmove && color == BLACK && corner == UPPER_RIGHT) ? V_FIRST + 9 : V_FIRST, *val, *equal_moves)) {
	for (i = 0; i < 8; i++)
	  table[i] = large_board[i];
	sum_of_weights = 0;
	/* looking on the sides for changing probability of possible corner moves */
	analyze_sides(corner, table, color);
	
	for (i = 0; i < 8; i++)
	  sum_of_weights += table[i];
	q = (int) ((double) sum_of_weights *rand() / (RAND_MAX + 1.0));
	
	for (i = 0; i < 8; i++) {
	  q -= table[i];
	  if (q < 0)
	    break;
	}
	*m = corners[i][0];
	*n = corners[i][1];
	
	absolute(corner, m, n);
	found_empty = 1;
      }
    }
    corner = right(corner);
  } while (corner != UPPER_RIGHT);
  
  if (firstmove)
    firstmove = 0;

  return found_empty;
}


/* every corner is played, look for a second move in a corner */
static int play_somewhere(int *m, int *n, int *val, int *equal_moves, int color)
{
  int corner;
  int found_move = 0;
  int found_proposal = 0;
  int fval = 0;
  int i, j, minus;

  i = j = 0;
  minus = 0;

  /* decrease value to not disturb fight when more corners are disputed */
  corner = UPPER_RIGHT;
  do {
    if (short_angle[where(corner)].nb_stone <= 3
	&& short_angle[where(corner)].color == GRAY_BORDER)
      minus += 8;
    corner = right(corner);
  } while (corner != UPPER_RIGHT);


  /* find some corner with only one stone played */
  corner = UPPER_RIGHT;
  do {
    if (angle[where(corner)].nb_stone == 1) {
      fval = choose_second(angle, corner, &i, &j, color, 0) - minus;
      if (fval > 0) {
	found_proposal = 1;
	if (BETTER_MOVE(fval, *val, *equal_moves)) {
	  *m = i;
	  *n = j;
	  found_move = 1;
	}
      }
    }
    corner = right(corner);
  } while (corner != UPPER_RIGHT);


/*  search oba... */
  /* look for big points on middle side */

    fval = search_oba(&i, &j, color) - minus;
    if (fval > 0) {
    found_proposal = 1;
    if (BETTER_MOVE(fval, *val, *equal_moves)) {
      *m = i;
      *n = j;
      found_move = 1;
    }
  }
  /* find some corner with two stones played, using short_angle */
  corner = UPPER_RIGHT;
  do {
    if (angle[where(corner)].nb_stone == 2 && short_angle[where(corner)].nb_stone == 1) {
      fval = choose_second(short_angle, corner, &i, &j, color, 0) - minus - 15;
      if (fval > 0) {
	found_proposal = 1;
      if (BETTER_MOVE(fval, *val, *equal_moves)) {
	*m = i;
	*n = j;
	found_move = 1;
}
    }
  }
    corner = right(corner);
  } while (corner != UPPER_RIGHT);

  /* look for little big points near middle side */

  if (!found_proposal) {
    fval = search_small_oba(&i, &j, color) - minus;
    if (fval > 0) {
      found_proposal = 1;
      if (BETTER_MOVE(fval, *val, *equal_moves)) {
	*m = i;
	*n = j;
	found_move = 1;
      }
    }
  }
  /* look for early extension */

  if (!found_proposal) {
    fval = search_extend(&i, &j, color) - minus;
    if (fval > 0) {
      found_proposal = 1;
      if (BETTER_MOVE(fval, *val, *equal_moves)) {
	*m = i;
	*n = j;
	found_move = 1;
      }
    }
  }
  if (!found_proposal)
    fuseki_ended = 1;
  if ((style & STY_TENUKI) && found_move)
    *val = 850;
  return found_move;
}





/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
