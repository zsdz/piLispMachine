/*
                 GNUGO - the game of Go (Wei-Chi)
                Version 1.2   last revised 10-31-95
           Copyright (C) Free Software Foundation, Inc.
                      written by Man L. Li
                      modified by Wayne Iba
        modified by Frank Pursel <fpp%minor.UUCP@dragon.com>
                    documented by Bob Webber
*/
/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation - version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License in file COPYING for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Please report any bug/fix, modification, suggestion to

           manli@cs.uh.edu
*/
/*------------------------------
  main.c -- gnugo main program
------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gnugo.h"

unsigned char p[19][19];  /* go board */
unsigned char l[19][19];  /* liberty of current color */
unsigned char ma[19][19]; /* working matrix for marking */
unsigned char ml[19][19]; /* working matrix for marking */
int mymove, umove;        /* computer color, opponent color */
int lib;                  /* current stone liberty */
int play;                 /* game state */
int pass;                 /* pass indicator */
int mik, mjk;             /* location of computer stone captured */
int uik, ujk;             /* location of opponent stone captured */
int mk, uk;               /* no. of stones captured by computer and oppoent */
int opn[9];               /* opening pattern flag */

// int main(int argc,char *argv[])
void gnugo1()
{
  FILE *fp;
  int i, j;
  char move[10], ans[5];
  int cont = 0;
  time_t tm;

  /* show instruction */
  //showinst();

  if ((fp = fopen("gnugo.dat", "r")) != NULL) /* continue old game */
  {
    cont = 1;

    /* read board configuration */
    for (i = 0; i < 19; i++)
      for (j = 0; j < 19; j++)
        fscanf(fp, "%c", &p[i][j]);

    /* read my color, pieces captured */
    fscanf(fp, "%d %d %d ", &mymove, &mk, &uk);
    /* read opening pattern flags */
    for (i = 0; i < 9; i++)
      fscanf(fp, "%d ", &opn[i]);

    fclose(fp);
    umove = 3 - mymove;

    /* delete file */
    remove("gnugo.dat");
  }
  else
  {
    /* init opening pattern numbers to search */
    for (i = 0; i < 9; i++)
      opn[i] = 1;
    opn[4] = 0;

    /* init board */
    for (i = 0; i < 19; i++)
      for (j = 0; j < 19; j++)
        p[i][j] = EMPTY;
    /* init global variables */
    mk = 0;
    uk = 0;
  }

  /* init global variables */
  play = 1;
  pass = 0;
  mik = -1;
  mjk = -1;
  uik = -1;
  ujk = -1;
  srand((unsigned)time(&tm)); /* start random number seed */

  if (!cont) /* new game */
  {
    /* ask for handicap */
    printf("Number of handicap for black (0 to 17)? ");
    scanf("%d", &i);
    getchar();
    sethand(i);

    /* display game board */
    showboard();

    /* choose color */
    printf("\nChoose side(b or w)? ");
    scanf("%c", ans);
    if (ans[0] == 'b')
    {
      mymove = 1; /* computer white */
      umove = 2;  /* human black */
      if (i)
      {
        genmove(&i, &j); /* computer move */
        p[i][j] = mymove;
      }
    }
    else
    {
      mymove = 2; /* computer black */
      umove = 1;  /* human white */
      if (i == 0)
      {
        genmove(&i, &j); /* computer move */
        p[i][j] = mymove;
      }
    }
  }

  showboard();

  /* main loop */
  while (play > 0)
  //while (1)
  {
    printf("your move? ");
    scanf("%s", move);
    if(strcmp(move,"exit\n"))
     break;
    getmove(move, &i, &j); /* read human move */
    if (play > 0)
    {
      if (i >= 0) /* not pass */
      {
        p[i][j] = umove;
        examboard(mymove); /* remove my dead pieces */
      }
      if (pass != 2)
      {
        genmove(&i, &j); /* computer move */
        if (i >= 0)      /* not pass */
        {
          p[i][j] = mymove;
          examboard(umove); /* remove your dead pieces */
        }
      }
      showboard();
    }
    if (pass == 2)
      play = 0; /* both pass then stop game */
  }

  if (play == 0)
  {
    /* finish game and count pieces */
    getchar();
    printf("Do you want to count score (y or n)? ");
    scanf("%c", ans);
    if (ans[0] == 'y')
      endgame();
  }

  //return 0;
} /* end main */
