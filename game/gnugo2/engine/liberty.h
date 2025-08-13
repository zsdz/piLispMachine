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




#ifndef _LIBERTY_H_
#define _LIBERTY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



/* FIXME : as a temporary measure, until we get public / private interfaces
 * worked out, we adopt the convention that BUILDING_GNUGO_ENGINE is defined
 * by the source files making up the engine. Only these are allowed
 * access to all the internal functions, and write access to some of
 * the state variables.
 * One day, the public stuff will get moved into a separate file
 */

#ifdef BUILDING_GNUGO_ENGINE
#define PUBLIC_VARIABLE extern
#else
#define PUBLIC_VARIABLE extern const
#endif


/* some public macros everyone uses */

#define EMPTY 0
#define WHITE 1
#define BLACK 2
#define GRAY_BORDER 3
#define WHITE_BORDER 4
#define BLACK_BORDER 5
#define NONE 6               /* for use with is_computer_player */

#define MIN_BOARD 3          /* minimum supported board size        */
#define MAX_BOARD 21         /* maximum supported board size        */
#define MAX_HANDICAP 9      /* maximum supported handicap          */

#define OTHER_COLOR(color)  (WHITE+BLACK-(color))


#define DEAD 0
#define ALIVE 1
#define CRITICAL 2 
#define UNKNOWN 3
#define MAX_DRAGON_STATUS 4  /* used to size an array in matchpat.c */


/* aix defines abs in stdlib.h */

#ifdef abs
#undef abs
#endif

/* MS-VC defines max and min */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


#define abs(x) ((x) < 0 ? -(x) : (x))
#define line(x) (abs(x - 9))
#define max(a,b) ((a)<(b) ? (b) : (a))
#define min(a,b) ((a)<(b) ? (a) : (b))


/* This is the type used to store each piece on the board.  On a 486,
 * char is best, since the time taken to push and pop becomes
 * significant otherwise. On other platforms, an int may be better, if
 * memcpy() is particularly fast for example, or if character access is
 * very slow.  
 */

typedef unsigned char board_t;

/* public functions */

/* tracing/debugging fns */

void showboard(int xo);  /* ascii rep. of board to stdout */
void gprintf(const char *fmt, ...);  /* like fprintf(stderr, ...), but accepts %m */
void mprintf(const char *fmt, ...);  /* like fprintf(stdout, ...), but accepts %m */

#ifdef __GNUC__
/* gnuc allows variadic macros, so the tests can be done inline */
#define TRACE(fmt, args...)  do { if (verbose) gprintf(fmt, ##args); } while(0)
#define RTRACE(fmt, args...)  do { if (verbose >= 3) gprintf(fmt, ##args); } while(0)
#define VTRACE(fmt, args...)  do { if (verbose >= 4) gprintf(fmt, ##args); } while(0)
#define DEBUG(level, fmt, args...)  do { if ((debug & (level))) gprintf(fmt, ##args); } while(0)
#else
/* else we need fns to do these */
void TRACE(const char *fmt, ...);
void RTRACE(const char *fmt, ...);
void VTRACE(const char *fmt, ...);
void DEBUG(int level, const char *fmt, ...);
#endif


/* high-level routine to generate the best move for given color */
int genmove(int *i,int *j, int color);
int legal(int i, int j, int color);  /* can "color" play at i,j */
int sethand(int i);  /* fill board with handicap stones */
int updateboard(int i, int j, int color);  /* make a move and remove prisoners */
void remove_string(int i, int j);
void remove_stone(int i, int j);
void count_territory(int *white_territory, int *black_territory);
void evaluate_territory(int *white_territory, int *black_territory);
void save_state(void);
void restore_state(void);
int dragon_status(int i, int j);
void change_dragon_status(int x, int y, int status);

void who_wins(int color, float fkomi, FILE* stdwhat);

/* data concerning a dragon. A copy is kept at each stone of the string */

struct dragon_data {
  int color;    /* its color                                                  */
  int origini;  /* (origini,originj) is the origin of the string. Two         */
  int originj;  /* vertices are in the same dragon iff they have same origin. */
  int borderi;  /* if color=BLACK_BORDER or WHITE_BORDER the worm is a cavity */
  int borderj;  /* surrounded by the BLACK or WHITE dragon with origin (borderi,borderj) */
  int size;     /* size of the dragon          */
  int heyes;    /* the number of half eyes    */
  int heyei;    /* coordinates of a half eye */
  int heyej;
  int genus;    /* the number of eyes (approximately)        */
  int value;    /* if you have to ask you can't afford it  */
  int escape_route;      /* the maximum of worm[*][*].liberties4 over worms in the dragon  */
  int lunchi;           /* if lunchi != -1 then (lunchi,lunchj) points to a boundary worm */
  int lunchj;          /* which can be captured easily.                                  */
  int status;         /* (ALIVE, DEAD, UNKNOWN, CRITICAL)                               */
  int safety;        /* dragon is weak by the algorithm in moyo.c                      */
  int vitality;     /* a stronger measure of life potential than status (for semeai)  */
  int semeai;        /* true if a string is part of a semeai                           */
  int friendi;    /* nearby dragon to which we can connect                          */
  int friendj;    /*                                                               */
  int bridgei;    /* the move that connects to the friend                         */
  int bridgej;
};


#include "hash.h"


/* other modules get read-only access to these variables */

PUBLIC_VARIABLE board_t p[MAX_BOARD][MAX_BOARD];  /* go board */
PUBLIC_VARIABLE int ko_i;
PUBLIC_VARIABLE int ko_j;

PUBLIC_VARIABLE Hash_data  hashdata;

PUBLIC_VARIABLE unsigned char potential_moves[MAX_BOARD][MAX_BOARD];

PUBLIC_VARIABLE int board_size;            /* board size (usually 19) */
PUBLIC_VARIABLE int movenum;               /* movenumber */
PUBLIC_VARIABLE int black_captured, white_captured;   /* num. of black and white stones captured */
PUBLIC_VARIABLE int verbose;               /* bore the opponent */
PUBLIC_VARIABLE int debug;                 /* debug flags */
PUBLIC_VARIABLE int last_move_i;           /* The position of the last move */
PUBLIC_VARIABLE int last_move_j;           /* -""-                          */
PUBLIC_VARIABLE int terri_eval[3];
PUBLIC_VARIABLE int moyo_eval[3];
PUBLIC_VARIABLE struct dragon_data dragon[MAX_BOARD][MAX_BOARD];
PUBLIC_VARIABLE int hashflags;             /* hash flags */
PUBLIC_VARIABLE  struct hashtable  * movehash;
PUBLIC_VARIABLE  char *analyzerfile;
PUBLIC_VARIABLE  int style;

extern volatile int time_to_die;   /* set by signal handlers */

/* debug flag bits */
#define DEBUG_GENERAL 0x0001  /* NOTE : can specify -d0x... */
#define DEBUG_COUNT   0x0002
#define DEBUG_BOARD   0x0004
#define DEBUG_CAPTURE 0x0008
#define DEBUG_STACK   0x0010
#define DEBUG_WIND    0x0020
#define DEBUG_HELPER  0x0040
#define DEBUG_LOADSGF 0x0080
#define DEBUG_WORMS   0x0100
#define DEBUG_LADDER  0x0200
#define DEBUG_MATCHER 0x0400
#define DEBUG_DEFENDER 0x0800
#define DEBUG_ATTACKER 0x1000
#define DEBUG_BORDER   0x2000
#define DEBUG_DRAGONS  0x4000
#define DEBUG_SAVESGF  0x8000
#define DEBUG_HEY      0x10000
#define DEBUG_SEMEAI   0x20000
#define DEBUG_EYES     0x40000

/* hash flag bits */
#define HASH_FIND_DEFENSE 0x0001  /* NOTE : can specify -d0x... */
#define HASH_DEFEND1      0x0002
#define HASH_DEFEND2      0x0004
#define HASH_DEFEND3      0x0008
#define HASH_ATTACK       0x0010
#define HASH_ATTACK2      0x0020
#define HASH_ATTACK3      0x0040
#define HASH_DEFEND4      0x0080
#define HASH_ALL          0xffff

#define MEMORY 8

void init_board(void);
float get_komi(void);
int inc_movenumber(void);
int attack(int m, int n, int *i, int *j);

/******** style definitions ******/
#define STY_DEFAULT 		1	/* standard fuseki, default */
#define STY_NO_FUSEKI		2	/* no fuseki, just plays 4 corners */
#define STY_TENUKI		8    	/* more tenuki, compatible with default */
#define STY_FEARLESS		16	/* fearless, compatible with default */

#define BUILDING_GNUGO_ENGINE
#ifdef BUILDING_GNUGO_ENGINE

/* start of private stuff */

/* 
 * It is assumed in reading a ladder if stackp >= depth that
 * as soon as a bounding stone is in atari, the string is safe.
 * It is used similarly at other places in reading.c to implement simplifying
 * assumptions when stackp is large. DEPTH is the default value of depth.
 *
 * Unfortunately any such scheme invites the ``horizon effect.'' Increasing
 * DEPTH will make the program stronger and slower.
 *
 */

#define DEPTH 14
#define BACKFILL_DEPTH 9
#define FOURLIB_DEPTH 5
#define KO_DEPTH 8

#define FALSE_EYE 1
#define HALF_EYE 2
#define INHIBIT_CONNECTION 4


/* A string with n stones can have at most 2(n+1) liberties. From this
 * follows that an upper bound on the number of liberties of a string
 * on a board of size N^2 is 2/3 (N^2+1).
 */
#define MAXLIBS 2*(MAX_BOARD*MAX_BOARD + 1)/3
#define MAXSTACK 160
#define MAXCHAIN 160


/* board utility fns */

/* Really an interal fn, but exposed for efficiency.
 * Count up to maxlib liberties of stone at i,j, to globals lib and size.
 * Needs mx[][] initialised. Does not reset lib / size.
 */

int count(int i, int j, int color, char mx[MAX_BOARD][MAX_BOARD], int maxlib,
	  char mark);

void find_origin(int i, int j, int *origini, int *originj);
void chainlinks(int m, int n, int *adj, int adji[MAXCHAIN],
		int adjj[MAXCHAIN], int adjsize[MAXCHAIN],
		int adjlib[MAXCHAIN]);

int approxlib(int m, int n, int color, int maxlib);  /* count up to maxlib liberties at i,j to lib and size. */

#define countlib(i,j,c)  approxlib( (i), (j), (c), 9999) /* Helpful wrapper round approxlib() */

#define TRANSFORM(i,j,ti,tj,trans) \
do { \
  *ti = transformations[trans][0][0] * (i) + transformations[trans][0][1] * (j); \
  *tj = transformations[trans][1][0] * (i) + transformations[trans][1][1] * (j); \
} while(0)


/* save and restore state of board globals */
int pushgo(void);
int popgo(void); 
/* push stack then make a move (if legal). Otherwise stack unchanged. */
int trymove(int i, int j, int color, const char *message, int k, int l);
int tryko(int i, int j, int color, const char *message);
int trysafe(int i, int j, int color, const char *message);
void dump_stack(void);

void compile_for_match(void);  /* must be called once before using matchpat */

struct pattern; /* keep gcc happy */
/* try to match a pattern in the database to the board. Callback for each match */
typedef void (*matchpat_callback_fn_ptr)(int m, int n, int color, struct pattern *, int rotation);
void matchpat(int m, int n, matchpat_callback_fn_ptr callback, int color, int minwt, struct pattern *database);

int defend1(int si, int sj, int *i, int *j);
int attack2(int si, int sj, int *i, int *j);
int defend2(int si, int sj, int *i, int *j);
int defend3(int si, int sj, int *i, int *j);
int defend4(int si, int sj, int *i, int *j);
int attack3(int i, int j, int *ti, int *tj);
int attack4(int i, int j, int *ti, int *tj);
int break_chain(int si, int sj, int *i, int *j, int *k, int *l);
int break_chain2(int si, int sj, int *i, int *j);
int double_atari_chain2(int si, int sj, int *i, int *j);
int find_cap2(int m, int n, int *i, int *j);
int is_ko(int, int, int);
int singleton(int, int);
int confirm_safety(int, int, int, int);
void clear_safe_move_cache(void);
int safe_move(int i, int j, int color);
int naive_ladder(int si, int sj, int *i, int *j);
int safe_stone(int i, int j);
int make_worms(void);
void make_dragons(void);
void join_dragons(int i, int j, int m, int n);
void show_dragons(void);
void make_domains(void);
void find_neighbors(void);
int find_border(int m, int n, int *edge);
void propagate_worm(int m, int n);
int strategic_distance_to(int color, int i, int j);
int distance_to(int color, int i, int j);
int static_eval(int color);
int readnet3(int si, int sj, int *i, int *j);
int readnet2(int si, int sj, int *i, int *j);
void clear_wind_cache(void);
int wind_assistance(int i, int j, int color, int *params);
void testwind(int i, int j, int color, int *upower, int *mypower);
int moyo_assistance(int i, int j, int color, int *params);
int snapback(int si, int sj, int i, int j, int color); 
void transform(int i, int j, int *ti, int *tj, int trans);
void offset(int i, int j, int basei, int basej, int *ti, int *tj, int trans);
void find_cuts(void);
void find_connections(void);
void endgame(void);

/* various different strategies for finding a move */

int fuseki (int *i, int *j, int *val, int *equal_moves, int color);
int semeai (int *i, int *j, int *val, int *equal_moves, int color);
void small_semeai(void);
void small_semeai_analyzer(int, int, int, int);

int shapes(int *i, int *j, int *val, int *equal_moves, int color);
int defender(int *i, int *j, int *val, int *equal_moves, int color,
	     int shapei, int shapej);
int attacker(int *i, int *j, int *val, int *equal_moves, int color,
	     int shapei, int shapej);
int eye_finder(int *i, int *j, int *val, int *equal_moves, int color,
	       int shapei, int shapej);
int fill_liberty(int *i, int *j, int *val, int color);
int revise_semeai(int color);
int find_lunch(int m, int n, int *wi, int *wj, int *ai, int *aj);
int find_defense(int m, int n, int *ti, int *tj);
int string_value(int m, int n);
int connection_value(int ai, int aj, int bi, int bj, int ti, int tj);
void change_defense(int, int, int, int);
void change_attack(int, int, int, int);
int does_attack(int, int, int, int);
int does_defend(int, int, int, int);
int play_attack_defend_n(int color, int do_attack, int num_moves, ...);
int cut_possible(int i, int j, int color);

int liberty_of(int i, int j, int m, int n);
int defend_against(int ti, int tj, int color, int ai, int aj);

/* moyo functions */
void init_moyo(void);
int make_moyo(int color);
int delta_moyo(int ti, int tj,int color);
int delta_moyo_simple(int ti, int tj,int color);
int delta_terri(int ti, int tj,int color);
int diff_moyo(int ti, int tj,int color);
int average_moyo_simple(int ti, int tj,int color);
int diff_terri(int ti, int tj,int color);
void print_moyo(int color);
int area_stone(int m,int n);
int area_space(int m,int n);
int area_color(int m,int n);
int area_tag(int m,int n);
int meta_connect(int ti, int tj,int color);
void set_area_tag(int m,int n,int tag);
int moyo_color(int m,int n);
int terri_color(int m,int n);
int delta_terri_color(int ti,int tj, int color, int m, int n);
int delta_moyo_color(int ti,int tj, int color, int m, int n);
int delta_area_color(int ti,int tj, int color, int m, int n);
void search_big_move(int ti,int tj, int color, int val);
int number_weak(int color);

/* Eye space functions. */
int eye_space(int i, int j);
int proper_eye_space(int i, int j);
int marginal_eye_space(int i, int j);

/* Macro to help keeping track of the best move, with equal probability
   for all moves of the same highest value. */
#define BETTER_MOVE(new,best,q) ((best)>(new) ? 0 : (best) < (new) ? ((best)=(new),(q)=2) : rand()%((q)++) ? 0: 1 )

/* The macro is functionally equivalent to the following function:
 * 
 * int better_move(int new_value, int *best_value, int *equal_moves)
 * {
 *   if (*best_value > new_value)
 *     return 0;
 *   else if (*best_value < new_value) {
 *     *best_value = new;
 *     *equal_moves = 2;
 *     return 1;
 *   }
 *   else if (rand()%(*equal_moves) == 0) {
 *     (*equal_moves)++;
 *     return 1;
 *   }
 *   else {
 *     (*equal_moves)++;
 *     return 0;
 *   }
 * }
 *
 */

/* helper functions allow the pattern matcher to act smart */


/* debugging support */
void move_considered(int i, int j, int value);


/* FIXME : these are actually in sgf/sgf.h, but I consider them
 * to be part of the engine
 */

int sgf_open_file(const char *);
int sgf_write_game_info(int, int, float, int, const char *);
int sgf_write_line(const char *, ...);
int sgf_close_file(void);
int sgf_flush_file(void);
void begin_sgfdump(const char *filename);
void end_sgfdump(void);
void sgf_decidestring(int i, int j, const char *sgf_output);
void sgf_dragon_status(int, int, int );



/* globals variables */

extern int lib;                              /* current stone liberty FIXME : written by count() */
extern int libi[MAXLIBS], libj[MAXLIBS];     /* array of liberties found : filled by count() */
extern int size;                  /* cardinality of a group : written by count() */
extern int color_has_played;      /* color has placed a stone */
extern int stackp;                /* stack pointer */
extern int depth;                 /* deep reading cutoff */
extern int backfill_depth;        /* deep reading cutoff */
extern int fourlib_depth;         /* deep reading cutoff */
extern int ko_depth;
    
/* FIXME : perhaps some of the following should be flag-bits in debug */

extern int showstack;             /* debug stack pointer */
extern int showstatistics;        /* print statistics */
extern int allpats;               /* generate all patterns, even small ones */
extern int printworms;            /* print full data on each string */
extern int printmoyo;             /* print moyo board each move */
extern int printdragons;          /* print full data on each dragon */
extern int printboard;            /* print board each move */
extern int count_variations;      /* count (decidestring) */
extern int sgf_dump;              /* writing file (decidestring) */

struct stats_data {
  int  nodes;			  /* Number of visited nodes while reading */
  int  position_entered;	  /* Number of Positions entered. */
  int  read_result_entered;	  /* Number of Read_results entered. */
  int  position_hits;		  /* Number of hits of Positions. */
  int  read_result_hits;	  /* Number of hits of Read_results */
};

extern struct stats_data stats;

struct half_eye_data {
  int type;         /* HALF_EYE or FALSE_EYE; */
  int ki;           /* (ki,kj) is the move to kill or live */
  int kj;
};

extern struct half_eye_data half_eye[MAX_BOARD][MAX_BOARD];      /* array of half-eye data */

/* data concerning a worm. A copy is kept at each vertex of the worm */

struct worm_data {
  int color;         /* its color */
  int size;          /* its cardinality */
  int origini;       /* (origini,originj) is the origin of the string. Two */
  int originj;       /* vertices are in the same worm iff they have same origin. */
  int liberties;     /* number of liberties */
  int liberties2;    /* number of second order liberties */
  int liberties3;    /* third order liberties (empty vertices at distance 3) */
  int liberties4;    /* fourth order liberties */
  int attacki;       /* (attacki, attackj), if attacki != -1 points to move which captures */
  int attackj;       /* the string */
  int attack_code;   /* 1=unconditional win, 2 or 3=win with ko */
  int lunchi;        /* if lunchi != -1 then (lunchi,lunchj) points to a boundary worm */
  int lunchj;        /* which can be captured easily. */
  int defendi;       /* (defendi, defendj), if defendi != -1 points to a defensive move */
  int defendj;       /* protecting the string */
  int defend_code;   /* 1=unconditional win, 2 or 3=win with ko */
  int cutstone;      /* 1=potential cutting stone; 2=cutting stone */
  int genus;         /* number of connected components of the complement, less one */
  int value;         /* value of the worm */
  int ko;            /* 1=ko at this location */
  int inessential;   /* 1=inessential worm */
};

extern struct worm_data worm[MAX_BOARD][MAX_BOARD];

struct eye_data {
  int color;           /* BLACK, WHITE, BLACK_BORDERED, WHITE_BORDERED or GRAY_BORDERED   */
  int esize;           /* size of the eyespace                                           */
  int msize;           /* number of marginal vertices                                   */
  int origini;         /* The origin                                                   */
  int originj;         /*                                                             */
  int maxeye;          /* number of eyes if defender plays first                     */
  int mineye;          /* number of eyes if attacker plays first                    */
  int attacki;         /* vital point of attack                                    */
  int attackj;         /*                                                         */
  int dragoni;         /* origin of the surrounding dragon                       */
  int dragonj;
/* The above fields are constant on the whole eyespace. The following are not. */
  int marginal;             /* This vertex is marginal                        */
  int type;                 /* used to mark INHIBIT_CONNECTION               */
  int neighbors;            /* number of neighbors in eyespace              */
  int marginal_neighbors;   /* number of marginal neighbors                */
  int cut;                  /* Opponent can cut at vertex.                */
};

extern struct eye_data white_eye[MAX_BOARD][MAX_BOARD];
extern struct eye_data black_eye[MAX_BOARD][MAX_BOARD];

/* the following declarations have to be postponed until after the definition of struct eye_data */

void compute_eyes(int, int, int *, int *, int*, int *, struct eye_data eye[MAX_BOARD][MAX_BOARD]);
void propagate_eye (int, int, struct eye_data eye[MAX_BOARD][MAX_BOARD]);
void add_half_eye(int m, int n, struct eye_data eye[MAX_BOARD][MAX_BOARD]);
void retrofit_half_eye(int m, int n, int di, int dj);
void retrofit_genus(int m, int n, int step);

extern int distance_to_black[MAX_BOARD][MAX_BOARD];
extern int distance_to_white[MAX_BOARD][MAX_BOARD];
extern int strategic_distance_to_black[MAX_BOARD][MAX_BOARD];
extern int strategic_distance_to_white[MAX_BOARD][MAX_BOARD];
extern int black_domain[MAX_BOARD][MAX_BOARD];
extern int white_domain[MAX_BOARD][MAX_BOARD];


/* data concerning moyo */
extern int terri_test[3];
extern int moyo_test[3];
extern int very_big_move[3];

/* our own abort() which prints board state on the way out.
 * i,j is a "relevant" board position for info */
void abortgo(const char *file, int line, const char *msg, int i, int j);

#ifndef NDEBUG
#define ASSERT(x,i,j)  if (x) ; else abortgo(__FILE__, __LINE__, #x,i,j)   /* avoid dangling else */
#else
#define ASSERT(x,i,j)
#endif

#endif  /* BUILDING_GNUGO_ENGINE */

#endif


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
