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
#include <signal.h>

#include <stdlib.h>

#include "interface.h"
#include "../engine/liberty.h"
#include "gmp.h"
#include "../sgf/sgf.h"
#include "../sgf/ttsgf.h"
#include "../sgf/ttsgf_write.h"
#include "../sgf/sgfana.h"

/* --------------------------------------------------------------*/
/* play a game agaist a go-modem-protocol (GMP) client */
/* --------------------------------------------------------------*/
void play_gmp()
{
   Gmp  *ge;
   GmpResult  message;
   const char  *error;

   int i, j;
   int moval;
   int pass=0;  /* two passes and its over */
   int who;  /* who's turn is next ? */

   int mymove;  /* who has which color */
   /* if we are trying to record the game to a file,
    * trap cgoban's rather rude attempt to kill us,
    * and schedule a clean shutdown
    */

   int bsize, umove, handicap, seed;
   float komi,score;
   SGFNodeP curnode=0; /*current SGFNode*/
   int white_points,black_points;
   
   bsize=get_boardsize();
   komi=get_komi();
   seed=get_seed();
   handicap=get_handicap();
   mymove=get_computer_player();
   umove=OTHER_COLOR(mymove);

   ge = gmp_create(0, 1);
   TRACE("board size=%d\n",bsize);

   curnode=sgf_root;
   
   /* leave all the -1's so the client can negotiate the game parameters */
   gmp_startGame(ge, -1, -1, 5.5, 0, -1);
   
   /*
     if(umove==WHITE) gmp_startGame(ge, bsize, handicap, komi, 0, 1);
     else if(umove==BLACK) gmp_startGame(ge, bsize, handicap, komi, 0, 0);
     else gmp_startGame(ge, bsize, handicap, komi, 0, -1);
     */

   do  {
     message = gmp_check(ge, 1, NULL, NULL, &error);
   } while (!time_to_die && 
	    ((message == gmp_nothing) || 
	     (message == gmp_reset)));
   
   if (message == gmp_err)  {
     fprintf(stderr, "gnugo-gmp: Error \"%s\" occurred.\n", error);
     exit(1);
   } else if (message != gmp_newGame)  {
     fprintf(stderr, "gnugo-gmp: Expecting a newGame, got %s\n",
	     gmp_resultString(message));
     exit(1);
   }

   handicap=gmp_handicap(ge);
   bsize=gmp_size(ge);
   /*komi=gmp_komi(ge);
    This should work, but doesn't: problem is in gmp.c
    query_komi is not defined in go modem protocol ?!*/
   if (handicap>1)
     komi=0.5;

   set_boardsize(bsize);
   sgfOverwritePropertyInt(sgf_root,"SZ",bsize);

   set_komi(komi);
   
   TRACE("size=%d, handicap=%d\n, komi=%d",bsize, handicap,komi);

   sgf_write_game_info(bsize, handicap, komi, seed, "gmp");

   set_handicap(handicap=sethand(handicap));

   if (handicap)
     who = WHITE;
   else
     who = BLACK;

   if (gmp_iAmWhite(ge))
   {
     mymove = WHITE;   /* computer white */
     umove = BLACK;   /* human black */
   }
   else
   {
     mymove = BLACK;
     umove = WHITE;
   }


/* main GMP loop */
   while (pass < 2 && !time_to_die) {

     if (who == umove) {
       moval=0;
       /* get opponents move from gmp client */
       message = gmp_check(ge, 1, &j, &i, &error);

       if (message == gmp_err) {
           fprintf(stderr, "GNU Go: Sorry, error from gmp client\n");
	 sgf_close_file();
	 return;
       }
       if (message==gmp_undo)
       {
           int movenumber=get_movenumber();
           curnode=sgf_root;
           movenumber-=j-1;
           if(movenumber<0)
           {
               fprintf(stderr,"GNU Go: %s UNDO: already at the beginning of game tree\n",__FUNCTION__);
               continue;
           }
           who=sgfPlayTree(&curnode,&movenumber);
           set_movenumber(movenumber-1);
           continue;
       }
       if (message==gmp_pass) {
	 ++pass;
         curnode=sgfAddPlay(curnode,who,board_size, board_size);
	 updateboard(board_size, board_size, umove);
	 sgf_move_made(19, 19, who, moval);
	 inc_movenumber();
       } else {
	 /* not pass */
	 pass=0;
	 inc_movenumber();
         curnode=sgfAddPlay(curnode, who,i,j);
	 TRACE("\nyour move: %m\n\n", i, j);
	 updateboard(i, j, umove);
	 sgf_move_made(i, j, who, moval);
       }

     } else {
       /* generate my next move */

       moval=genmove(&i, &j, mymove);  
       inc_movenumber();
       updateboard(i, j, mymove);

       if (moval < 0) {
	 /* pass */
         curnode=sgfAddPlay(curnode,who,get_boardsize(), get_boardsize());
	 gmp_sendPass(ge);
	 sgf_move_made(i, j, who, moval);
	 ++pass;
       } else {

	 /* not pass */
         curnode=sgfAddPlay(curnode, who,i,j);
	 gmp_sendMove(ge, j, i);
	 sgf_move_made(i, j, who, moval);
	 pass = 0;
	 TRACE("\nmy move: %m\n\n", i, j);
       }

     }

     who = OTHER_COLOR(who);
   }

   /* two passes : game over */
   gmp_sendPass(ge);   

   /* We hang around here until cgoban asks us to go, since
    * sometimes cgoban crashes if we exit first
    */

   if (!get_opt_quiet())
     fprintf(stderr, "Game over---waiting for client to shut us down\n");
   sgf_close_file();

   who_wins(mymove,get_komi(),stderr);

   /*play_gmp() does not return to main(), therefore the analyzerfile
     writing code is here*/
   evaluate_territory(&white_points,&black_points);
   score=black_points-white_points-black_captured+white_captured-komi;
   sgfWriteResult(score);
   sgfShowTerritory(0);
   if(analyzerfile&&sgf_root)
       writesgf(sgf_root,analyzerfile,seed);
   
   while(!time_to_die)
   {
     message = gmp_check(ge, 1, &j, &i, &error);
     if (!get_opt_quiet())
       fprintf(stderr, "Message %d from gmp\n", message);
     if (message == gmp_err)
       break;
   }
   if (!get_opt_quiet())
     fprintf(stderr,"gnugo going down\n");
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
