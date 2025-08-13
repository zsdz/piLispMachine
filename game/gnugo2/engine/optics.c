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
#include <string.h>
#include <assert.h>
#include "liberty.h"
#include "eyes.c"

#define lively(i, j)         (!worm[i][j].inessential \
			      && ((worm[i][j].attacki == -1) \
				  || (worm[i][j].defendi != -1)))
#define black_inf(i, j)      (black_domain[i][j] \
			      || (p[i][j]==BLACK && lively(i, j)))
#define white_inf(i, j)      (white_domain[i][j] \
			      || (p[i][j]==WHITE && lively(i, j)))
#define hadj(a, b, c, d)     ((half_eye[a][b].type==HALF_EYE) \
			      && (half_eye[a][b].ki==c) \
			      && (half_eye[a][b].kj==d))

/*
 * Two eye points are defined to be adjacent if they are either
 * next to each other or if one vertice is a half eye and the
 * other one is the point making it a real eye.
 */

#define adjacent(a, b, c, d) (((a==c) && ((b==d+1)||(b==d-1))) \
			      || ((b==d) && ((a==c+1)||(a==c-1))) \
			      || hadj(a, b, c, d) \
			      || hadj(c, d, a, b))

#define MAXEYE 20

static void originate_eye(int i, int j, int m, int n,
			  int *esize, int *msize,
			  struct eye_data eye[MAX_BOARD][MAX_BOARD]);
static int linear_eye_space(int i, int j, int *attacki, int *attackj,
			    int *max, int *min,
			    struct eye_data eye[MAX_BOARD][MAX_BOARD]);
static int recognize_eye(int i, int j, int *ki, int *kj, 
			 int *max, int *min,
			 struct eye_data eye[MAX_BOARD][MAX_BOARD]);
static int next_map(int *q, int map[MAXEYE], int esize);
static void print_eye(struct eye_data eye[MAX_BOARD][MAX_BOARD], int i, int j);

static void
clear_eye(struct eye_data *eye)
{
  eye->color = 0;
  eye->esize = 0;
  eye->msize = 0;
  eye->origini = -1;
  eye->originj = -1;
  eye->maxeye = 0;
  eye->mineye = 0;
  eye->attacki = -1;
  eye->attackj = -1;
  eye->dragoni = -1;
  eye->dragonj = -1;
  eye->marginal = 0;
  eye->type = 0;
  eye->neighbors = 0;
  eye->marginal_neighbors = 0;
  eye->cut = 0;
}

/*
 * make_domains() is called from make_dragons(). It marks the black
 * and white domains (eyeshape regions) and collects some statistics
 * about each one.
 */

void
make_domains(void)
{
  int i, j;
  int found_one;
  
  memset(black_domain, 0, sizeof(black_domain));
  memset(white_domain, 0, sizeof(white_domain));

  /* Initialize eye data. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      clear_eye(&(black_eye[i][j]));
      clear_eye(&(white_eye[i][j]));
    }

/* First we compute the domains of influence of each color. For this
 * algorithm the strings which are not lively are invisible. Ignoring
 * these, the algorithm assigns black influence to:
 *
 * (1) every vertex which is occupied by a (lively) black stone, 
 * (2) every empty vertex adjoining a (lively) black stone,
 * (3) every empty vertex for which two adjoining vertices (not
 *     on the first line) in the (usually 8) surrounding ones have black 
 *     influence, with another CAVEAT explained below.
 *
 * Thus in the following diagram, e would be assigned black influence
 * if a and b have black influence, or a and d. It is not sufficent
 * for b and d to have black influence, because they are not adjoining.
 * 
 *         abc
 *         def
 *         ghi
 * 
 * The constraint that the two adjoining vertices not lie on the first
 * line prevents influence from leaking under a stone on the third line.
 * 
 * The CAVEAT alluded to above is that even if a and b have black
 * influence, this does not cause e to have black influence if there
 * is a lively white stone at d. This constraint prevents influence
 * from leaking past knight's move extensions.
 */

  found_one=1;
  while (found_one) {
    found_one=0;
    for (i=0; i<board_size; i++)
      for (j=0; j<board_size; j++) {
	if (black_domain[i][j]
	    || ((p[i][j]!=EMPTY) && ((p[i][j]!=WHITE) || lively(i, j))))
	  continue;

	if ((p[i][j]==BLACK && lively(i, j))
	    || ((i>0)            && p[i-1][j]==BLACK && lively(i-1, j))
	    || ((i<board_size-1) && p[i+1][j]==BLACK && lively(i+1, j))
	    || ((j>0)            && p[i][j-1]==BLACK && lively(i, j-1))
	    || ((j<board_size-1) && p[i][j+1]==BLACK && lively(i, j+1)))
	  {
	    found_one=1;
	    black_domain[i][j]=1;
	  } else {
	    if ((    ((i>1) && black_inf(i-1, j))
		     && (((j>1) && black_inf(i-1, j-1)
			  && (p[i][j-1]!=WHITE || !lively(i, j-1)))
			 || ((j<board_size-2)
			     && black_inf(i-1, j+1)
			     && (p[i][j+1]!=WHITE || !lively(i, j+1)))))
		|| (((i<board_size-2) && black_inf(i+1, j))
		    && (((j>1) && black_inf(i+1, j-1)
			 && (p[i][j-1]!=WHITE || !lively(i, j-1)))
			|| ((j<board_size-2) && black_inf(i+1, j+1)
			    && (p[i][j+1]!=WHITE || !lively(i, j+1))))) 
		|| (((j>1) && black_inf(i, j-1)) 
		    && (((i>1) && black_inf(i-1, j-1) 
			 && (p[i-1][j]!=WHITE || !lively(i-1, j)))
			|| ((i<board_size-2) && black_inf(i+1, j-1)
			    && (p[i+1][j]!=WHITE || !lively(i+1, j))))) 
		|| (((j<board_size-2) && black_inf(i, j+1)) 
		    && (((i>1) && black_inf(i-1, j+1)
			 && (p[i-1][j]!=WHITE || !lively(i-1, j))) 
			|| ((i<board_size-2) && black_inf(i+1, j+1)
			    && (p[i+1][j]!=WHITE || !lively(i+1, j))))))
	      {
		found_one=1;
		black_domain[i][j]=1;
	      }
	  }
      }
  }

  found_one=1;
  while (found_one) {
    found_one=0;
    for (i=0; i<board_size; i++)
      for (j=0; j<board_size; j++) {
	if (((p[i][j]==EMPTY) || ((p[i][j]==BLACK) && !lively(i, j)))
	    && !white_domain[i][j]) 
	{
	  if ((p[i][j]==WHITE && lively(i, j)) 
	      || ((i>0) && p[i-1][j]==WHITE && lively(i-1, j)) 
	      || ((i<board_size-1) && p[i+1][j]==WHITE && lively(i+1, j))
	      || ((j>0) && p[i][j-1]==WHITE && lively(i, j-1))
	      || ((j<board_size-1) && p[i][j+1]==WHITE && lively(i, j+1)))
	  {
	    found_one=1;
	    white_domain[i][j]=1;
	  } else {
	    if ((((i>1) && white_inf(i-1, j)) 
		 && (((j>1) && white_inf(i-1, j-1)
		      && (p[i][j-1]!=BLACK || !lively(i, j-1))) 
		     || ((j<board_size-2) && white_inf(i-1, j+1)
			 && (p[i][j+1]!=BLACK || !lively(i, j+1))))) 
		|| (((i<board_size-2) && white_inf(i+1, j)) 
		    && (((j>1) && white_inf(i+1, j-1) 
			 && (p[i][j-1]!=BLACK || !lively(i, j-1))) 
			|| ((j<board_size-2) && white_inf(i+1, j+1)
			    && (p[i][j+1]!=BLACK || !lively(i, j+1))))) 
		|| (((j>1) && white_inf(i, j-1)) 
		    && (((i>1) && white_inf(i-1, j-1)
			 && (p[i-1][j]!=BLACK || !lively(i-1, j))) 
			|| ((i<board_size-2) && white_inf(i+1, j-1)
			    && (p[i+1][j]!=BLACK || !lively(i+1, j))))) 
		|| (((j<board_size-2) && white_inf(i, j+1)) 
		    && (((i>1) && white_inf(i-1, j+1)
			 && (p[i-1][j]!=BLACK || !lively(i-1, j))) 
			|| ((i<board_size-2) && white_inf(i+1, j+1)
			    && (p[i+1][j]!=BLACK || !lively(i+1, j))))))
	      {
		found_one=1;
		white_domain[i][j]=1;
	      }
	  }
	}
      }
  }

  /* 
   * Now we fill out the arrays black_eye and white_eye with data
   * describing each eye shape.
   */

  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((p[i][j]==EMPTY) || !lively(i, j)) {
	if ((black_domain[i][j] == 0) && (white_domain[i][j]==0)) {
	  white_eye[i][j].color=GRAY_BORDER;
	  black_eye[i][j].color=GRAY_BORDER;
	}
	else if ((black_domain[i][j] == 1) && (white_domain[i][j]==0)) {
	  black_eye[i][j].color=BLACK_BORDER;
	  black_eye[i][j].origini=-1;
	  black_eye[i][j].originj=-1;
	  if ((   (i>0)
		  && white_domain[i-1][j]
		  && (!black_domain[i-1][j]
		      || safe_move(i, j, WHITE)))
	      || ((i<board_size-1)
		  && white_domain[i+1][j]
		  && (!black_domain[i+1][j]
		      || safe_move(i, j, WHITE)))
	      || ((j>0)
		  && white_domain[i][j-1]
		  && (!black_domain[i][j-1]
		      || safe_move(i, j, WHITE)))
	      || ((j<board_size-1)
		  && white_domain[i][j+1]
		  && (!black_domain[i][j+1]
		      || safe_move(i, j, WHITE))))
	    black_eye[i][j].marginal=1;
	  else 
	    black_eye[i][j].marginal=0;
	}
	else if ((black_domain[i][j] == 0) && (white_domain[i][j]==1)) {
	  white_eye[i][j].color=WHITE_BORDER;
	  white_eye[i][j].origini=-1;
	  white_eye[i][j].originj=-1;
	  if ((   (i>0)
		  && black_domain[i-1][j]
		  && (!white_domain[i-1][j]
		      || safe_move(i, j, BLACK)))
	      || ((i<board_size-1)
		  && black_domain[i+1][j]
		  && (!white_domain[i+1][j]
		      || safe_move(i, j, BLACK)))
	      || ((j>0)
		  && black_domain[i][j-1]
		  && (!white_domain[i][j-1]
		      || safe_move(i, j, BLACK)))
	      || ((j<board_size-1)
		  && black_domain[i][j+1]
		  && (!white_domain[i][j+1]
		      || safe_move(i, j, BLACK))))
	    white_eye[i][j].marginal=1;
	  else
	    white_eye[i][j].marginal=0;
	}
	else if ((black_domain[i][j] == 1) && (white_domain[i][j]==1)) {
	  if (((i>0) && (black_domain[i-1][j]) && (!white_domain[i-1][j])) 
	      || ((i<board_size-1) && (black_domain[i+1][j])
		  && (!white_domain[i+1][j])) 
	      || ((j>0) && (black_domain[i][j-1])
		  && (!white_domain[i][j-1])) 
	      || ((j<board_size-1) && (black_domain[i][j+1])
		  && (!white_domain[i][j+1])))
	    {
	      black_eye[i][j].marginal=1;
	      black_eye[i][j].color=BLACK_BORDER;
	    }
	  else
	    black_eye[i][j].color=GRAY_BORDER;

	  if (((i>0) && (white_domain[i-1][j]) && (!black_domain[i-1][j]))
	      || ((i<board_size-1) && (white_domain[i+1][j]) 
		  && (!black_domain[i+1][j])) 
	      || ((j>0) && (white_domain[i][j-1])
		  && (!black_domain[i][j-1])) 
	      || ((j<board_size-1) && (white_domain[i][j+1])
		  && (!black_domain[i][j+1])))
	    {
	      white_eye[i][j].marginal=1;
	      white_eye[i][j].color=WHITE_BORDER;
	    }
	  else
	    white_eye[i][j].color=GRAY_BORDER;
	}
      }      
    }      

  /* Search connection database for cutting points, which may modify
   * the eyespace in order to avoid amalgamation and reflect the
   * weakness in the position.
   */
  find_cuts();
  
 /* The eye spaces are all found. Now we need to find the origins. */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((black_eye[i][j].origini == -1)
	  && (black_eye[i][j].color == BLACK_BORDER)) 
      {
	int esize=0;
	int msize=0;

	originate_eye(i, j, i, j, &esize, &msize, black_eye);
	black_eye[i][j].esize=esize;
	black_eye[i][j].msize=msize;
      }
    }

  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if ((white_eye[i][j].origini == -1)
	  && (white_eye[i][j].color == WHITE_BORDER)) 
      {
	int esize=0;
	int msize=0;

	originate_eye(i, j, i, j, &esize, &msize, white_eye);
	white_eye[i][j].esize=esize;
	white_eye[i][j].msize=msize;
      }
    }

  /* Now we calculate the number of neighbors and marginal neighbors
   * of each vertex.
   */
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++) {
      if (white_eye[i][j].origini != -1) {
	white_eye[i][j].esize = white_eye[white_eye[i][j].origini]
	                                 [white_eye[i][j].originj].esize;
	white_eye[i][j].msize = white_eye[white_eye[i][j].origini]
	                                 [white_eye[i][j].originj].msize;
	white_eye[i][j].neighbors=0;
	white_eye[i][j].marginal_neighbors=0;

	if ((i>0)
	    && (white_eye[i-1][j].origini==white_eye[i][j].origini)
	    && (white_eye[i-1][j].originj==white_eye[i][j].originj))
	{
	  white_eye[i][j].neighbors++;
	  if (white_eye[i-1][j].marginal)
	    white_eye[i][j].marginal_neighbors++;
	}
	if ((i<board_size-1) 
	    && (white_eye[i+1][j].origini==white_eye[i][j].origini)
	    && (white_eye[i+1][j].originj==white_eye[i][j].originj)) 
	{
	  white_eye[i][j].neighbors++;
	  if (white_eye[i+1][j].marginal)
	    white_eye[i][j].marginal_neighbors++;
	}
	if ((j>0) 
	    && (white_eye[i][j-1].origini==white_eye[i][j].origini)
	    && (white_eye[i][j-1].originj==white_eye[i][j].originj)) 
	{
	  white_eye[i][j].neighbors++;
	  if (white_eye[i][j-1].marginal)
	    white_eye[i][j].marginal_neighbors++;
	}
	if ((j<board_size-1) 
	    && (white_eye[i][j+1].origini==white_eye[i][j].origini)
	    && (white_eye[i][j+1].originj==white_eye[i][j].originj)) 
	{
	  white_eye[i][j].neighbors++;
	  if (white_eye[i][j+1].marginal)
	    white_eye[i][j].marginal_neighbors++;
	}
      }

      if (black_eye[i][j].origini != -1) {
	black_eye[i][j].esize = black_eye[black_eye[i][j].origini]
	                                 [black_eye[i][j].originj].esize;
	black_eye[i][j].msize = black_eye[black_eye[i][j].origini]
	                                 [black_eye[i][j].originj].msize;
	black_eye[i][j].neighbors=0;
	black_eye[i][j].marginal_neighbors=0;

	if ((i>0) 
	    && (black_eye[i-1][j].origini==black_eye[i][j].origini) 
	    && (black_eye[i-1][j].originj==black_eye[i][j].originj))
	{
	  black_eye[i][j].neighbors++;
	  if (black_eye[i-1][j].marginal)
	    black_eye[i][j].marginal_neighbors++;
	}
	if ((i<board_size-1) 
	    && (black_eye[i+1][j].origini==black_eye[i][j].origini) 
	    && (black_eye[i+1][j].originj==black_eye[i][j].originj)) 
	{
	  black_eye[i][j].neighbors++;
	  if (black_eye[i+1][j].marginal)
	    black_eye[i][j].marginal_neighbors++;
	}
	if ((j>0) 
	    && (black_eye[i][j-1].origini==black_eye[i][j].origini) 
	    && (black_eye[i][j-1].originj==black_eye[i][j].originj))
	{
	  black_eye[i][j].neighbors++;
	  if (black_eye[i][j-1].marginal)
	    black_eye[i][j].marginal_neighbors++;
	}
	if ((j<board_size-1) 
	    && (black_eye[i][j+1].origini==black_eye[i][j].origini)
	    && (black_eye[i][j+1].originj==black_eye[i][j].originj)) 
	{
	  black_eye[i][j].neighbors++;
	  if (black_eye[i][j+1].marginal)
	    black_eye[i][j].marginal_neighbors++;
	}
      }
    }
}


/*
 * originate_eye(i, j, m, n, *size) creates an eyeshape with origin (i, j)
 * the last variable returns the size. 
 */
static void
originate_eye(int i, int j, int m, int n,
	      int *esize, int *msize, 
	      struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  assert (m>=0);
  assert (m<board_size);
  assert (n>=0);
  assert (n<board_size);

  eye[m][n].origini=i;
  eye[m][n].originj=j;
  (*esize)++;
  if (eye[m][n].marginal)
    (*msize)++;
  if (eye[m][n].type & INHIBIT_CONNECTION)
    return;
  if ((m>0) 
      && (eye[m-1][n].color == eye[m][n].color)
      && (eye[m-1][n].origini == -1) 
      && (!eye[m-1][n].marginal || !eye[m][n].marginal))
    originate_eye(i, j, m-1, n, esize, msize, eye);

  if ((m<board_size-1) 
      && (eye[m+1][n].color == eye[m][n].color)
      && (eye[m+1][n].origini == -1) 
      && (!eye[m+1][n].marginal || !eye[m][n].marginal))
    originate_eye(i, j, m+1, n, esize, msize, eye);

  if ((n>0) 
      && (eye[m][n-1].color == eye[m][n].color)
      && (eye[m][n-1].origini == -1) 
      && (!eye[m][n-1].marginal || !eye[m][n].marginal))
    originate_eye(i, j, m, n-1, esize, msize, eye);

  if ((n<board_size-1) 
      && (eye[m][n+1].color == eye[m][n].color)
      && (eye[m][n+1].origini == -1) 
      && (!eye[m][n+1].marginal || !eye[m][n].marginal))
    originate_eye(i, j, m, n+1, esize, msize, eye);
}


/* Print debugging data for the eyeshape at (i,j). Useful with GDB. */

static void
print_eye(struct eye_data eye[MAX_BOARD][MAX_BOARD], int i, int j)
{
  int m,n;
  int mini, maxi;
  int minj, maxj;
  int origini = eye[i][j].origini;
  int originj = eye[i][j].originj;
  
  /* Determine the size of the eye. */
  mini = board_size;
  maxi = -1;
  minj = board_size;
  maxj = -1;
  for (m = 0; m < board_size; m++)
    for (n = 0; n < board_size; n++) {
      if (eye[m][n].origini != origini || eye[m][n].originj != originj)
	continue;

      if (m < mini) mini = m;
      if (m > maxi) maxi = m;
      if (n < minj) minj = n;
      if (n > maxj) maxj = n;
    }

  for (m = mini; m <= maxi; m++) {
    for (n = minj; n <= maxj; n++) {
      if (eye[m][n].origini == origini && eye[m][n].originj == originj) {
	if (p[m][n] == EMPTY) {
	  if (eye[m][n].marginal)
	    gprintf("!");
	  else
	    gprintf(".");
	} else
	  gprintf("X");
      } else
	gprintf(" ");
    }
    gprintf("\n");
  }
}


/* 
 * Given an eyespace with origin (i,j), this function computes the
 * minimum and maximum numbers of eyes the space can yield.
 */

void
compute_eyes(int i, int  j, int *max, int *min, int *attacki, int *attackj, 
	     struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int m, n;

  *attacki=-1;
  *attackj=-1;

  if (debug & DEBUG_EYES) {
    DEBUG(DEBUG_EYES, "Eyespace at %m: color=%d, esize=%d, msize=%d\n",
	  i, j, eye[i][j].color, eye[i][j].esize, eye[i][j].msize);

    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++) {
	if ((eye[m][n].origini == i) && (eye[m][n].originj == j)) 
	{
	  if (eye[m][n].marginal && p[m][n] != EMPTY)
	    DEBUG(DEBUG_EYES, "%m (X!)\n",m,n);
	  else if (eye[m][n].marginal && p[m][n] == EMPTY)
	    DEBUG(DEBUG_EYES, "%m (!)\n",m,n);
	  else if (!eye[m][n].marginal && p[m][n] != EMPTY)
	    DEBUG(DEBUG_EYES, "%m (X)\n",m,n);
	  else
	    DEBUG(DEBUG_EYES, "%m\n",m,n);
	}
      }
  }
  
  /* First we try to find the eye space by matching in the graphs database. */
  if (recognize_eye(i, j, attacki, attackj, max, min, eye))
    return;
  else if (eye[i][j].esize < 6) {
    /* made these printouts contingent on DEBUG_EYES /gf */
    if (debug & DEBUG_EYES) {
      gprintf("===========================================================\n");
      gprintf("Unrecognized eye of size %d shape at %m\n", 
	      eye[i][j].esize, i, j);
      print_eye(eye, i, j);
    }
  }
  
  /* If not found we examine whether we have a linear eye space. */
  if (linear_eye_space(i, j, attacki, attackj, max, min, eye)) {
    if (debug & DEBUG_EYES)
      gprintf("Linear eye shape at %m\n", i, j);
    return;
  }

  /* Ideally any eye space that hasn't been matched yet should be two
   * secure eyes. Until the database becomes more complete we have
   * some additional heuristics to guess the values of unknown
   * eyespaces.
   */
  if (eye[i][j].esize-2*eye[i][j].msize>3) {
    *min = 2;
    *max = 2;
  }
  else if (eye[i][j].esize-2*eye[i][j].msize>0) {
    *min = 1;
    *max = 1;
  }
  else {
    *min = 0;
    *max = 0;
  }
}


/* 
 * propagate_eye(i, j) copies the data at the origin (i, j) to the
 * rest of the eye (certain fields only).
 */

void
propagate_eye (int i, int j, struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int m, n;

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((eye[m][n].origini==i) && (eye[m][n].originj==j)) {
	eye[m][n].color   = eye[i][j].color;
	eye[m][n].esize   = eye[i][j].esize;
	eye[m][n].msize   = eye[i][j].msize;
	eye[m][n].origini = eye[i][j].origini;
	eye[m][n].originj = eye[i][j].originj;
	eye[m][n].maxeye  = eye[i][j].maxeye;
	eye[m][n].mineye  = eye[i][j].mineye;
	eye[m][n].attacki = eye[i][j].attacki;
	eye[m][n].attackj = eye[i][j].attackj;
	eye[m][n].dragoni = eye[i][j].dragoni;
	eye[m][n].dragonj = eye[i][j].dragonj;
      }
    }
}


/*
 * A linear eyespace is one in which each vertex has 2 neighbors, 
 * except for two vertices on the end. Only the end vertices can
 * be marginal.
 */
static int
linear_eye_space (int i, int j, int *attacki, int *attackj, int *max, int *min,
		  struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int m, n;
  int end1i=-1, end1j=-1;
  int end2i=-1, end2j=-1;
  int centers=0;
  int centeri=-1, centerj=-1;
  int middlei=-1, middlej=-1;
  int is_line=1;
  int msize=eye[i][j].msize;
  int esize=eye[i][j].esize;

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if ((eye[m][n].origini == i) && (eye[m][n].originj == j)) 
      {
	if (eye[m][n].neighbors>2) {
	  if (centeri == -1) {
	    centeri=m;
	    centerj=n;
	  }
	  centers++;
	  is_line=0;
	}
	if (eye[m][n].neighbors == 2) {
	  middlei=m;
	  middlej=n;
	  if (eye[m][n].marginal)
	    is_line=0;
	}
	if (eye[m][n].neighbors == 1) {
	  if (end1i == -1) {
	    end1i=m;
	    end1j=n;
	  }
	  else if (end2i == -1) {
	    end2i=m;
	    end2j=n;
	  }
	}
      }
    }
  
  if (!is_line)
    return 0;

  /* Come here --> Indeed a linear eye space. 
   * Now check how many eyes we can get. 
   */

  /* 1. Check eye spaces with no marginal vertices. */
  if (msize==0) {
    if ((esize==1) || (esize==2)) {
      *max=1;
      *min=1;
      return 1;
    }
    if (esize == 3) {
      assert (middlei != -1);
      if (p[middlei][middlej]==EMPTY) {
	*max=2;
	*min=1;
	*attacki=middlei;
	*attackj=middlej;
	return 1;
      }
      else {
	*max=1;
	*min=1;
	return 1;
      }
    }
    if (esize == 4) {
      int farmiddlei;
      int farmiddlej;
      if ((middlei>0) 
	  && (eye[middlei-1][middlej].origini==i)
	  && (eye[middlei-1][middlej].originj==j)
	  && (eye[middlei-1][middlej].neighbors==2)) 
      {
	farmiddlei=middlei-1;
	farmiddlej=middlej;
      }
      else if ((middlei<board_size-1) 
	       && (eye[middlei+1][middlej].origini==i)
	       && (eye[middlei+1][middlej].originj==j)
	       && (eye[middlei+1][middlej].neighbors==2)) 
      {
	farmiddlei=middlei+1;
	farmiddlej=middlej;
      }
      else if ((middlej>0) 
	       && (eye[middlei][middlej-1].origini==i)
	       && (eye[middlei][middlej-1].originj==j)
	       && (eye[middlei][middlej-1].neighbors==2)) 
      {
	farmiddlei=middlei;
	farmiddlej=middlej-1;
      }
      else if ((middlej<board_size-1) 
	       && (eye[middlei][middlej+1].origini==i)
	       && (eye[middlei][middlej+1].originj==j)
	       && (eye[middlei][middlej+1].neighbors==2)) 
      {
	farmiddlei=middlei;
	farmiddlej=middlej+1;
      }
      else {
	farmiddlei=-1; /* to prevent compiler warning */
	farmiddlej=-1;
	abort();
      }

      if (p[middlei][middlej]==EMPTY) {
	if (p[farmiddlei][farmiddlej]==EMPTY) {
	  *max=2;
	  *min=2;
	  return 1;
	}
	else {
	  *max=2;
	  *min=1;
	  *attacki=middlei;
	  *attackj=middlej;
	  return 1;
	}
      }
      else
	if (p[farmiddlei][farmiddlej]==EMPTY) {
	  *max=2;
	  *min=1;
	  *attacki=farmiddlei;
	  *attackj=farmiddlej;
	  return 1;
	}
	else {
	  *max=1;
	  *min=1;
	  return 1;
	}
    }
    if (esize > 4) {
      *max=2;
      *min=2;
      return 1;
    }
  }
  /* 2. Check eye spaces with one marginal vertice. */
  if (msize == 1) {
    if (esize == 1) {
      *max=0;
      *min=0;
      return 1;
    }
    if (esize == 2) {
      *max=1;
      *min=0;
      if (eye[end1i][end1j].marginal) {
	*attacki=end1i;
	*attackj=end1j;
      } else {
	*attacki=end2i;
	*attackj=end2j;
      }
      
      /* We need to make an exception for cases like this:
       * XXOOO
       * .OX.O
       * -----
       */
      if (p[*attacki][*attackj] != EMPTY) {
	*max = 0;
	*attacki = -1;
	*attackj = -1;
      }
      return 1;
    }

    if (esize == 3) {
      if (p[middlei][middlej]==EMPTY) {
	*max=1;
	*min=1;
	return 1;
      }
      else {
	*max=1;
	*min=0;
	if (eye[end1i][end1j].marginal) {
	  *attacki=end1i;
	  *attackj=end1j;
	} else {
	  *attacki=end2i;
	  *attackj=end2j;
	}
	return 1;
      }
    }

    if (esize == 4) {
      if (p[middlei][middlej]==EMPTY) {
	*max=1;
	*min=1;
	return 1;
      }
      else {
	int farmiddlei;
	int farmiddlej;
	if ((middlei>0) 
	    && (eye[middlei-1][middlej].origini==i)
	    && (eye[middlei-1][middlej].originj==j)
	    && (eye[middlei-1][middlej].neighbors==2)) 
	{
	  farmiddlei=middlei-1;
	  farmiddlej=middlej;
	}
	else if ((middlei<board_size-1) 
		 && (eye[middlei+1][middlej].origini==i) 
		 && (eye[middlei+1][middlej].originj==j)
		 && (eye[middlei+1][middlej].neighbors==2)) 
	{
	  farmiddlei=middlei+1;
	  farmiddlej=middlej;
	}
	else if ((middlej>0)
		 && (eye[middlei][middlej-1].origini==i)
		 && (eye[middlei][middlej-1].originj==j)
		 && (eye[middlei][middlej-1].neighbors==2)) 
	{
	  farmiddlei=middlei;
	  farmiddlej=middlej-1;
	}
	else if ((middlej<board_size-1) 
		 && (eye[middlei][middlej+1].origini==i)
		 && (eye[middlei][middlej+1].originj==j)
		 && (eye[middlei][middlej+1].neighbors==2))
	{
	  farmiddlei=middlei;
	  farmiddlej=middlej+1;
	}
	else {
	  farmiddlei=-1; /* to prevent compiler warning */
	  farmiddlej=-1;
	  abort();
	}

	if (p[farmiddlei][farmiddlej]==EMPTY) {
	  *max=1;
	  *min=1;
	  return 1;
	}
	else {
	  *max=1;
	  *min=0;
	  if (eye[end1i][end1j].marginal) {
	    *attacki=end1i;
	    *attackj=end1j;
	  }
	  else {
	    *attacki=end2i;
	    *attackj=end2j;
	  }
	  return 1;
	}
      }
    }

    if (esize == 5) {
      *max=2;
      *min=1;
      if (eye[end1i][end1j].marginal) {
	*attacki=end1i;
	*attackj=end1j;
      }
      else {
	*attacki=end2i;
	*attackj=end2j;
      }
      return 1;
    }
    if (esize == 6) {
      *max=2;
      *min=2;
      return 1;
    }
  }
  
  if (msize == 2) {
    if (esize < 4) {
      *max=0;
      *min=0;
      return 1;
    }
    if (esize == 4) {
      *max=1;
      *min=0;
      *attacki=end1i;
      *attackj=end1j;
      return 1;
    }
    if ((esize == 5) || (esize==6)) {
      *max=1;
      *min=1;
      return 1;
    }
    if (esize == 7) {
      *max=2;
      *min=1;
      *attacki=end1i;
      *attackj=end1j;
      return 1;
    }
    if (esize > 7) {
      *max=2;
      *min=2;
      return 1;
    }
  }

  return 0;
}

/* recognize eye (i, j, *ki, *kj), where (i,j) is the origin of
 * an eyespace, returns 1 if there is a pattern in eyes.c matching 
 * the eyespace, or 0 if no match is found. If there is a key point,
 * (*ki, *kj) are set to its location, or (-1,-1) if there is none.
 *
 */

static int
recognize_eye (int i, int j, int *ki, int *kj,
	       int *max, int *min, 
	       struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int m, n;
  int l=0;
  int vi[MAXEYE], vj[MAXEYE], marginal[MAXEYE], neighbors[MAXEYE];
  int graph;
  int q, r;
  int map[MAXEYE];
  int ok, contin;

  if (eye[i][j].esize-eye[i][j].msize > 7)
    return 0;

  if (eye[i][j].msize>MAXEYE)
    return 0;
  
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if (   (eye[m][n].origini==i) 
	  && (eye[m][n].originj==j)) 
      {
	vi[l]=m;
	vj[l]=n;
	marginal[l]=eye[m][n].marginal;
	neighbors[l]=eye[m][n].neighbors;
	if (0) {
	  if (marginal[l])
	    TRACE("(%m)", vi[l], vj[l]);
	  else
	    TRACE(" %m ", vi[l], vj[l]);
	  TRACE("\n");
	}
	l++;
      }
    }

  /* We attempt to construct a map from the graph to the eyespace
   * preserving the adjacency structure. If this can be done, we've
   * identified the eyeshape.
   */

  for (graph=0; graph < GRAPHS; graph ++) {
    if ((graphs[graph].esize != eye[i][j].esize) 
	|| (graphs[graph].msize != eye[i][j].msize)) 
      continue;

    for (r=0; r<eye[i][j].esize; r++)
      map[r] = 0;

    q=0;
    contin=1;
    while (contin && (q>=0) && (q<eye[i][j].esize)) {
      ok=1;

      if (0)
	TRACE("q=%d: %d %d %d %d %d %d\n", 
	      q, map[0], map[1], map[2], map[3], map[4], map[5]);

      if (eye[vi[map[q]]][vj[map[q]]].neighbors 
	  != graphs[graph].vertex[q].neighbors)
	ok=0;

      if (ok && eye[vi[map[q]]][vj[map[q]]].marginal 
	  && (graphs[graph].vertex[q].type != '!')
	  && (graphs[graph].vertex[q].type != '@'))
	ok=0;
      if (ok && !eye[vi[map[q]]][vj[map[q]]].marginal
	  && ((graphs[graph].vertex[q].type == '!') ||
	      (graphs[graph].vertex[q].type == '@')))
	ok=0;
      if (ok && (p[vi[map[q]]][vj[map[q]]] != EMPTY) 
	  && (graphs[graph].vertex[q].type != 'X')
	  && (graphs[graph].vertex[q].type != 'x'))
	ok=0;
      if (ok && (p[vi[map[q]]][vj[map[q]]]==EMPTY) 
	  && (graphs[graph].vertex[q].type == 'X'))
	ok=0;
      if (ok && graphs[graph].vertex[q].n1<q
	  && (graphs[graph].vertex[q].n1 != -1)) 
	{
	  if (!adjacent(vi[map[q]], vj[map[q]], 
			vi[map[graphs[graph].vertex[q].n1]],
			vj[map[graphs[graph].vertex[q].n1]]))
	    ok=0;
	}
      if (ok && graphs[graph].vertex[q].n2<q
	  && (graphs[graph].vertex[q].n2 != -1)) 
	{
	  if (!adjacent(vi[map[q]], vj[map[q]], 
			vi[map[graphs[graph].vertex[q].n2]],
			vj[map[graphs[graph].vertex[q].n2]]))
	    ok=0;
	}
      if (ok && graphs[graph].vertex[q].n3<q
	  && (graphs[graph].vertex[q].n3 != -1)) 
	{
	  if (!adjacent(vi[map[q]], vj[map[q]], 
			vi[map[graphs[graph].vertex[q].n3]],
			vj[map[graphs[graph].vertex[q].n3]]))
	    ok=0;
	}
      if (ok && graphs[graph].vertex[q].n4<q
	  && (graphs[graph].vertex[q].n4 != -1)) 
	{
	  if (!adjacent(vi[map[q]], vj[map[q]], 
			vi[map[graphs[graph].vertex[q].n4]],
			vj[map[graphs[graph].vertex[q].n4]]))
	    ok=0;
	}

      if (!ok) {
	contin=next_map(&q, map, eye[i][j].esize);
	if (0)
	  gprintf("  q=%d, esize=%d: %d %d %d %d %d\n",
		  q, eye[i][j].esize, 
		  map[0], map[1], map[2], map[3], map[4]);
      }
      else 
	q++;
    }

    if (q==eye[i][j].esize) {
      *max = graphs[graph].max;
      *min = graphs[graph].min;
      if (*max != *min) {
	assert(graphs[graph].vital >= 0);
	*ki = vi[map[graphs[graph].vital]];
	*kj = vj[map[graphs[graph].vital]];
      }
      TRACE("eye space at %m of type %d\n", i, j, graphs[graph].id);

      return 1;
    }
  }

  return 0;
}


/* a MAP is a map of the integers 0,1,2, ... ,q into 
 * 0,1, ... , esize-1 where q < esize. This determines a 
 * bijection of the first q+1 elements of the graph into the 
 * eyespace. The function next_map produces the next map when
 * these are ordered lexicographically. If no next map can
 * be found, q is decremented, then we try again. If q==0
 * and no next map can be found, the function returns false.
 */

static int
next_map(int *q, int map[MAXEYE], int esize)
{
  int mapok=0;
  int r;

  if (0)
    gprintf("  q=%d, esize=%d: %d %d %d %d %d\n",
	    *q, esize, map[0], map[1], map[2], map[3], map[4]);

  if (((*q)==0) && (map[*q]==esize-1))
    return 0;

  map[*q]++;
  while (!mapok) {
    mapok=1;
    for (r=0; r<*q; r++) {
      if (map[r]==map[*q]) {
	map[*q]++;
	mapok=0;
      }
    }
  }

  if (map[*q]>=esize) {
    map[*q]=0;
    (*q)--;
    return next_map(q, map, esize);
  }
  else
    return 1;
}     


/* add_half_eye adds a half eye to an eye shape. */

void
add_half_eye(int m, int n, struct eye_data eye[MAX_BOARD][MAX_BOARD])
{
  int ei, ej, ki, kj;

  if (half_eye[m][n].type)
    DEBUG(DEBUG_EYES, "half or false eye found at %m\n", m, n);

  if (half_eye[m][n].type == FALSE_EYE) {
    DEBUG(DEBUG_EYES, "false eye at %m for dragon at %m\n",
	  m, n, eye[m][n].dragoni, eye[m][n].dragonj);
    if (eye[m][n].color != GRAY_BORDER) {
      if (eye[m][n].marginal==0) {
	eye[m][n].marginal=1;
	(eye[eye[m][n].origini][eye[m][n].originj].msize)++;
	if ((m>0) 
	    && (eye[m-1][n].origini==eye[m][n].origini) 
	    && (eye[m-1][n].originj==eye[m][n].originj))
	  eye[m-1][n].marginal_neighbors++;
	if ((m<board_size-1) 
	    && (eye[m+1][n].origini==eye[m][n].origini) 
	    && (eye[m+1][n].originj==eye[m][n].originj))
	  eye[m+1][n].marginal_neighbors++;
	if ((n>0)
	    && (eye[m][n-1].origini==eye[m][n].origini) 
	    && (eye[m][n-1].originj==eye[m][n].originj))
	  eye[m][n-1].marginal_neighbors++;
	if ((n<board_size-1)
	    && (eye[m][n+1].origini==eye[m][n].origini) 
	    && (eye[m][n+1].originj==eye[m][n].originj))
	  eye[m][n+1].marginal_neighbors++;
	propagate_eye(eye[m][n].origini, eye[m][n].originj, eye);
      }
    }
  }
  else if (half_eye[m][n].type == HALF_EYE) {
    ki=half_eye[m][n].ki;
    kj=half_eye[m][n].kj;
    ei=eye[m][n].origini;
    ej=eye[m][n].originj;
    
    if (eye[ki][kj].origini == -1) {
      eye[ei][ej].esize++;
      eye[ei][ej].msize++;
      eye[m][n].neighbors++;
      eye[m][n].marginal_neighbors++;
      eye[ki][kj].origini=ei;
      eye[ki][kj].originj=ej;
      eye[ki][kj].marginal=1;
      eye[ki][kj].neighbors=1;
      if (eye[m][n].marginal)
	eye[ki][kj].marginal_neighbors=1;
      else
	eye[ki][kj].marginal_neighbors=0;
      propagate_eye(ei, ej, eye);
    }
    else if ((eye[ki][kj].origini != ei) || (eye[ki][kj].originj != ej)) {
      int fi=eye[ki][kj].origini;
      int fj=eye[ki][kj].originj;
      int i, j;
	  
      if (!eye[ki][kj].marginal) {
	eye[ki][kj].marginal=1;
	eye[fi][fj].msize++;
      }
      eye[ei][ej].esize+=eye[fi][fj].esize;
      eye[ei][ej].msize+=eye[fi][fj].msize;
      eye[m][n].neighbors++;      
      eye[ki][kj].neighbors++;
      if (eye[m][n].marginal)
	eye[ki][kj].marginal_neighbors++;
      if (eye[ki][kj].marginal)
	eye[m][n].marginal_neighbors++;
      for (i=0; i<board_size; i++)
	for (j=0; j<board_size; j++) {
	  if ((eye[i][j].origini==fi) && (eye[i][j].originj==fj)) {
	    eye[i][j].origini=ei;
	    eye[i][j].originj=ej;
	  }
	}
      propagate_eye(ei, ej, eye);
    }
  }
}


/* retrofit_half_eye(i, j, di, dj) is intended to add a half eye which
 * is missed during make_dragons. These half eyes are found during shapes
 * by helpers to the LD patterns. This function increments dragon[di][dj].heyes
 * and sets dragon[di][dj].heyei, heyj to (i, j). 
 */
    
void 
retrofit_half_eye(int i, int j, int di, int dj)
{
  int m, n, status;

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if ((dragon[m][n].origini==dragon[di][dj].origini)
	  && (dragon[m][n].originj==dragon[di][dj].originj)) {
	dragon[m][n].heyes++;
	dragon[m][n].heyei=i;
	dragon[m][n].heyej=j;
      }
  status=dragon_status(di, dj);
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if ((dragon[m][n].origini==dragon[di][dj].origini)
	  && (dragon[m][n].originj==dragon[di][dj].originj))
	dragon[m][n].status=status;
}


/* retrofit_genus(i, j, step) adds or subtracts a full eye which is missed
 * during make_dragons. To add an eye, take step=1. To delete an eye, take
 * step=-1.
 */
    
void 
retrofit_genus(int di, int dj, int step)
{
  int m, n, status;

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if ((dragon[m][n].origini==dragon[di][dj].origini)
	  && (dragon[m][n].originj==dragon[di][dj].originj))
	dragon[m][n].genus += step;
  status=dragon_status(di, dj);
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if ((dragon[m][n].origini==dragon[di][dj].origini)
	  && (dragon[m][n].originj==dragon[di][dj].originj))
	dragon[m][n].status=status;
}



/* These functions are used from constraints to identify eye spaces,
 * primarily for late endgame moves.
 */

int
eye_space(int i, int j)
{
  return ((white_eye[i][j].color == WHITE_BORDER)
	  || (black_eye[i][j].color == BLACK_BORDER));
}

int
proper_eye_space(int i, int j)
{
  return ((   (white_eye[i][j].color == WHITE_BORDER)
	   && !white_eye[i][j].marginal)
	  || ((black_eye[i][j].color == BLACK_BORDER)
	      && !black_eye[i][j].marginal));
}

int
marginal_eye_space(int i, int j)
{
  return (white_eye[i][j].marginal || black_eye[i][j].marginal);
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
