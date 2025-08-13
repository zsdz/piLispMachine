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
/*----------------------------------------------------------
  showinst.c -- Show instructions at the beginning of game
----------------------------------------------------------*/

#include <stdio.h>
#include "gnugo.h"

void showinst(void)
/* show program instructions */
{
 printf("XOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOX");
 printf("OXOXOXOXOXOX\n");
 printf("O                                                                  ");
 printf("           O\n");
 printf("X                           GNUGO (Previously Hugo)                ");
 printf("           X\n");
 printf("O                           the game of Go (Wei-Chi)               ");
 printf("           O\n");
 printf("X                                                                  ");
 printf("           X\n");
 printf("O                            version 1.2   10-31-95                ");
 printf("           O\n");
 printf("X           Copyright (C) 1989, 1995 Free Software Foundation, Inc.");
 printf("           X\n");
 printf("O                              Author: Man L. Li                   ");
 printf("           O\n");
 printf("X           GNUGO comes with ABSOLUTELY NO WARRANTY; see COPYING fo");
 printf("r          X\n");
 printf("O           detail.   This is free software, and you are welcome to");
 printf("           O\n");
 printf("X           redistribute it; see COPYING for copying conditions.   ");
 printf("           X\n");
 printf("O                                                                  ");
 printf("           O\n");
 printf("X              Please report all bugs, modifications, suggestions  ");
 printf("           X\n");
 printf("O                             to manli@cs.uh.edu                   ");
 printf("           O\n");
 printf("X                                                                  ");
 printf("           X\n");
 printf("OXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXOXO");
 printf("XOXOXOXOXOXO\n");
 printf("\n\n\n\n\n\n\n\nPress return to continue");
 getchar();
 printf("\n\nTo play this game first select number of handicap pieces (0 to");
 printf(" 17) for the\nblack side.  Next choose your color (black or white).");
 printf("  To place your piece,\nenter your move as coordinate on the board");
 printf(" in column and row.  The column\nis from 'A' to 'T'(excluding 'I').");
 printf("  The row is from 1 to 19.\n\nTo pass your move enter 'pass' for");
 printf(" your turn.  After both you and the computer\npassed the game will");
 printf(" end.  To save the board and exit enter 'save'.  The game\nwill");
 printf(" continue the next time you start the program.  To stop the game in");
 printf(" the\nmiddle of play enter 'stop' for your move.  You will be");
 printf(" asked whether you want\nto count the result of the game.  If you");
 printf(" answer 'y' then you need to remove the\nremaining dead pieces and");
 printf(" fill up neutral turf on the board as instructed.\nFinally, the");
 printf(" computer will count all pieces for both side and show the result.\n\n");
}  /* end showinst */

