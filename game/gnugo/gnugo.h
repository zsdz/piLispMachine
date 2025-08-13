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
  gnugo.h -- gnugo common header
------------------------------*/

/* definitions */

#define EMPTY 0
#define WHITE 1
#define BLACK 2

/* public functions */

extern void count(int i,
                  int j,
                  int color);

extern void countlib(int m,
                     int n,
                     int color);

extern void endgame(void);

extern void eval(int color);

extern void examboard(int color);

extern int getij(char move[],
                 int *i,
                 int *j);

extern unsigned int findcolor(int i,
                              int j);

extern int findnextmove(int m,
                        int n,
                        int *i,
                        int *j,
                        int *val,
                        int minlib);

extern int findopen(int m,
                    int n,
                    int i[],
                    int j[],
                    int color,
                    int minlib,
                    int *ct);

extern int findpatn(int *i,
                    int *j,
                    int *val);

extern int findsaver(int *i,
                     int *j,
                     int *val);

extern int findwinner(int *i,
                      int *j,
                      int *val);

extern int fioe(int i,
                int j);

extern void genmove(int *i,
                    int *j);

extern void getmove(char move[],
                    int *i,
                    int *j);

extern void initmark(void);

extern int matchpat(int m,
                    int n,
                    int *i,
                    int *j,
                    int *val);

extern int opening(int *i,
                   int *j,
                   int *cnd,
                   int type);

extern int openregion(int i1,
                      int j1,
                      int i2,
                      int j2);

extern void showboard(void);

extern void showinst(void);

extern void sethand(int i);

extern int suicide(int i,
                   int j);
