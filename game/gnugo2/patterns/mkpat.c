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






/* Compile the patterns, halfeyes, or connections database. This
 * produces patterns.c or conn.c
 */

/* See also patterns.h, patterns.db and conn.db. */


/* Differences when compiling half-eye patterns (-h) :
 *  'h' is allowed (posn of eye)
 *  'a' is allowed (empty and X can play there).
 *  'h' will always be written as the first element
 */

/* Differences when compiling connections patterns (-c) :
 *  '*' means cutting point
 *  '!' is allowed (inhibit connection there).
 *  '!' will always be written as the first elements
 */

/* As in the rest of gnugo, co-ordinate convention (i,j) is 'i' down from
 * the top, then 'j' across from the left
 */

#define MAX_BOARD 19
#define USAGE "\
Usage : mkpat [-cvh] <prefix>\n\
 options : -v = verbose\n\
           -h = compile half-eye database (default is pattern database)\n\
           -c = compile connections database (default is pattern database)\n\
"


#define PATTERNS 0
#define HALFEYES 1
#define CONNECTIONS 2


#define MAXLINE 500
#define MAXCONSTRAINT 10000
#define MAXPATNO 2000
#define MAXLABELS 20
#define MAXPARAMS 9

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "patterns.h"
#include "getopt.h"


#define DEBUG(x)  /* printf x */


/* valid characters that can appear in a pattern
 * position in string is att value to store
 */
const char VALID_PATTERN_CHARS[]     = ".XOxoha!*?";
const char VALID_EDGE_CHARS[]        = "+-|";
const char VALID_CONSTRAINT_LABELS[] = "abcdefghijklmnpqrstuvwyzABCDEFGHIJKLMNPQRSTUVWYZ";


/* the offsets into the list are the ATT_* defined in patterns.h
 * The following defns are for internal use only, and are not
 * written out to the compiled pattern database
 */

#define ATT_star  8
#define ATT_wild  9


/* stuff used in reading/parsing pattern rows */
int maxi, maxj;                 /* (i,j) offsets of largest element */
int mini, minj;                 /* offset of top-left element
				   (0,0) unless there are edge constraints */
int where;                      /* NORTH | WEST, etc */
int el;                         /* next element number in current pattern */
struct patval elements[360];    /* elements of current pattern */
int num_stars;

int ci=-1,cj=-1;                /* positionn of origin (first piece element)
				   relative to top-left */
int patno;		        /* current pattern */
int pats_with_constraints = 0;  /* just out of interest */
int label_coords[256][2];       /* Coordinates for labeled stones in the 
				   autohelper patterns. */
int current_i;		        /* Counter for the line number of a 
				   constraint diagram. */
char constraint[MAXCONSTRAINT]; /* Store constraint lines. */


/* stuff to maintain info about patterns while reading */
struct pattern pattern[MAXPATNO];  /* accumulate the patterns into here */
char pattern_names[MAXPATNO][80];  /* with optional names here, */
char helper_fn_names[MAXPATNO][80]; /* helper fn names here */
char autohelper_code[MAXPATNO*300]; /* code for automatically generated */
                                    /* helper functions here */
char *code_pos;                     /* current position in code buffer */

struct autohelper_func {
  const char *name;
  int params;
  const char *code;
};

static struct autohelper_func autohelper_functions[] = {
  {"lib2",            1, "worm[%ci][%cj].liberties2"},
  {"lib3",            1, "worm[%ci][%cj].liberties3"},
  {"lib4",            1, "worm[%ci][%cj].liberties4"},
  {"lib",             1, "worm[%ci][%cj].liberties"},
  {"alive",           1, "(dragon[%ci][%cj].status == ALIVE)"},
  {"unknown",         1, "(dragon[%ci][%cj].status == UNKNOWN)"},
  {"critical",        1, "(dragon[%ci][%cj].status == CRITICAL)"},
  {"dead",            1, "(dragon[%ci][%cj].status == DEAD)"},
  {"status",          1, "dragon[%ci][%cj].status"},
  {"ko",              1, "worm[%ci][%cj].ko"},
  {"xdefend_against", 2, "defend_against(%ci,%cj,OTHER_COLOR(color),%ci,%cj)"},
  {"odefend_against", 2, "defend_against(%ci,%cj,color,%ci,%cj)"},
  {"does_defend",     2, "does_defend(%ci,%cj,%ci,%cj)"},
  {"does_attack",     2, "does_attack(%ci,%cj,%ci,%cj)"},
  {"attack",          1, "(worm[%ci][%cj].attacki != -1)"},
  {"defend",          1, "(worm[%ci][%cj].defendi != -1)"},
  {"weak",            1, "(dragon[%ci][%cj].safety==CRITICAL)"},
  {"safe_xmove",      1, "safe_move(%ci,%cj,OTHER_COLOR(color))"},
  {"safe_omove",      1, "safe_move(%ci,%cj,color)"},
  {"xmoyo",           1, "(moyo_color(%ci,%cj) == OTHER_COLOR(color))"},
  {"omoyo",           1, "(moyo_color(%ci,%cj) == color)"},
  {"xarea",           1, "(area_color(%ci,%cj) == OTHER_COLOR(color))"},
  {"oarea",           1, "(area_color(%ci,%cj) == color)"},
  {"xterri",          1, "(terri_color(%ci,%cj) == OTHER_COLOR(color))"},
  {"oterri",          1, "(terri_color(%ci,%cj) == color)"},
  {"genus",           1, "dragon[%ci][%cj].genus"},
  {"cutstone",        1, "worm[%ci][%cj].cutstone"},
  {"xlib",            1, "countlib(%ci,%cj,OTHER_COLOR(color))"},
  {"olib",            1, "countlib(%ci,%cj,color)"},
  {"xcut",            1, "cut_possible(%ci,%cj,OTHER_COLOR(color))"},
  {"ocut",            1, "cut_possible(%ci,%cj,color)"},
  {"xplay_defend",   -1, "play_attack_defend_n(OTHER_COLOR(color), 0, %d"},
  {"oplay_defend",   -1, "play_attack_defend_n(color, 0, %d"},
  {"xplay_attack",   -1, "play_attack_defend_n(OTHER_COLOR(color), 1, %d"},
  {"oplay_attack",   -1, "play_attack_defend_n(color, 1, %d"},
  {"seki_helper",     1, "seki_helper(%ci,%cj)"},
  {"diff_moyo",       1, "diff_moyo(%ci,%cj,color)"},
  {"area_stone",      1, "area_stone(%ci,%cj)"},
  {"area_space",      1, "area_space(%ci,%cj)"},
  {"eye",             1, "eye_space(%ci,%cj)"},
  {"proper_eye",      1, "proper_eye_space(%ci,%cj)"},
  {"marginal_eye",    1, "marginal_eye_space(%ci,%cj)"}
};


/* To get a valid function pointer different from NULL. */
static int
dummyhelper (struct pattern *patt, int transformation,
	     int ti, int tj, int color)
{
  UNUSED(patt); UNUSED(transformation); UNUSED(ti); UNUSED(tj); UNUSED(color);
  return 0;
}


#define CONN_PREAMBLE "\
/* This file is automatially generated by mkpat. Do not\n\
edit it directly. Instead, edit the connection database. */\n\
\n\
#include <stdio.h> /* for NULL */\n\
#include \"liberty.h\"\n\
#include \"patterns.h\"\n\
"

#define PAT_PREAMBLE "\
/* This file is automatially generated by mkpat. Do not\n\
edit it directly. Instead, edit the pattern database. */\n\
\n\
#include <stdio.h> /* for NULL */\n\
#include \"liberty.h\"\n\
#include \"patterns.h\"\n\
"



int fatal_errors = 0;

/* options */
int verbose = 0;  /* -v */
int pattern_type = PATTERNS;  /* -h for HALFEYES, -c for CONNECTIONS */



/* table for use by TRANSFORM() (patterns.h) */

/* FIXME : copied from matchpat.c : please put it somewhere so we can share one copy */

const int transformations[8][2][2] = {
  {{ 1,  0}, { 0,  1}}, /* linear transformation matrix */
  {{ 0,  1}, {-1,  0}}, /* rotate 90 */
  {{-1,  0}, { 0, -1}}, /* rotate 180 */
  {{ 0, -1}, { 1,  0}}, /* rotate 270 */
  {{ 0, -1}, {-1,  0}}, /* rotate 90 and invert */
  {{-1,  0}, { 0,  1}}, /* flip left */
  {{ 0,  1}, { 1,  0}}, /* rotate 90 and flip left */
  {{ 1,  0}, { 0, -1}}  /* invert */
};




/**************************
 *
 * stuff to parse the input
 *
 **************************/

/* reset state of all pattern variables */
static void
reset_pattern(void)
{
  int i;

  maxi = 0;
  maxj = 0;
  ci = -1;
  cj = -1;
  where = 0;
  el = 0;
  num_stars = 0;
  strcpy(helper_fn_names[patno], "NULL");
  for(i=0; i<256; i++)
    label_coords[i][0] = -1;
  current_i = 0;
  constraint[0] = 0;
}
  


/* this is called to compute the extents of the pattern, applying
 * edge constraints as necessary
 */

static void
find_extents(void)
{

  /* When this is called, elements go from (mini,minj) inclusive to
   * (maxi-1, maxj-1) [ie exclusive]. Make them inclusive.
   * ie maximum element lies on (maxi,maxj)
   */

  --maxi;
  --maxj;

  /* apply edge constraints to the size of the pattern */

  if (where & (NORTH|SOUTH|EAST|WEST))
    ++pats_with_constraints;

  if (verbose)
    fprintf(stderr, "Pattern %s has constraints 0x%x\n",
	    pattern_names[patno], where);


#if 0
  /* where is a combination of NORTH's, EASTS, etc.
   * Don't forget the case where the pattern might
   * already be the full-width of the board. I think
   * this order works
   */

  if (where & WEST) {
    /* We are extending the pattern out to the
     * right.
     */
    maxj = MAX_BOARD-1;  /* = minj + MAX_BOARD-1, but minj is 0 */
  }

  if (where & EAST) {
    /* pattern is constrained to lie at right edge.
     * So minj must be extended -ve such that
     * maxj - minj = MAX_BOARD-1.
     */
    minj = maxj - MAX_BOARD-1;
  }

  if (where & NORTH)
    maxi = MAX_BOARD-1; /* extend to the bottom */

  if (where & SOUTH)
    mini = maxi - MAX_BOARD-1;  /* extend to the top */

#endif

  pattern[patno].edge_constraints = where;


  /* At this point, (mini,minj) -> (maxi,maxj) contain
   * the extent of the pattern, relative to top-left
   * of pattern, rather than (ci,cj).
   *
   * But we store them in the output file relative
   * to (ci,cj), so that we can transform the corners
   * of the pattern like any other relative co-ord.
   */

  pattern[patno].mini = mini - ci;
  pattern[patno].minj = minj - cj;
  pattern[patno].maxi = maxi - ci;
  pattern[patno].maxj = maxj - cj;
}


/* For good performance, we want to reject patterns as quickly as
 * possible. For each pattern, this combines 16 positions around
 * the anchor stone into a 32-bit mask and value. In the matcher,
 * the same 4x4 grid is precomputed, and then we can quickly
 * test 16 board positions with one test.
 * See matchpat.c for details of how this works - basically, if
 * we AND what is on the board with the and_mask, and get the
 * value in the val_mask, we have a match. This test can be
 * applied in parallel : 2 bits per posn x 16 posns = 32 bits.
 * "Don't care" has and_mask = val_mask = 0, which is handy !
 */

static void
compute_grids(void)
{
#ifdef GRID_OPT
  /*                       element : .  X  O  x  o  h  a  ! */
  static const uint32 and_mask[] = { 3, 3, 3, 1, 2, 1, 3, 1 };
  static const uint32 val_mask[] = { 0, 2, 1, 0, 0, 0, 0, 0 };

  int ll;  /* iterate over rotations */
  int k;   /* iterate over elements */

  for (ll=0; ll < 8; ++ll) {
    for (k=0; k<el; ++k) {
      int di, dj;

      TRANSFORM(elements[k].x - ci, elements[k].y - cj, &di, &dj, ll);
      ++di;
      ++dj;
      if (di >= 0 && di < 4 && dj >= 0 && dj < 4) {
	pattern[patno].and_mask[ll]
	  |= and_mask[elements[k].att] << (30 - di * 8 - dj * 2);
	pattern[patno].val_mask[ll]
	  |= val_mask[elements[k].att] << (30 - di * 8 - dj * 2);
      }
    }
  }
#endif
}



/* We've just read a line that looks like a pattern line.
 * Now process it.
 */

static void
read_pattern_line(char *p)
{
  const char *char_offset;
  int j;
  int width;

  if (where & SOUTH)
    /* something wrong here : pattern line after a SOUTH constraint */
    goto fatal;


  if (*p == '+' || *p == '-') {
    /* must be a north/south constraint */

    if (maxi == 0)
      where |= NORTH;
    else
      where |= SOUTH;

    if (*p == '+') {
      if (maxi > 0 && !(where & WEST))
	/* if this is end of pattern, must already know about west */
	goto fatal;

      where |= WEST;
      ++p;
    }

    /* count the number of -'s */
    for (width=0; *p == '-' ; ++p, ++width)
      ;

    if (width == 0)
      goto fatal;

    if (*p == '+') {
      if (maxi > 0 && !(where & EAST))
	/* if this is end of pattern, must already know about west */
	goto fatal;
      where |= EAST;
    }

    if (maxi > 0 && width != maxj)
      goto notrectangle;

    return;
  }

  /* get here => its a real pattern entry, 
   * rather than a north/south constraint 
   */

  /* we have a pattern line - add it into the current pattern */
  if (*p == '|') {
    /* if this is not the first line, or if there is a north
     * constraint, we should already know about it
     */
    if (!(where & WEST) && ((where & NORTH) || maxi > 0))
      /* we should already know about this constraint */
      goto fatal;

    where |= WEST;
    ++p;
  }
  else if (where & WEST)
    /* need a | if we are already constrained to west */
    goto fatal;


  for (j=0; 
       (char_offset = strchr(VALID_PATTERN_CHARS, *p)) != NULL;
       ++j, ++p) {

    /* char_offset is a pointer within the VALID_PATTERN_CHARS string.
     * so  (char-VALID_PATTERN_CHARS) is the att (0 to 11) to write to the
     * pattern element
     */

    /* one of ATT_* - see above */
    int off = char_offset - VALID_PATTERN_CHARS;

    if (off == ATT_wild)
      continue;  /* boring - pad character */

    if (off >= ATT_h && off <= ATT_a && pattern_type != HALFEYES)
      goto fatal;

    if (off == ATT_not && pattern_type != CONNECTIONS)
      goto fatal;

    if (off == ATT_star) {
      /* '*' */
      pattern[patno].movei = maxi;
      pattern[patno].movej = j;
      ++num_stars;
      off = ATT_dot;  /* add a '.' to the pattern instead */
    }

    assert(off <= ATT_not);

	
    if ((ci==-1) && (off == ATT_O
		     || (off == ATT_X && pattern_type == PATTERNS))) {
      /* use this posn as the pattern origin - only 'O' for halfeye or
       * connection mode
       */
      ci=maxi;
      cj=j;
      pattern[patno].anchored_at_X = (off == ATT_X) ? 3 : 0;
    }

    elements[el].x = maxi;
    elements[el].y = j;
    elements[el].att = off;  /* '*' mapped to .  above*/
    ++el;
  }

  if (*p == '|') {

    /* if this is not the first line, or if there is a north
     * constraint, we should already know about it
     */
    if (!(where & EAST) && ((where & NORTH) || maxi > 0))
      goto fatal;  /* we should already know about this constraint */

    where |= EAST;

  }
  else if (where & EAST)
    goto fatal;  /* need a | if we are already constrained to east */


  if (maxi > 0 && j != maxj)
    goto notrectangle;

  if (j > maxj)
    maxj = j;

  maxi++;

  return;

fatal:
 fprintf(stderr, "Illegal pattern %s\n", pattern_names[patno]);
 fatal_errors=1;
 return;

notrectangle:
 fprintf(stderr, "Warning pattern %s not rectangular\n", pattern_names[patno]);
 return;
}


/*
 * We've just read a line that looks like a constraint pattern line.
 * Now process it.
 */

static void
read_constraint_diagram_line(char *p)
{
  int j;

  /* North or south boundary, no letter to be found. */
  if (*p == '+' || *p == '-')
    return;

  /* Skip west boundary. */
  if (*p == '|')
    p++;
  
  for (j=0; 
       strchr(VALID_PATTERN_CHARS, *p) 
	 || strchr(VALID_CONSTRAINT_LABELS, *p);
       ++j, ++p) {
    if (strchr(VALID_CONSTRAINT_LABELS, *p) 
	&& label_coords[(int)*p][0] == -1) {

      /* New constraint letter */
      label_coords[(int)*p][0] = current_i;
      label_coords[(int)*p][1] = j;
    }
  }

  current_i++;

  return;
}



/* On reading a line starting ':', finish up and write
 * out the current pattern 
 */

static void
finish_pattern(char *line)
{
  /* end of pattern layout */

  char symmetry;		/* the symmetry character */
  int max_assistance = 0;	/* maximum value from assistance */
  
  mini = minj = 0; /* initially : can change with edge-constraints */

  if (num_stars > 1 || (pattern_type == PATTERNS && num_stars == 0)) {
    fprintf(stderr, "No or too many *'s in pattern %s\n",
	    pattern_names[patno]);
    fatal_errors=1;
  }

  if (ci == -1 || cj == -1) {
    fprintf(stderr, "No origin for pattern %s\n", pattern_names[patno]);
    fatal_errors=1;
    ci=0; cj=0;
  }

  /* translate posn of * to relative co-ords
   */

  if (num_stars == 1) {
    pattern[patno].movei -= ci;
    pattern[patno].movej -= cj;
  }
  else if (num_stars == 0) {
    pattern[patno].movei = -1;
    pattern[patno].movej = -1;
  }

  find_extents();

  compute_grids();

  /* now read in the weights of the line
   * It is not necessary for all the values to be given,
   * but at least two fields must be supplied.
   * The compiler guarantees that all the fields are already
   * initialised to 0.
   */

  pattern[patno].patlen = el;

  {
    int s;
    char class[32];
    char *p = line;
    int n;

    class[0] = 0;  /* in case sscanf doesn't get that far */
    s = sscanf(p, ":%c%n", &symmetry, &n);
    
    if (pattern_type == PATTERNS && s == 1) {
      p += n;
      s += sscanf(p, ",%d,%n", &pattern[patno].patwt, &n);
      if (s == 2)
	p += n;
      pattern[patno].assistance = NO_ASSISTANCE;
      if (strncmp(p,"wind",4) == 0) {
	int ucutoff;
	int uvalue;
	int mycutoff;
	int myvalue;
	pattern[patno].assistance = WIND_ASSISTANCE;
	p += 4;
	sscanf(p, "(%d,%d,%d,%d),%n", &ucutoff, &uvalue, &mycutoff,
	       &myvalue, &n);
	p += n;
	printf("static int assist_params%d[] = {%d,%d,%d,%d};\n\n", patno,
	       ucutoff, uvalue, mycutoff, myvalue);
	max_assistance = ucutoff*uvalue + mycutoff*myvalue;
      }
      else if (strncmp(p,"moyo",4) == 0) {
	int moyocutoff;
	int moyovalue;
	pattern[patno].assistance = MOYO_ASSISTANCE;
	p += 4;
	sscanf(p, "(%d,%d),%n", &moyocutoff, &moyovalue, &n);
	p += n;
	printf("static int assist_params%d[] = {%d,%d};\n\n", patno,
	       moyocutoff, moyovalue);
	max_assistance = moyocutoff * moyovalue;
      }
      else {
	sscanf(p, "%*[^,],%n", &n);
	p += n;
      }
      
      s += sscanf(p, "%[^,],%d,%d,%d,%d,%d,%s",
		  class,
		  &pattern[patno].obonus,
		  &pattern[patno].xbonus,
		  &pattern[patno].splitbonus,
		  &pattern[patno].minrand,
		  &pattern[patno].maxrand,
		  helper_fn_names[patno]);
    }
    else if (pattern_type == CONNECTIONS && s==1) {
      p += n;
      s += sscanf(p, ",%[^,],%s",
		  class,
		  helper_fn_names[patno]);
    }      
    
    {
      if (strchr(class,'s')) pattern[patno].class |= CLASS_s;
      if (strchr(class,'O')) pattern[patno].class |= CLASS_O;
      if (strchr(class,'o')) pattern[patno].class |= CLASS_o;
      if (strchr(class,'X')) pattern[patno].class |= CLASS_X;
      if (strchr(class,'x')) pattern[patno].class |= CLASS_x;
      if (strchr(class,'D')) pattern[patno].class |= CLASS_D;
      if (strchr(class,'C')) pattern[patno].class |= CLASS_C;
      if (strchr(class,'n')) pattern[patno].class |= CLASS_n;
      if (strchr(class,'B')) pattern[patno].class |= CLASS_B;
      if (strchr(class,'A')) pattern[patno].class |= CLASS_A;
      if (strchr(class,'L')) pattern[patno].class |= CLASS_L;
    }

    switch (pattern_type) {
    case PATTERNS:
      if (s < 2) {
	fprintf(stderr, ": line must contain weight\n");
	++fatal_errors;
      }
      break;
    case HALFEYES:
      if (s != 1) {
	fprintf(stderr, ": line must contain only symmetry in -h mode\n");
	++fatal_errors;
      }
      break;
    case CONNECTIONS:
      if (s < 2) {
	fprintf(stderr, ": line must contain class in -c mode\n");
	++fatal_errors;
      }
      break;
    }      
  }

      
  /* Now get the symmetry. There are extra checks we can make to do with
   * square-ness and edges. We do this before we work out the edge constraints,
   * since that mangles the size info.
   */
  
  switch(symmetry) {
  case '+' :
    if (where & (NORTH|EAST|SOUTH|WEST))
      fprintf(stderr,
	      "Warning : symmetry inconsistent with edge constraints (pattern %s)\n", 
	      pattern_names[patno]);
    pattern[patno].trfno = 2;
    break;

  case 'X' : 
    if (where & (NORTH|EAST|SOUTH|WEST))
      fprintf(stderr,
	      "Warning : X symmetry inconsistent with edge constraints (pattern %s)\n", 
	      pattern_names[patno]);
    if (maxi != maxj)
      fprintf(stderr,
	      "Warning : X symmetry requires a square pattern (pattern %s)\n",
	      pattern_names[patno]);
    pattern[patno].trfno = 2;
    break;

  case '-' :
    if (where & (NORTH|SOUTH))
      fprintf(stderr,
	      "Warning : symmetry inconsistent with edge constraints (pattern %s)\n", 
	      pattern_names[patno]);
    pattern[patno].trfno = 4;
    break;
    
  case '|' :
    if (where & (EAST|WEST))
      fprintf(stderr,
	      "Warning : symmetry inconsistent with edge constraints (pattern %s)\n", 
	      pattern_names[patno]);
    pattern[patno].trfno = 4;
    break;

  case '\\' :
  case '/' :
    /* FIXME : can't be bothered putting in the edge tests */
    if (maxi != maxj)
      fprintf(stderr,
	      "Warning : \\ or / symmetry requires a square pattern (pattern %s)\n", 
	      pattern_names[patno]);

    pattern[patno].trfno = 4;
    break;

  default:
    fprintf(stderr,
	    "Warning : symmetry character '%c' not implemented - using '8' (pattern %s)\n", 
	    symmetry, pattern_names[patno]);
    /* FALLTHROUGH */
  case '8' :
    pattern[patno].trfno = 8;
    break;
  }

  if (pattern_type == PATTERNS) {
    pattern[patno].maxwt = pattern[patno].patwt + max_assistance 
      + pattern[patno].splitbonus + pattern[patno].maxrand;
    if (pattern[patno].obonus > 0)
      pattern[patno].maxwt += pattern[patno].obonus;
    if (pattern[patno].xbonus > 0)
      pattern[patno].maxwt += pattern[patno].xbonus;
  }
}
      

static void
read_constraint_line(char *line)
{
  /* Avoid buffer overrun. */
  assert(strlen(constraint) + strlen(line) < MAXCONSTRAINT);

  /* Append the new line. */
  strcat(constraint, line);
}


static void
generate_autohelper_code(int funcno, int number_of_params, int *labels)
{
  int i;
  
  if (autohelper_functions[funcno].params >= 0) {
    switch (number_of_params) {
    case 1:
      code_pos += sprintf(code_pos, autohelper_functions[funcno].code,
			  labels[0], labels[0]);
      break;
    case 2:
      code_pos += sprintf(code_pos, autohelper_functions[funcno].code,
			  labels[0], labels[0],
			  labels[1], labels[1]);
      break;
    case 3:
      code_pos += sprintf(code_pos, autohelper_functions[funcno].code,
			  labels[0], labels[0],
			  labels[1], labels[1],
			  labels[2], labels[2]);
    }
    return;
  }
  
  /* This is a very special case where there is assumed to be a
   * variable number of parameters and these constitute a series of
   * moves to make followed by a final attack or defense test.
   */
  code_pos += sprintf(code_pos, autohelper_functions[funcno].code,
		      number_of_params-1);
  for (i=0; i<number_of_params; i++) 
    code_pos += sprintf(code_pos, ", %ci, %cj", labels[i], labels[i]);
  code_pos += sprintf(code_pos, ")");
}


/* Finish up a constraint and generate the automatic helper code. 
 * The constraint text is in the global variable constraint. 
 */

static void
finish_constraint(void)
{
  unsigned int i;

  /* Mark that this pattern has an autohelper. */
  pattern[patno].autohelper = dummyhelper;
  
  /* Generate autohelper function declaration. */
  code_pos += sprintf(code_pos, "static int\nautohelper%d (struct pattern *patt, int transformation, int ti, int tj, int color)\n{\n  int", patno);

  /* Generate variable declarations. */
  for (i=0; i<sizeof(VALID_CONSTRAINT_LABELS); i++) {
    int c = (int) VALID_CONSTRAINT_LABELS[i];

    if (label_coords[c][0] != -1)
      code_pos += sprintf(code_pos, " %ci, %cj,", c, c);
  }

  /* Replace the last ',' with ';' */
  if (*(code_pos-1) != ',') {
    fprintf(stderr,
	    "Warning, no label has been specified in constraint (pattern %s).\n", 
	    pattern_names[patno]);
    return;
  }
  *(code_pos-1) = ';';

  /* Include UNUSED statements for two parameters */
  code_pos += sprintf(code_pos, "\n  UNUSED(patt);\n  UNUSED(color);\n");
  
  /* Generate coordinate transformations. */
  for (i=0; i<sizeof(VALID_CONSTRAINT_LABELS); i++) {
    int c = (int) VALID_CONSTRAINT_LABELS[i];

    if (label_coords[c][0] != -1)
      code_pos += sprintf(code_pos,
			  "\n  offset(%d, %d, ti, tj, &%ci, &%cj, transformation);",
			  label_coords[c][0] - ci - pattern[patno].movei,
			  label_coords[c][1] - cj - pattern[patno].movej,
			  c, c);
  }

  code_pos += sprintf(code_pos, "\n\n  return ");
  /* Parse the constraint and generate the corresponding helper code. */
  /* We use a small state machine. */
  {
    int state=0;
    char *p;
    int n=0;
    int label = 0;
    int labels[MAXLABELS];
    int N = sizeof(autohelper_functions)/sizeof(struct autohelper_func);
    int number_of_params = 0;
    for (p=constraint; *p; p++)
    {
      switch (state) {
      case 0: /* Looking for a token, echoing other characters. */
	for (n=0; n<N; n++) {
	  if (strncmp(p, autohelper_functions[n].name,
		      strlen(autohelper_functions[n].name)) == 0) {
	    state=1;
	    p += strlen(autohelper_functions[n].name)-1;
	    break;
	  }
	}
	if(state==0 && *p!='\n')
	  *(code_pos++) = *p;
	break;
	
      case 1: /* Token found, now expect a '('. */
	if (*p != '(') {
	  fprintf(stderr,
		  "Syntax error in constraint, '(' expected (pattern %s).\n", 
		  pattern_names[patno]);
	  return;
	}
	else {
	  assert(autohelper_functions[n].params <= MAXPARAMS);
	  number_of_params = 0;
	  state = 2;
	}
	break;

      case 2: /* Time for a label. */
	if ((*p != '*') && !strchr(VALID_CONSTRAINT_LABELS, *p)) {
	  if (strchr("XxOo", *p))
	    fprintf(stderr,
		    "Error, '%c' is not allowed as a constraint label (pattern %s).\n",
		    *p, pattern_names[patno]);
	  else
	    fprintf(stderr,
		    "Syntax error in constraint, label expected (pattern %s).\n", 
		    pattern_names[patno]);
	  return;
	}
	else {
	  if (*p == '*')
	    label = (int) 't';
	  else {
	    label = (int) *p;
	    if (label_coords[label][0] == -1) {
	      fprintf(stderr,
		      "Error, The constraint uses a label (%c) that wasn't specified in the diagram (pattern %s).\n", 
		      label, pattern_names[patno]);
	      return;
	    }
	  }
	  labels[number_of_params] = label;
	  number_of_params++;
	  state = 3;
	}
	break;

      case 3: /* A ',' or a ')'. */
	if (*p == ',') {
	  if ((autohelper_functions[n].params >= 0)
	      && (number_of_params == autohelper_functions[n].params)) {
	    fprintf(stderr,
		    "Syntax error in constraint, ')' expected (pattern %s).\n",
		    pattern_names[patno]);
	    return;
	  }
	  if (number_of_params == MAXPARAMS) {
	    fprintf(stderr,
		    "Error in constraint, too many parameters. (pattern %s).\n", 
		    pattern_names[patno]);
	    return;
	  }
	  state = 2;
	  break;
	}
	else if (*p != ')') {
	  fprintf(stderr, 
		  "Syntax error in constraint, ',' or ')' expected (pattern %s).\n", 
		  pattern_names[patno]);
	  return;
	}
	else { /* a closing parenthesis */
	  if ((autohelper_functions[n].params >= 0)
	      && (number_of_params < autohelper_functions[n].params)) {
	    fprintf(stderr,
		    "Syntax error in constraint, ',' expected (pattern %s).\n",
		    pattern_names[patno]);
	    return;
	  }
	  generate_autohelper_code(n, number_of_params, labels);
	  state = 0;
	}
	break;

      default:
	fprintf(stderr,
		"Internal error in finish_constraint(), unknown state.\n");
	return;
      }
    }
  }
  code_pos += sprintf(code_pos, ";\n}\n\n");
  
  /* Check that we have not overrun our buffer. That would be really bad. */
  assert(code_pos <= autohelper_code + sizeof(autohelper_code));
}



/* ================================================================ */
/*           stuff to write the elements of a pattern               */
/* ================================================================ */



/* callback function used to sort the elements in a pattern. 
 * We want to sort the patterns by attribute.
 *
 *  RANK 01234567
 *  ATT  57126340
 *       h!XOaxo.
 * 
 * so that cheaper / more specific tests are done early on
 * For halfeyes mode, the half-eye (5) must be first. (So that
 * code can find it easily !)
 * For connections mode, the cutting point (7) and inhibition points (8)
 * must be first.
 */

static int
compare_elements(const void *a, const void *b)
{
  static char order[] = {7,2,3,5,6,0,4,1};  /* score for each attribute */
  return  order[((const struct patval *)a)->att]
    - order[((const struct patval *)b)->att];
}



/* flush out the pattern stored in elements[]. Don't forget
 * that elements[].{x,y} and min/max{i,j} are still relative
 * to the top-left corner of the original ascii pattern, and
 * not relative to the anchor stone ci,cj
 */

static void
write_elements(char *name)
{
  int node;

  assert(ci != -1 && cj != -1);

  /* sort the elements so that least-likely elements are tested first. */
  qsort(elements, el, sizeof(struct patval), compare_elements);

  printf("static struct patval %s%d[]={\n",name,patno);
  for (node=0;node < el; node++) {
    assert(elements[node].x >= mini && elements[node].y >= minj);
    assert(elements[node].x <= maxi && elements[node].y <= maxj);

    printf("   {%d,%d,%d}%s",
	   elements[node].x - ci, elements[node].y - cj, elements[node].att,
	   node < el-1 ? ",\n" : "};\n\n");
  }
}





/* ================================================================ */
/*         stuff to write out the stored pattern structures         */
/* ================================================================ */




/* Function to compare two patterns, by maxweight
 * We are not actually sorting the array of patterns,
 * but an array of integers which index the array.
 * This saves a lot of shuffling of indexes, since the
 * data is actually stored across several arrays.
 * So the parameters are actually pointers to ints
 */

static int
compare_patterns(const void *a, const void *b)
{
  return pattern[*(const int *)b].maxwt - pattern[*(const int *)a].maxwt;
}


/* sort and write out the patterns */
static int
write_patterns(char *name)
{
  int indices[MAXPATNO];
  int i;
  int maxpat=0;

  /* fill a table of pointers to the stored patterns */
  for (i=0; i<patno; ++i)
    indices[i] = i;

  /* now sort this array of pointers, unless we are in half-eye mode */
  if (pattern_type == PATTERNS)
    qsort(indices, patno, sizeof(int), compare_patterns);

  /* and write out the patterns in sorted order */
  printf("struct pattern %s[]={\n",name);

  for(i=0;i<patno;++i) {
    int j = indices[i];  /* pattern number to write next */
    struct pattern *p = pattern + j;

    /* p->min{i,j} and p->max{i,j} are the maximum extent of the elements,
     * including any rows of * which do not feature in the elements list.
     * ie they are the positions of the topleft and bottomright corners of
     * the pattern, relative to the pattern origin. These just transform same
     * as the elements.
     */
    
    printf("  {%s%d,%d,%d, \"%s\",%d,%d,%d,%d,0x%x,%d,%d",
	     name, j,
	     p->patlen,
	     p->trfno,
	     pattern_names[j],
	     p->mini, p->minj,
	     p->maxi, p->maxj,
	     p->edge_constraints,
	     p->movei, p->movej);


#ifdef GRID_OPT
    printf(",\n    {");
    {
      int ll;

      for (ll=0; ll<8; ++ll)
	printf(" 0x%08x%s", p->and_mask[ll], ll<7 ? "," : "");
      printf("},\n    {");
      for (ll=0; ll<8; ++ll)
	printf(" 0x%08x%s", p->val_mask[ll], ll<7 ? "," : "");
    }
    printf("}\n   ");
#endif

    switch (pattern_type) {
    case PATTERNS:
      printf(", %d,%d,%d,",
	     p->patwt,
	     p->maxwt,
	     p->assistance);
      if (p->assistance != NO_ASSISTANCE)
	printf("assist_params%d",j);
      else
	printf("NULL");
      printf(", %d,%d,%d,%d,%d,%d,%s,",
	     p->class,
	     p->obonus,
	     p->xbonus,
	     p->splitbonus,
	     p->minrand,
	     p->maxrand,
	     helper_fn_names[j]);
      if (p->autohelper)
	printf("autohelper%d",j);
      else
	printf("NULL");
      printf(",%d},\n", p->anchored_at_X);
      break;

    case HALFEYES:
      printf(",0,0,0,NULL,0,0,0,0,0,0,NULL,NULL,0},\n");
      break;

    case CONNECTIONS:
      printf(",0,0,0,NULL,%d,0,0,0,0,0,%s,", p->class, helper_fn_names[j]);
      if (p->autohelper)
	printf("autohelper%d",j);
      else
	printf("NULL");
      printf(",0},\n");
      break;
    }
    ++maxpat;
  }

  printf("  {NULL, 0,0,NULL,0,0,0,0,0,0,0,{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},0,0,0,NULL,0,0,0,0,0,0,NULL,NULL,0}\n};\n");
  return maxpat;
}



//int main(int argc, char *argv[])
void mkpat(int argc, char *argv[])
{
  char line[MAXLINE];  /* current line from file */
  int maxpat;
  int state=0;
  
  {
    int i;
    /* parse command-line args */
    while ( (i=getopt(argc, argv, "vhc")) != EOF) {
      switch(i) {
      case 'v' : verbose = 1; break;
      case 'h' : pattern_type = HALFEYES; break;
      case 'c' : pattern_type = CONNECTIONS; break;
      default:
	fprintf(stderr, "Invalid argument ignored\n");
      }
    }
  }


  if (optind >= argc) {
    fputs(USAGE, stderr);
    exit(EXIT_FAILURE);
  }

  switch (pattern_type) {
  case PATTERNS:
    printf(PAT_PREAMBLE);
    break;
  case CONNECTIONS:
    printf(CONN_PREAMBLE);
    break;
  }

  /* Parse the input file, output pattern elements as as we find them,
   * and store pattern entries for later output.
   */

  /* Initialize pattern number and buffer for automatically generated
   * helper code.
   */
  patno=-1;
  autohelper_code[0]=0;
  code_pos=autohelper_code;
  
  /* We do this parsing too with the help of a state machine.
   * state
   *   0     Waiting for a Pattern line.
   *   1     Pattern line found, waiting for a position diagram.
   *   2     Reading position diagram.
   *   3     Waiting for entry line (":" line).
   *   4     Waiting for optional constraint diagram.
   *   5     Reading constraint diagram.
   *   6     Waiting for constraint line (";" line).
   *   7     Reading a constraint
   *
   * FIXME: This state machine should be possible to simplify.
   *
   */
  
  while (fgets(line,MAXLINE,stdin)) {
    if (line[strlen(line)-1] != '\n') {
      fprintf(stderr,"Error, line truncated: %s\n",line);
      fatal_errors++;
    }

    /* FIXME: we risk to overrun a buffer here */
    if (sscanf(line, "Pattern %s", pattern_names[patno+1]) == 1) {
      char *p = strpbrk(pattern_names[patno+1]," \t");

      if(p)
	*p=0;
      if (patno>=0) {
	switch (state) {
	case 1:
	  fprintf(stderr,"Warning, empty pattern %s\n",
		  pattern_names[patno]);
	  break;
	case 2:
	case 3:
	  fprintf(stderr,"Warning, no entry line for pattern %s\n",
		  pattern_names[patno]);
	  break;
	case 5:
	case 6:
	  fprintf(stderr,
		  "Warning, constraint diagram but no constraint line for pattern %s\n",
		  pattern_names[patno]);
	  break;
	case 7:
	  finish_constraint(); /* fall through */
	case 0:
	case 4:
	  patno++;
	  reset_pattern();
	}
      }
      else {
	patno++;
	reset_pattern();
      }
      state=1;
    } else if (line[0] == '\n' || line[0]== '#') { /* nothing */
      if (state==2 || state==5)
	state++;
    } else if ( strchr(VALID_PATTERN_CHARS, line[0]) ||
                strchr(VALID_EDGE_CHARS, line[0]) ||
		strchr(VALID_CONSTRAINT_LABELS, line[0])) { /* diagram line */
      switch (state) {
      case 0:
      case 3:
      case 6:
      case 7:
	fprintf(stderr,"Huh, another diagram here? (pattern %s)\n",
		pattern_names[patno]);
	break;
      case 1:
	state++; /* fall through */
      case 2:
	read_pattern_line(line);
	break;
      case 4:
	state++; /* fall through */
      case 5:
	read_constraint_diagram_line(line);
	break;
      }	
    } else if (line[0]==':') {
      if (state==2 || state==3) {
	finish_pattern(line);
	write_elements(argv[optind]);
	state=4;
      } else {
	fprintf(stderr,"Warning, unexpected entry line in pattern %s\n",
		pattern_names[patno]);
      }
    } else if (line[0]==';') {
      if (state==5 || state==6 || state==7) {
	read_constraint_line(line+1);
	state=7;
      } else {
	fprintf(stderr,"Warning, unexpected constraint line in pattern %s\n",
		pattern_names[patno]);
      }
    }
  }

  if (patno>=0) {
    switch (state) {
    case 1:
      fprintf(stderr,"Warning, empty pattern %s\n",
	      pattern_names[patno]);
      break;
    case 2:
    case 3:
      fprintf(stderr,"Warning, no entry line for pattern %s\n",
	      pattern_names[patno]);
      break;
    case 5:
    case 6:
      fprintf(stderr,
	      "Warning, constraint diagram but no constraint line for pattern %s\n",
	      pattern_names[patno]);
      break;
    case 7:
      finish_constraint(); /* fall through */
    case 0:
    case 4:
      patno++;
      reset_pattern();
    }
  }


  if (verbose)
    fprintf(stderr, "%d / %d patterns have edge-constraints\n",
	    pats_with_constraints, patno);

  /* Write the autohelper code. */
  printf("%s",autohelper_code);
  
  maxpat=write_patterns(argv[optind]);

  {
    char prefix[4];

    prefix[0]=toupper(argv[optind][0]);
    prefix[1]=toupper(argv[optind][1]);
    prefix[2]=toupper(argv[optind][2]);
    prefix[3]=0;

    printf("\n#define %sNO %d\n\n",prefix,maxpat);
  }

  return fatal_errors ? 1 : 0;
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
