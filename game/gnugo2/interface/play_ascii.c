/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU GO, a Go program. Contact gnugo@gnu.org, or see   *
 * http://www.gnu.org/software/gnugo/ for more information.      *
 *                                                               *
 * Copyright 1999 by the Free Software Foundation.               *
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




#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define BUILDING_GNUGO_ENGINE  /*FIXME ugly */
#include "../engine/liberty.h"
#include "interface.h"
#include "../sgf/sgf.h"
#include "../sgf/ttsgf.h"
#include "../sgf/ttsgf_write.h"
#include "../sgf/sgfana.h"

#define DEBUG_COMMANDS "\
capture <pos>    try to capture indicated group\n\
dead             Toggle display of dead stones\n\
defend <pos>     try to defend indicated group\n\
listdragons      print dragon info \n\
showarea         display area\n\
showdragons      display dragons\n\
showmoyo         display moyo\n\
showterri        display territory\n\
"


/* some options for the ascii interface */
static int opt_showboard = 1;
static int showscore = 0;
static int showdead = 0;
static int emacs = 0;
SGFNodeP curnode = 0;
char *filename = NULL;

void endgame(void);
void nngs_endgame(void);
void showcapture(char *line);
void showdefense(char *line);
void ascii_goto(char *line);
int ascii2pos(char *line, int *i, int *j);

/* --------------------------------------------------------------*/
/* create letterbar for the top and bottom of the ASCII board */
/* --------------------------------------------------------------*/
static void
make_letterbar(int bsize, char *letterbar)
{
  int i, letteroffset;
  char spaces[64];
  char letter[64];

  if (bsize <= 25)
    strcpy(spaces, " ");
  strcpy(letterbar, "   ");
  
  for (i=0; i<bsize; i++) {
    letteroffset = 'A';
    if (i+letteroffset >= 'I')
      letteroffset++;
    strcat(letterbar, spaces);
    sprintf(letter, "%c", i+letteroffset);
    strcat(letterbar, letter);
  }
}

/* this array contains +'s and -'s for the empty board posns.
 * hspot_size contains the board-size that the grid has been
 * initialised to.
 */

static int hspot_size;
static char hspots[MAX_BOARD][MAX_BOARD];

/* --------------------------------------------------------------*/
/* to mark the handicap spots on the board */
/* --------------------------------------------------------------*/
static void
set_handicap_spots(int bsize)
{
  if (hspot_size == bsize)
    return;
  
  hspot_size = bsize;
  
  memset(hspots, '.', sizeof(hspots));

  /* small sizes are easier to hardwire... */
  if (bsize==2 || bsize==4)
    return;
  if (bsize==3) {
    /* just the middle one */
    hspots[bsize/2][bsize/2] = '+';
    return;
  }
  if (bsize==5) {
    /* place the outer 4 */
    hspots[1][1] = '+';
    hspots[bsize-2][1] = '+';
    hspots[1][bsize-2] = '+';
    hspots[bsize-2][bsize-2] = '+';
    /* and the middle one */
    hspots[bsize/2][bsize/2] = '+';
    return;
  }

  if (!(bsize%2)) {
    /* if the board size is even, no center handicap spots */
    if (bsize > 2 && bsize < 12) {
      /* place the outer 4 only */
      hspots[2][2] = '+';
      hspots[bsize-3][2] = '+';
      hspots[2][bsize-3] = '+';
      hspots[bsize-3][bsize-3] = '+';
    }
    else {
      /* place the outer 4 only */
      hspots[3][3] = '+';
      hspots[bsize-4][3] = '+';
      hspots[3][bsize-4] = '+';
      hspots[bsize-4][bsize-4] = '+';
    }
  }
  else {
    if (bsize > 2 && bsize < 12) {
      /* place the outer 4 */
      hspots[2][2] = '+';
      hspots[bsize-3][2] = '+';
      hspots[2][bsize-3] = '+';
      hspots[bsize-3][bsize-3] = '+';
      /* and the middle one */
      hspots[bsize/2][bsize/2] = '+';
    }
    else {
      /* place the outer 4 */
      hspots[3][3] = '+';
      hspots[bsize-4][3] = '+';
      hspots[3][bsize-4] = '+';
      hspots[bsize-4][bsize-4] = '+';
      /* and the inner 4 */
      hspots[3][bsize/2] = '+';
      hspots[bsize/2][3] = '+';
      hspots[bsize/2][bsize-4] = '+';
      hspots[bsize-4][bsize/2] = '+';
      /* and the middle one */
      hspots[bsize/2][bsize/2] = '+';
    }
  }
  return;
}


/* --------------------------------------------------------------*/
/* the code to display the board position when playing in ASCII */
/* --------------------------------------------------------------*/
static void
ascii_showboard(int bsize)
{
   int i, j;
   char letterbar[64];
   int last_pos_was_move;
   int pos_is_move;
   int dead;
   
   make_letterbar(bsize, letterbar);
   set_handicap_spots(bsize);

   printf("\n");
   printf("    White has captured %d pieces\n", black_captured);
   printf("    Black has captured %d pieces\n", white_captured);
   if (showscore) {
     printf("    aprox. White territory: %d\n",
	    terri_eval[WHITE] - black_captured);
     printf("    aprox. Black territory: %d\n",
	    terri_eval[BLACK] - white_captured);
   }
   printf("\n");

   fflush (stdout);
   printf("%s%s\n", (emacs ? "EMACS1\n" : ""), letterbar);
   fflush (stdout);

   for (i = 0; i < bsize; i++) {
     printf(" %2d",bsize-i);
     last_pos_was_move = 0;
     for (j = 0; j < bsize; j++) {
       if ((last_move_i==i) && (last_move_j==j))
	 pos_is_move = 128;
       else
	 pos_is_move = 0;
       dead = (dragon[i][j].status==DEAD) && showdead;
       switch (p[i][j] + pos_is_move + last_pos_was_move) {
       case EMPTY+128:
       case EMPTY:
	 printf(" %c", hspots[i][j]);
	 last_pos_was_move = 0;
	 break;
       case BLACK:
	 printf(" %c", dead ? 'x' : 'X');
	 last_pos_was_move = 0;
	 break;
       case WHITE:
	 printf(" %c", dead ? 'o' : 'O');
	 last_pos_was_move = 0;
	 break;
       case BLACK+128:
	 printf("(%c)", dead ? 'X' : 'X');
	 last_pos_was_move = 256;
	 break; /* why have the newly placed stones the status dead? */
       case WHITE+128:
	 printf("(%c)", dead ? 'O' : 'O');
	 last_pos_was_move = 256;
	 break;
       case EMPTY+256:
	 printf("%c", hspots[i][j]);
	 last_pos_was_move = 0;
	 break;
       case BLACK+256:
	 printf("%c", dead ? 'x' : 'X');
	 last_pos_was_move = 0;
	 break;
       case WHITE+256:
	 printf("%c", dead ? 'o' : 'O');
	 last_pos_was_move = 0;
	 break;
       default: 
	 fprintf(stderr,"Illegal board value %d\n",p[i][j]);
	 exit(EXIT_FAILURE);
	 break;
       }
     }
     if (last_pos_was_move==0) {
       if (bsize>10)
	 printf(" %2d", bsize-i);
       else
	 printf(" %1d", bsize-i);
     }
     else {
       if (bsize>10)
	 printf("%2d",bsize-i);
       else
         printf("%1d",bsize-i);
     }
     printf("\n");
   }

   fflush (stdout);
   printf("%s\n\n", letterbar);
   fflush (stdout);
   
}  /* end ascii_showboard */


/* --------------------------------------------------------------*/
/* command help */
/* --------------------------------------------------------------*/
static void
show_commands(void)
{
  printf("\nCommands:\n");
  printf(" back             Take back your last move\n");
  printf(" boardsize        Set boardsize (on move 1 only!)\n");
  printf(" comment          Write a comment to outputfile\n");
  printf(" depth <num>      Set depth for reading\n");
  printf(" display          Display game board\n");
  printf(" exit             Exit GNU Go\n");
  printf(" force <move>     Force a move for current color\n");
  printf(" forward          Go to next node in game tree\n");
  printf(" goto <movenum>   Go to movenum in game tree\n");
  printf(" handicap         Set handicap (on move 1 only!)\n");
  printf(" help             Display this help menu\n");
  printf(" helpdebug        Display debug help menu\n");
  printf(" info             Display program settings\n");
  printf(" komi             Set komi (on move 1 only!)\n");
  printf(" last             Goto last node in game tree\n");
  printf(" pass             Pass on your move\n");
  printf(" play <num>       Play <num> moves\n");
  printf(" playblack        Play as Black (switch if White)\n");
  printf(" playwhite        Play as White (switch if Black)\n");
  printf(" quit             Exit GNU Go\n");
  printf(" resign           Resign the current game\n");
  printf(" score            Toggle display of score On/Off\n");
  printf(" showboard        Toggle display of board On/Off\n");
  printf(" switch           Switch the color you are playing\n");
  printf(" undo             Take the last move back (same as back)\n");
  printf(" <move>           A move of the format <letter><number>");
  printf("\n");
}

enum commands {INVALID=-1, END, EXIT, QUIT, RESIGN, 
	       PASS, MOVE, FORCE, SWITCH,
	       PLAY, PLAYBLACK, PLAYWHITE,
	       SETHANDICAP, SETBOARDSIZE, SETKOMI,
	       SETDEPTH,
               INFO, DISPLAY, SHOWBOARD, HELP, UNDO, COMMENT, SCORE,
               CMD_DEAD,CMD_BACK,CMD_FORWARD,CMD_LAST,
               CMD_CAPTURE,CMD_DEFEND,
               CMD_HELPDEBUG,CMD_SHOWAREA,CMD_SHOWMOYO,CMD_SHOWTERRI,
               CMD_GOTO,CMD_SAVE,CMD_SHOWDRAGONS,CMD_LISTDRAGONS,
	       NNGS,
};

/* --------------------------------------------------------------*/
/* check if we have a valid command */
/* --------------------------------------------------------------*/
static int
get_command(char *command)
{
  char c;
  int d;

  /* check to see if a move was input */
  if (!((sscanf(command, "%c%d", &c, &d) != 2)
	|| ((c=toupper(c)) < 'A')
	|| ((c=toupper(c)) > 'Z')
	|| (c == 'I')))
    return MOVE;

  /* help shortcut */
  if (command[0]=='?')
    return HELP;

  /* kill leading spaces */
  while (command[0]==' ')
    command++;


  if (!strncmp(command, "playblack", 9)) return PLAYBLACK;
  if (!strncmp(command, "playwhite", 9)) return PLAYWHITE;
  if (!strncmp(command, "showboard", 9)) return SHOWBOARD;
  if (!strncmp(command, "showdragons", 9)) return CMD_SHOWDRAGONS;
  if (!strncmp(command, "listdragons", 9)) return CMD_LISTDRAGONS;
  if (!strncmp(command, "boardsize", 9)) return SETBOARDSIZE;
  if (!strncmp(command, "handicap", 8)) return SETHANDICAP;
  if (!strncmp(command, "display", 7)) return DISPLAY;
  if (!strncmp(command, "helpdebug", 7)) return CMD_HELPDEBUG;
  if (!strncmp(command, "resign", 6)) return RESIGN;
  if (!strncmp(command, "showmoyo", 6)) return CMD_SHOWMOYO;
  if (!strncmp(command, "showterri", 6)) return CMD_SHOWTERRI;
  if (!strncmp(command, "showarea", 6)) return CMD_SHOWAREA;
  if (!strncmp(command, "depth", 5)) return SETDEPTH;
  if (!strncmp(command, "switch", 5)) return SWITCH;
  if (!strncmp(command, "komi", 4)) return SETKOMI;
  if (!strncmp(command, "play", 4)) return PLAY;
  if (!strncmp(command, "info", 4)) return INFO;
  if (!strncmp(command, "force", 4)) return FORCE;
  if (!strncmp(command, "pass", 4)) return PASS;
  if (!strncmp(command, "nngs", 4)) return NNGS;
  if (!strncmp(command, "save", 3)) return CMD_SAVE;
  if (!strncmp(command, "end", 3)) return END;
  if (!strncmp(command, "move", 3)) return MOVE;
  if (!strncmp(command, "undo", 3)) return UNDO;
  if (!strncmp(command, "comment", 3)) return COMMENT;
  if (!strncmp(command, "score", 3)) return SCORE;
  if (!strncmp(command, "dead", 3)) return CMD_DEAD;
  if (!strncmp(command, "capture", 3)) return CMD_CAPTURE;
  if (!strncmp(command, "defend", 3)) return CMD_DEFEND;
  if (!strncmp(command, "exit", 2)) return EXIT;
  if (!strncmp(command, "quit", 1)) return QUIT;
  if (!strncmp(command, "help", 1)) return HELP;
  if (!strncmp(command, "back", 1)) return CMD_BACK;
  if (!strncmp(command, "forward", 1)) return CMD_FORWARD;
  if (!strncmp(command, "last", 1)) return CMD_LAST;
  if (!strncmp(command, "goto", 1)) return CMD_GOTO;

  /* no valid command found */
  return INVALID;
}

/* --------------------------------------------------------------*/
/* Write sgf root node.                                          */
/* --------------------------------------------------------------*/
static void
init_sgf(int *sgf_initialized)
{
  if (!*sgf_initialized) {
    sgf_write_game_info(get_boardsize(), get_handicap(),
                        get_komi(), get_seed(), "ascii");
    if (get_handicap() > 0) {
      if (!filename)
        set_handicap(sethand(get_handicap()));
    }
    else
      set_tomove(BLACK);

  }
  *sgf_initialized = 1;
}

/* --------------------------------------------------------------*/
/* generate the computer move                                    */
/* --------------------------------------------------------------*/
static void
computer_move(int movecolor, int *pass, int *sgf_initialized)
{
  int i, j;
  int move_val;

  init_sgf(sgf_initialized);
  
  /* generate computer move */
  move_val = get_move(&i, &j, movecolor);  
  /*ttsgf_move_made(i, j, get_tomove(), move_val);*/
  curnode = sgfAddPlay(curnode, movecolor, i, j);
  
  if (move_val) {
    /* An ordinary move */
    if (movecolor==WHITE)
      mprintf("White(%d): %m\n", get_movenumber(), i, j);
    if (movecolor==BLACK)
      mprintf("Black(%d): %m\n", get_movenumber(), i, j);
    *pass = 0;
  }
  else {
    if (movecolor==WHITE)
      mprintf("White(%d): Pass\n", get_movenumber());
    if (movecolor==BLACK)
      mprintf("Black(%d): Pass\n", get_movenumber());
    (*pass)++;
  }

  switch_tomove();
}

/* --------------------------------------------------------------*/
/* make a move                                                   */
/* --------------------------------------------------------------*/
static void
do_move(char *command, int *pass, int force, int *sgf_initialized)
{
  char c;
  int d, i, j;
  sscanf(command, "%c%d", &c, &d);
  /* got a move... check if it's legal... */
  /* compensate for no 'I' on the board */
  c = toupper(c);
  if ((c - 'J') > (get_boardsize() - 9)
      || (d <= 0 || d > get_boardsize())) {
    printf("\nInvalid move: %c%d\n", c, d);
    return;
  }
  
  if (c > 'I')
    c--;
  i = get_boardsize() - d;
  j = c - 'A';
  
  if (!legal(i, j, get_tomove())) {
    if (c >= ('I'))
      c++;
    printf("\nIllegal move: %c%d\n", c, d);
    return;
  }
  else {
    *pass = 0;
    inc_movenumber();
    TRACE("\nyour move: %m\n\n", i, j);
    updateboard(i, j, get_tomove());
    init_sgf(sgf_initialized);
    sgf_move_made(i, j, get_tomove(), 0);
    /*ttsgf_move_made(i, j, get_tomove(), 0);*/
    curnode = sgfAddPlay(curnode, get_tomove(), i, j);

  }
  if (opt_showboard && !emacs) {
    ascii_showboard(get_boardsize());
    printf("GNU Go is thinking...\n");
  }
  if (force) {
    switch_computer_player();
    switch_tomove();
    return;
  }
  switch_tomove();
  computer_move(get_tomove(), pass, sgf_initialized);
}

/* --------------------------------------------------------------*/
/* make a pass                                                   */
/* --------------------------------------------------------------*/
static void
do_pass(int *pass, int force, int *sgf_initialized)
{
  (*pass)++;
  updateboard(get_boardsize(), get_boardsize(), get_tomove());
  inc_movenumber();
  init_sgf(sgf_initialized);
  sgf_move_made(get_boardsize(), get_boardsize(), get_tomove(), 0);
  curnode = sgfAddPlay(curnode, get_tomove(), get_boardsize(),
		       get_boardsize());

  if (force) {
    switch_computer_player();
    switch_tomove();
    return;
  }
  switch_tomove();
  computer_move(get_tomove(), pass, sgf_initialized);
}  

/* --------------------------------------------------------------*/
/* play a game against an ascii client                           */
/* --------------------------------------------------------------*/
void
play_ascii(char *fname)
{
  int m, num, sz;
  float fnum;
  int pass = 0;  /* two passes and its over */
  int tmp;
  char line[80];
  char *line_ptr = line;
  char *command;
  char *tmpstring;
  int sgf_initialized = 0;
  int nngs = 0; /* NNGS style customization */
  
  curnode = sgf_root;
  filename = fname;

#ifdef HAVE_SETLINEBUF
  setlinebuf(stdout); /* need at least line buffer NNGS and gnugo-gnugo */
#else
  setbuf(stdout, NULL); /* else set it to completely UNBUFFERED */
#endif

  if (sgfGetIntProperty(curnode, "SZ", &sz))
    set_boardsize(sz);
  if (get_handicap() > 0) {
    set_tomove(WHITE);
    if (filename) {
      sgfAddMoveFromTree(sgf_root,get_tomove());
    }
  }
  else
    set_tomove(BLACK);

  printf("\nBeginning ASCII mode game.\n\n");
  print_ginfo();
  
  /* start the move numbering at 0... */
  /* when playing with gmp it starts at 0 /NE */
  set_movenumber(0);
  
  /* does the computer play first?  If so, make a move */
  if (is_computer_player(get_tomove()))
    computer_move(get_tomove(), &pass, &sgf_initialized);
  
  /* main ASCII Play loop */
  while (pass < 2 && !time_to_die) {
    /* display game board */
    if (opt_showboard)
      ascii_showboard(get_boardsize());
      
    /* print the prompt */
    if (get_tomove() == WHITE)
      printf("White(%d): ", get_movenumber()+1);
    if (get_tomove() == BLACK)
      printf("Black(%d): ", get_movenumber()+1);
      
    /* read the line of input */
    line_ptr = line;
    if (!fgets(line, 80, stdin))
      break; /* EOF or some error */
      
    while (command = strtok(line_ptr, ";"), line_ptr = 0, command) {
      
      /* get the command or move */
      switch (get_command(command)) {
      case RESIGN:
	printf("\nGNU Go wins by resignation.");
	sgfWriteResult(get_tomove()==BLACK ? (float) -1000 : (float) 1000);
      case END:
      case EXIT:
      case QUIT:
	printf("\nThanks for playing GNU Go.\n\n");
	return ;
	break;
      case HELP:
	show_commands();
	break;
      case CMD_HELPDEBUG:
	printf(DEBUG_COMMANDS);
	break;
      case SHOWBOARD:
	opt_showboard = !opt_showboard;
	break;
      case INFO:
	printf("\n");
	print_ginfo();
	break;
      case SETBOARDSIZE:
	if (get_movenumber() > 0) {
	  printf("Boardsize can be modified on move 1 only!\n");
	  break;
	}
	command += 10;
	if (sscanf(command, "%d", &num) != 1) {
	  printf("\nInvalid command syntax!\n");
	  break;
	}
	if (num<2 || num>25) {
	  printf("\nInvalid board size: %d\n", num);
	  break;
	}
	/* init board */
	clear_board(NULL);
	set_boardsize(num);
	break;
      case SETHANDICAP:
	if (get_movenumber() > 0) {
	  printf("Handicap can be modified on move 1 only!\n");
	  break;
	}
	command += 9;
	if (sscanf(command, "%d", &num) != 1) {
	  printf("\nInvalid command syntax!\n");
	  break;
	}
	if (num<0 || num>MAX_HANDICAP) {
	  printf("\nInvalid handicap: %d\n", num);
	  break;
	}
	/* init board */
	clear_board(NULL);
	set_handicap(num);
	printf("\nSet handicap to %d\n", get_handicap());
	init_sgf(&sgf_initialized); /* not good to do it here, but -o needs it */
	set_tomove(WHITE);
	break;
      case SETKOMI:
	if (get_movenumber() > 0) {
	  printf("Komi can be modified on move 1 only!\n");
	  break;
	}
	command += 5;
	if (sscanf(command, "%f", &fnum) != 1) {
	  printf("\nInvalid command syntax!\n");
	  break;
	}
	set_komi(fnum);
	printf("\nSet Komi to %.1f\n", get_komi());
	break;
	
      case SETDEPTH:
	{
	  extern int depth;	/* ugly hack */
	  
	  command += 6;
	  if (sscanf(command, "%d", &num) != 1) {
	    printf("\nInvalid command syntax!\n");
	    break;
	  }
	  depth = num;
	  printf("\nSet depth to %d\n", depth);
	  break;
	}
      
      case DISPLAY:
	if (!opt_showboard)
	  ascii_showboard(get_boardsize());
	break;
      case FORCE:
	command += 6; /* skip the force part... */
	switch (get_command(command)) {
	case MOVE:
	  do_move(command, &pass, 1, &sgf_initialized);
	  break;
	case PASS:
	  do_pass(&pass, 1, &sgf_initialized);
	  break;
	default:
	  printf("Illegal forced move: %s %d\n", command,
		 get_command(command));
	  break;
	}
	break;
      case MOVE:
	do_move(command, &pass, 0, &sgf_initialized);
	break;
      case PASS:
	do_pass(&pass, 0, &sgf_initialized);
	break;
      case PLAY:
	command += 5;
	if (sscanf(command, "%d", &num) != 1) {
	  printf("\nInvalid command syntax!\n");
	  break;
	}
	if (num >= 0)
	  for (m=0; m<num; m++) {
	    switch_computer_player();
	    computer_move(get_tomove(), &pass, &sgf_initialized);
	    if (pass >= 2)
	      break;
	  }
	else {
	  printf("\nInvalid number of moves specified: %d\n", num);
	  break;
	}
	break;
      case PLAYBLACK:
	if (get_computer_player() == WHITE)
	  switch_computer_player();
	if (is_computer_player(get_tomove()))
	  computer_move(get_tomove(), &pass, &sgf_initialized);
	break;
      case PLAYWHITE:
	if (get_computer_player() == BLACK)
	  switch_computer_player();
	if (is_computer_player(get_tomove()))
	  computer_move(get_tomove(), &pass, &sgf_initialized);
	break;
      case SWITCH:
	switch_computer_player();
	computer_move(get_tomove(), &pass, &sgf_initialized);
	break;
	/*FIXME
	  don't forget boardupdate after back!
	  nach back boardupdate nicht vergessen!
	  */
      case UNDO:
      case CMD_BACK:
	/*reloading the game*/
	{
	  int movenumber = get_movenumber();
	  int next;
	  if (movenumber > 0) {
	    curnode = sgf_root;
	    next = sgfPlayTree(&curnode, &movenumber);
	    movenumber--;
	    set_tomove(next);
	    set_movenumber(movenumber);
	  }
	  else
	    printf("\nBeginning of game tree.\n");
	}
        break;
      case CMD_FORWARD:
	if (curnode->child) {
	  set_tomove(sgfAddMoveFromTree(curnode->child, get_tomove()));
	  curnode = curnode->child;
#if 0
	  make_worms();
	  make_dragons();
	  last_move_i = -1;
	  last_move_j = -1;
#endif
	}
	else
	  printf("\nEnd of game tree.\n");
	break;
      case CMD_LAST:
	while (curnode->child) {
	  set_tomove(sgfAddMoveFromTree(curnode->child, get_tomove()));
	  curnode = curnode->child;
	}
	break;
      case COMMENT:
	printf("\nEnter comment. Press ENTER when ready.\n");
	fgets(line, 80, stdin);
	sgfAddComment(curnode, line);
	break;
      case SCORE:
	showscore = !showscore;
	break;
      case CMD_DEAD:
	showdead = !showdead;
	break;
      case CMD_CAPTURE:
	strtok(command, " ");
	showcapture(strtok(0, " "));
	break;
      case CMD_DEFEND:
	strtok(command, " ");
	showdefense(strtok(0, " "));
	break;
      case CMD_SHOWMOYO:
	tmp = printmoyo;
	printmoyo = 4;
	make_worms();
	make_dragons();
	make_moyo(get_tomove());
	print_moyo(0);
	printmoyo = tmp;
	break;
      case CMD_SHOWTERRI:
	tmp = printmoyo;
	printmoyo = 1;
	make_worms();
	make_dragons();
	make_moyo(get_tomove());
	print_moyo(0);
	printmoyo = tmp;
	break;
      case CMD_SHOWAREA:
	tmp = printmoyo;
	printmoyo = 16;
	make_worms();
	make_dragons();
	make_moyo(get_tomove());
	print_moyo(0);
	printmoyo = tmp;
	break;
      case CMD_SHOWDRAGONS:
	make_worms();
	make_dragons();
	showboard(1);
	break;
      case CMD_GOTO:
	strtok(command, " ");
	ascii_goto(strtok(0, " "));
	break;
      case CMD_SAVE:
	strtok(command, " ");
	tmpstring = strtok(0, " ");
	if (tmpstring) {
	  tmpstring[strlen(tmpstring)-1] = 0;
	  writesgf(sgf_root, tmpstring, get_seed());
	}
	break;
      case CMD_LISTDRAGONS:
	make_worms();
	make_dragons();
	show_dragons();
	break;
      case NNGS:
	/* Setting the nngs flag will customize the ascii interface
	 * for use by the NNGS connecting program. This mode of
	 * operation is unlikely to be useful for other purposes.
	 */
	nngs = 1;
	break;
      case INVALID:
      default:
	printf("\nInvalid command: %s",command);
	break;
      }
    }
  }

  /* two passes : game over */

  if (pass >= 2) {
    printf("My final evaluation of the game :\n");
    who_wins(get_computer_player(), get_komi(), stdout);
    if (nngs)
      nngs_endgame();
    else
      endgame();
  }
  printf("\nThanks for playing GNU Go.\n\n");
}

void
play_ascii_emacs(char *fname)
{
  emacs=1;
  play_ascii(fname);
}

/* endgame() scores the game. */

void
endgame(void)
{
  char line[12];
  int done = 0;
  int d, i, j;
  char c;
  float score;
  int black_points = 0;
  int white_points = 0;
  int xyzzy = 1;
  float komi;

  komi = get_komi();

  save_state();
  printf("\nGame over. Let's count!.\n");  
  showdead = 1;
  ascii_showboard(board_size);
  while (!done) {
    printf("Dead stones are marked with small letters (x,o).\n");
    printf("\nIf you don't agree, enter the location of a group\n\
to toggle its state or \"done\".\n");

    if (!fgets(line, 12, stdin))
      return; /* EOF or some error */
    
    for (i=0; i<12; i++)
      line[i] = (isupper ((int) line[i]) ? tolower (line[i]) : line[i]);
    if (!strncmp(line, "done", 4))
      done = 1;
    else if (!strncmp(line, "quit", 4))
      return;
    else if (!strncmp(line, "xyzzy", 5)) {
      if (xyzzy) {
	printf("\nYou are in a debris room filled with stuff washed in from the\n");
	printf("surface.  A low wide passage with cobbles becomes plugged\n");
	printf("with mud and debris here, but an awkward canyon leads\n");
	printf("upward and west.  A note on the wall says:\n");
	printf("   Magic Word \"XYZZY\"\n\n");
	xyzzy = 0;
      }
      else {
	printf("You are inside a building, a well house for a large spring.\n\n");
	xyzzy = 1;
      }
    }
    else if (!strncmp(line, "help", 4)) {
      printf("\nIf there are dead stones on the board I will remove them.\n");
      printf("You must tell me where they are. Type the coordinates of a\n");
      printf("dead group, or type \"done\"\n");
    }      
    else if (!strncmp(line, "undo", 4)) {
      /* restore_state(); */
      printf("UNDO not allowed anymore. The status of the stones now toggles after entering the location of it.\n");
      ascii_showboard(board_size);
    }
    else {
      sscanf(line, "%c%d", &c, &d);
      c = toupper(c);
      if (c > 'I')
	c--;
      i = board_size - d;
      j = c - 'A';
      if ((i < 0) || (i >= board_size) || (j < 0) || (j >= board_size) || 
	  (p[i][j] == EMPTY))
	printf("\ninvalid!\n");
      else {
	int status = dragon[i][j].status;
	status = (status == DEAD) ? ALIVE : DEAD;
	change_dragon_status(i, j, status);
	/* remove_string(i, j); */
	ascii_showboard(board_size);
      }
    }
  }

  /* count_territory(&white_points, &black_points); */

  evaluate_territory(&white_points, &black_points);

  printf("\n\nBlack has %d points and has captured %d white stones\n",
	 black_points, white_captured);
  printf("White has %d points and has captured %d black stones\n",
	 white_points, black_captured);
  printf("komi is %.1f.\n", komi);

  score = (black_points - white_points - black_captured + white_captured
	   - komi);
  sgfShowTerritory(0);
  sgfWriteResult(score);

  if (score>0) 
    printf("\nBLACK WINS by %.1f points\n", score);
  else if (score<0)
    printf("\nWHITE WINS by %.1f points\n", -score);    
  else
    printf("\nTIED GAME!\n");
}

/* nngs_endgame() records the strings the opponent says are dead and
   compares this to our own opinion. */
void
nngs_endgame(void)
{
  char line[12];
  int d, i, j;
  char c;
  int worm_status[MAX_BOARD][MAX_BOARD];
  int m, n;
  static const char *snames[] = 
    {"dead", "alive", "critical", "unknown"};
  
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      worm_status[m][n] = ALIVE;
  
  while (1) {
    printf("Bring out your dead!\n");

    if (!fgets(line, 12, stdin))
      return; /* EOF or some error */
    
    for (i=0; i<12; i++)
      line[i] = (isupper ((int) line[i]) ? tolower (line[i]) : line[i]);

    if (!strncmp(line, "done", 4))
      break;
    else if (!strncmp(line, "quit", 4))
      return;
    else if (!strncmp(line, "pass", 4))  /* ignore */
      continue;
    else if (!strncmp(line, "undo", 4)) {
      for (m=0; m<board_size; m++)
	for (n=0; n<board_size; n++)
	  worm_status[m][n] = ALIVE;
    }
    else {
      sscanf(line, "%c%d", &c, &d);
      c = toupper(c);
      if (c > 'I')
	c--;
      i = board_size - d;
      j = c - 'A';
      if ((i < 0) || (i >= board_size) || (j < 0) || (j >= board_size)
	  || (p[i][j] == EMPTY))
	printf("\ninvalid!\n");
      else {
	for (m=0; m<board_size; m++)
	  for (n=0; n<board_size; n++)
	    if (worm[m][n].origini == worm[i][j].origini
		&& worm[m][n].originj == worm[i][j].originj)
	      worm_status[m][n] = DEAD;
      }
    }
  }

  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++)
      if (worm[m][n].origini == m
	  && worm[m][n].originj == n
	  && p[m][n] != EMPTY) {
	if (worm_status[m][n] != dragon[m][n].status) {
	  char buf[120];
	  sprintf(buf,
		  "Disagreement over %c%d: GNU Go says %s, opponent says %s.\n",
 		  'A'+n+(n>=8), board_size-m, snames[dragon[m][n].status],
		  snames[worm_status[m][n]]);
	  mprintf(buf);
	  sgfAddComment(curnode, buf);
	}
      }
}


void
showcapture(char *line)
{
  int i, j, x, y;
  if (line)
    if (!ascii2pos(line, &i, &j)) {
      printf("\ninvalid point!\n");
      return;
    }
  if (attack(i, j, &x, &y))
    mprintf("\nSuccessfull attack of %m at %m\n", i, j, x, y);
  else
    mprintf("\n%m cannot be attacked\n", i, j);
}

void
showdefense(char *line)
{
  int i, j, x, y;
  if (line)
    if (!ascii2pos(line, &i, &j)) {
      printf("\ninvalid point!\n");
      return;
    }
    if (attack(i, j, &x, &y)) {
      if (find_defense(i, j, &x, &y))
        mprintf("\nSuccessfull defense of %m at %m\n", i, j, x, y);
      else
        mprintf("\n%m cannot be defended\n", i, j);
    }
    else
      mprintf("\nThere is no need to defend %m\n", i, j);
}

void
ascii_goto(char *line)
{
  int movenumber = 0;
  if (!line)
    return;
  if (!strncmp(line, "last", 4))
    movenumber = 9999;
  else {
    if (!strncmp(line, "first", 4))
      movenumber = 1;
    else
      sscanf(line, "%i", &movenumber);
  }
  printf("goto %i\n", movenumber);
  curnode = sgf_root;
  set_tomove(sgfPlayTree(&curnode, &movenumber));
  set_movenumber(movenumber-1);
  return;
}


int
ascii2pos(char *line, int *i, int *j)
{
  int d;
  char c;
  if (sscanf(line, "%c%d", &c, &d) == 2) {
    c = toupper(c);
    if (c > 'I') c--;
    *i = board_size - d;
    *j = c - 'A';
    if ((*i < 0) || (*i >= board_size)
	|| (*j<0) || (*j >= board_size)
	|| (p[*i][*j] == EMPTY))
      return 0;
    else
      return 1;
  }
  return 0;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
