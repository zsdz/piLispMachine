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




/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
 *    Ditto for AIX 3.2 and <stdlib.h>.  
 */

#ifndef _NO_PROTO
#define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define BUILDING_GNUGO_ENGINE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <time.h>
#include <signal.h>

#ifdef HAVE_VISUAL_C
#include <io.h>  /* MS-C has read and write here... */
#endif

#include "getopt.h"

#include "../engine/main.h"
#include "interface.h"
#include "gmp.h"
#include "../sgf/sgf.h"
#include "../sgf/sgf_utils.h"
#include "../sgf/ttsgf_read.h"
#include "../sgf/ttsgf_write.h"
#include "../sgf/ttsgf.h"
#include "../sgf/sgfana.h"
#include "../engine/liberty.h"
#include "../engine/hash.h"


/* 
 * Show usage of GNU Go.
 *
 * This string is modelled on the gnu tar --help output.
 */

#define USAGE "\n\
Usage : gnugo [-opts]\n\
\n\
Main Options:\n\
       --mode <mode>     Force the playing mode ('ascii', 'test' or 'gmp').\n\
                         Default is ASCII.\n\
                         If no terminal is detected GMP (Go Modem Protocol)\n\
                         will be assumed.\n\
       --quiet           Don't print copyright and other messages\n\
   -l, --infile <file>   Load name sgf file\n\
   -L, --until <move>    Stop loading just before move is played. <move>\n\
                         can be the move number or location (eg L10).\n\
   -o, --outfile file    Write sgf output to file\n\
   -p, --playstyle <style>    style of play, use --help playstyle for usage\n\
\n\
Options that affect strength (higher=stronger, slower):\n\
   -D, --depth [depth]          deep reading cutoff (default %d)\n\
   -B, --backfill_depth [depth] deep reading cutoff (default %d)\n\
   -F, --fourlib_depth [depth]  deep reading cutoff (default %d)\n\
   -K, --ko_depth [depth]       deep reading cutoff (default %d)\n\
\n\
Option that affects speed (higher=faster, more memory usage):\n\
   -M, --memory [megabytes]     hash memory (default %d)\n\n\
Game Options: (--mode ascii)\n\
       --boardsize num   Set the board size to use (%d--%d)\n\
       --color <color>   Choose your color ('black' or 'white')\n\
       --handicap <num>  Set the number of handicap stones (0--%d)\n\
       --komi <num>      Set the komi\n\
\n\
Informative Output:\n\
   -v, --version         Display the version of GNU Go\n\
   -h, --help            Display this help message\n\
       --help analyze    Display help about analyzer options\n\
       --help debug      Display help about debugging options\n\
       --help playstyle  Display help about playstyle options\n\
       --copyright       Display copyright notice\n\
\n\
"

#define COPYRIGHT "\n\n\
This is GNU GO, a Go program. Contact gnugo@gnu.org, or see\n\
http://www.gnu.org/software/gnugo/ for more information.\n\n\
Copyright 1999 by the Free Software Foundation.\n\n\
This program is free software; you can redistribute it and/or\n\
modify it under the terms of the GNU General Public License\n\
as published by the Free Software Foundation - version 2.\n\n\
This program is distributed in the hope that it will be\n\
useful, but WITHOUT ANY WARRANTY; without even the implied\n\
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n\
PURPOSE.  See the GNU General Public License in file COPYING\n\
for more details.\n\n\
You should have received a copy of the GNU General Public\n\
License along with this program; if not, write to the Free\n\
Software Foundation, Inc., 59 Temple Place - Suite 330,\n\
Boston, MA 02111, USA\n\n\
"
#define USAGE_DEBUG "\n\
Debugging Options:\n\
\n\
       --testmode <mode> set the test mode to one of the following:\n\
                         (requires -l, implies --mode test)\n\
	             move: test at move node only\n\
		     annotation: test at annotation node only\n\
		     both: test at move and annotation nodes\n\
		     game: test to see if gnugo considered each move made\n\
                 This overrides a testmode=... comment in the sgf file\n\
   -a, --allpats                test all patterns\n\
   -T, --printboard             colored display of dragons\n\
   -E                           colored display of eye spaces\n\
   -d, --debug [level]          debugging output (see liberty.h for bits)\n\
   -H, --hash [level]		hash (see liberty.h for bits)\n\
   -w, --worms                  worm debugging\n\
   -m, --moyo [level]           moyo debugging, show moyo board\n\
   -b, --benchmark num          benchmarking mode - can be used with -l\n\
   -s, --stack                  stack trace (for debugging purposes)\n\n\
   -S, --statistics             print statistics (for debugging purposes)\n\n\
   -t, --trace                  verbose tracing (use twice or more to trace reading)\n\
   -r, --seed number            set random number seed\n\
       --decidestring string    give full read tracing when studying this string\n\
       --score [end|last|move]  count or estimate territory of the input file\n\
       --printsgf outfile       load SGF file, output final position (requires -l)\n\
"
    

/* long options which have no short form */
enum {OPT_BOARDSIZE=1,
      OPT_HANDICAPSTONES,
      OPT_COLOR,
      OPT_KOMI,
      OPT_MODE,
      OPT_INFILE,
      OPT_OUTFILE, 
      OPT_QUIET,
      OPT_SHOWCOPYRIGHT,
      OPT_TESTMODE,
      OPT_DECIDE_STRING,
      OPT_SCORE,
      OPT_PRINTSGF,
      OPT_ANALYZER_FILE,
      OPT_ANALYZE,
      OPT_HELPANALYZE,
};

/* names of playing modes */

enum mode {
  MODE_UNKNOWN=0,
  MODE_ASCII,
  MODE_ASCII_EMACS,
  MODE_GMP,
  MODE_SGF,
  MODE_LOAD_AND_ANALYZE,
  MODE_LOAD_AND_SCORE,
  MODE_LOAD_AND_PRINT,
  MODE_SOLO,
  MODE_TEST,
  MODE_DECIDE_STRING
};


/* Definitions of the --long options. Final column is
 * either an OPT_ as defined in the enum above, or it
 * is the equivalent single-letter option.
 * It is useful to keep them in the same order as the
 * help string, for maintenance purposes only.
 */

static struct option const long_options[] =
{
  {"mode",     required_argument, 0, OPT_MODE},
  {"testmode", required_argument, 0, OPT_TESTMODE},
  {"quiet", no_argument, 0, OPT_QUIET},

  {"infile",         required_argument, 0, 'l'},
  {"until",          required_argument, 0, 'L'},
  {"outfile",        required_argument, 0, 'o'},

  {"boardsize",      required_argument, 0, OPT_BOARDSIZE},
  {"color",          required_argument, 0, OPT_COLOR},
  {"handicap",       required_argument, 0, OPT_HANDICAPSTONES},
  {"komi",           required_argument, 0, OPT_KOMI},

  {"help",           optional_argument, 0, 'h'},
  {"helpanalyze",    no_argument,       0, OPT_HELPANALYZE },
  {"copyright",      no_argument,       0, OPT_SHOWCOPYRIGHT},
  {"version",        no_argument,       0, 'v'},

  {"allpats",        no_argument,       0, 'a'},
  {"printboard",     no_argument,       0, 'T'},
  {"debug",          required_argument, 0, 'd'},
  {"depth",          required_argument, 0, 'D'},
  {"backfill_depth", required_argument, 0, 'B'},
  {"fourlib_depth",  required_argument, 0, 'F'},
  {"ko_depth",       required_argument, 0, 'K'},
#if HASHING
  {"memory",         required_argument, 0, 'M'},
  {"hash",           required_argument, 0, 'H'},
#endif
  {"worms",          no_argument,       0, 'w'},
  {"moyo",           required_argument, 0, 'm'},
  {"benchmark",      required_argument, 0, 'b'},
  {"stack",          no_argument,       0, 's'},
  {"statistics",     no_argument,       0, 'S'},
  {"trace",          no_argument,       0, 't'},
  {"seed",           required_argument, 0, 'r'},
  {"playstyle",      required_argument, 0, 'p'},
  {"decidestring",   required_argument, 0, OPT_DECIDE_STRING},
  {"score",          required_argument, 0, OPT_SCORE},
  {"printsgf",       required_argument, 0, OPT_PRINTSGF},
  {"analyzerfile",   required_argument, 0, OPT_ANALYZER_FILE},
  {"analyze",        required_argument, 0, OPT_ANALYZE},
  {NULL, 0, NULL, 0}
};

int memory = MEMORY;		/* Megabytes of memory used for hash table. */

/* 
 * Cgoban sends us a sigterm when it wants us to die. But it doesn't
 * close the pipe, so we cannot rely on gmp to pick up an error.
 * 
 * We want to keep control so we can close the output sgf file
 * properly, so we trap the signal.
 */

volatile int  time_to_die = 0;   /* set by signal handlers */

void 
sigterm_handler(int sig)
{
  time_to_die = 1;
  if (!get_opt_quiet)
    write(2, "*SIGTERM*\n", 10);  /* bad to use stdio in a signal handler */

  close(0);                  /* this forces gmp.c to return an gmp_err */

  /* I thought signal handlers were one-shot, yet on my linux box it is kept.
   * Restore the default behaviour so that a second signal has the
   * original effect - in case we really are stuck in a loop
   */
  signal(sig, SIG_DFL);

  /* schedule a SIGALRM in 5 seconds, in case we haven't cleaned up by then
   * - cgoban sends the SIGTERM only once 
   */
#ifdef HAVE_ALARM
  alarm(5);
#endif
}


void
sigint_handler(int sig)
{
  time_to_die = 1;
  write(2, "*SIGINT*\n", 9);  /* bad to use stdio in a signal handler */
  signal(sig, SIG_DFL);

  /* Don't bother with an alarm - user can just press ^c a second time */
}


static int
show_version(void)
{
  fprintf(stderr, "GNU Go Version %s\n", VERSION);

  return 1;
}

static int
show_help(void)
{
  fprintf(stderr, USAGE, 
	  DEPTH, BACKFILL_DEPTH, FOURLIB_DEPTH, KO_DEPTH, MEMORY, 
	  MIN_BOARD, MAX_BOARD, MAX_HANDICAP);
  return 1;
}


static int
show_copyright(void)
{
  fputs(COPYRIGHT, stderr);

  return 1;
}


#define USAGE_STYLE "\n\
Usage : gnugo [-opts]\n\
\n\
Play Style Options;\n\
   -p, --playstyle <style>      style of play\n\
   <style> can be:\n\
              standard          default opening\n\
              no_fuseki         minimal opening\n\
              tenuki            often plays tenuki in the opening\n\
              fearless          risky style of play\n\
              aggressive        both style tenuki and fearless\n\
"

static void set_style(char *arg)
{
  if (strncmp(arg, "standard", 5) == 0)
    style = STY_DEFAULT;
  else if (strncmp(arg, "no_fuseki", 5) == 0)
    style = STY_NO_FUSEKI;
  else if (strncmp(arg, "tenuki", 5) == 0)
    style = STY_TENUKI|STY_DEFAULT;
  else if (strncmp(arg, "fearless", 5) == 0)
    style = STY_FEARLESS|STY_DEFAULT;
  else if (strncmp(arg, "aggressive", 5) == 0)
    style = STY_FEARLESS|STY_TENUKI|STY_DEFAULT;
  else if (strncmp(arg, "help", 5) == 0) {
    fprintf(stderr, USAGE_STYLE);
    exit(EXIT_SUCCESS);
  } else {
    fprintf(stderr, "Invalid play style selection: %s\n", arg);
    fprintf(stderr, "Try `gnugo --help' for more information.\n");

    exit( EXIT_FAILURE);
  }
}


//int main(int argc, char *argv[])
void gnugo2()
{
  /*
  */
   int i, umove;
   enum mode playmode = MODE_UNKNOWN;
   enum testmode testmode = UNKNOWN_TESTMODE;

   char *outfile = NULL;
   char *until = NULL;

   char *printsgffile = NULL;

   char *infile = NULL;

   char decidestring[4];

   int benchmark = 0;  /* benchmarking mode (-b) */
   float komi = 0.0;

   int seed=0;           /* If seed is zero, GNU Go will play a different game 
			  each time. If it is set using -r, GNU Go will play the
			  same game each time. (Change seed to get a different
			  game). */


   /* Initialize the entire hashing system. */
   hash_init();

   /* Initialize the board. */
   init_board();
   
     /* Initialize the game info structure. */
   init_ginfo();
   init_gopt();

   /* Set some standard options. */
   umove=BLACK;
   
   /*
   
   */
   

   /* start random number seed */
   if (!seed) seed=time(0);
   srand(seed);
   set_seed(seed);

#if 0
Done in init_board()
   /* initialize moyo structure */
   init_moyo();
#endif

#if HASHING
  {
    float nodes;
    
    nodes = (float)( (memory * 1024 * 1024)
	     / (sizeof(Hashnode) + sizeof(Read_result) * 1.4));
    movehash = hashtable_new((int) (1.5 * nodes), (int)(nodes), (int)(nodes * 1.4) );
  } 
#endif

   /* clear some caches */
   clear_wind_cache();
   clear_safe_move_cache();
   
   if(!get_opt_quiet())
     {
       fprintf(stderr, "\n");
       show_version();
       show_copyright();
     }
   

  /*
  if (outfile)
   {

   if ((playmode != MODE_DECIDE_STRING) &&  (outfile))
     if (!sgf_open_file(outfile))
       {
	 fprintf(stderr, "Error: could not open '%s'\n", optarg);
	 exit(EXIT_FAILURE);
       }
   }

   switch(playmode) 
   {
     case MODE_GMP:     
       if(!sgf_root)
         sgfCreateHeaderNode(komi);
       play_gmp();
       break;
     case MODE_SOLO:
       if(!sgf_root)
         sgfCreateHeaderNode(komi);
       play_solo(benchmark);
       break;
     case MODE_TEST:    
       if (!sgf_root) {
	 fprintf(stderr, "You must use -l infile with test mode.\n");
	 exit(EXIT_FAILURE);
       }
       play_test(sgf_root, testmode);
       break;

     case MODE_LOAD_AND_ANALYZE:
       load_and_analyze_sgf_file(sgf_root, until, benchmark);
       break;

   case MODE_LOAD_AND_SCORE:
       if (!sgf_root) {
         fprintf(stderr, "gnugo: --score must be used with -l\n");
	 exit(EXIT_FAILURE);
       }
       load_and_score_sgf_file(sgf_root,until);
       break;
   case MODE_LOAD_AND_PRINT:
     if (!sgf_root) {
       fprintf(stderr, "gnugo: --printsgf must be used with -l\n");
       exit(EXIT_FAILURE);
     } else {
       int next;
       load_sgf_header(sgf_root);
       sgf_write_game_info(board_size, get_handicap(),get_komi(),get_seed(), "load and print"); 
       next = load_sgf_file(sgf_root, NULL);
       sgf_close_file();
       sgf_open_file(printsgffile);
       sgf_write_game_info(board_size, get_handicap(),get_komi(),get_seed(), "load and print"); 
       sgf_printboard(next);
     }
     break;
   case MODE_DECIDE_STRING:
     {
       int m, n;
       
       load_sgf_file(sgf_root, until);
       
       n = tolower(*decidestring) - 'a';
       if (tolower(*decidestring) >= 'i')
	 --n;
       m = get_boardsize() - atoi(decidestring+1);
       
       if (!infile) {
	 fprintf(stderr, "gnugo: --decidestring must be used with -l\n");
	 return (EXIT_FAILURE);
       }
       sgf_decidestring(m, n, outfile);
     }
     break;

   case MODE_ASCII_EMACS :  
     set_computer_player(OTHER_COLOR(umove));
     if(!sgf_root)
       sgfCreateHeaderNode(komi);
     else
       {
         load_sgf_header(sgf_root);
         set_computer_player(NONE);
       }
     play_ascii_emacs(infile);
     break;

   case MODE_ASCII :  
   default :     
     set_computer_player(OTHER_COLOR(umove));
     if(!sgf_root)
       sgfCreateHeaderNode(komi);
     else
       {
         load_sgf_header(sgf_root);
         set_computer_player(NONE);
       }
     play_ascii(infile);
     break;
   }
  */
     sgfCreateHeaderNode(komi);
     play_ascii(infile);
   
   //sgf_close_file();
   
   //if (analyzerfile&&sgf_root)
     //writesgf(sgf_root,analyzerfile,seed);
   
   //return 0;
}  /* end main */


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
