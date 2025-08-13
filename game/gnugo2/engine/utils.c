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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "liberty.h"
#include "ttsgf.h"
#include "ttsgf_write.h"
#include "interface.h"

/* Stack of boards for storing positions while reading. */
static board_t  stack[MAXSTACK][MAX_BOARD][MAX_BOARD]; 

/* Stack of trial moves to get to current position 
 * position and which color made them. Perhaps 
 * this should be one array of a structure 
 */
static int      stacki[MAXSTACK];
static int      stackj[MAXSTACK];      
static int      move_color[MAXSTACK];  
static int      ko_stacki[MAXSTACK];
static int      ko_stackj[MAXSTACK];      

static Hash_data  hashdata_stack[MAXSTACK];

/* Stack of black and white captured stones */
static int      stackbc[MAXSTACK];     
static int      stackwc[MAXSTACK];


/* pushgo pushes the position onto the stack. */

int 
pushgo()
{
  if (showstack)
    gprintf("        *** STACK before push: %d\n", stackp); 
  memcpy(stack[stackp], p, sizeof(p));
  ko_stacki[stackp] = ko_i;
  ko_stackj[stackp] = ko_j;
  memcpy(&hashdata_stack[stackp], &hashdata, sizeof(hashdata));

  stackbc[stackp]=black_captured;
  stackwc[stackp]=white_captured;
  stackp++;
  return(stackp);
}

/* popgo pops the position from the stack. */

int 
popgo()
{
  stackp--;
  if (showstack)
    gprintf("<=    *** STACK  after pop: %d\n", stackp);
   
  memcpy(p, stack[stackp], sizeof(p) );
  ko_i = ko_stacki[stackp];
  ko_j = ko_stackj[stackp];

  memcpy(&hashdata, &(hashdata_stack[stackp]), sizeof(hashdata) );

  black_captured=stackbc[stackp];
  white_captured=stackwc[stackp];
  if (count_variations) 
    sgf_write_line(")\n");
  return (stackp);
}




/* trymove pushes the position onto the stack, and makes a move
at (i, j) of color. Returns zero if the move is not legal. The
stack pointer is only incremented if the move is legal.

So the way to use this is:

   if (trymove(i, j, color, [message], k, l)) {
        ...
        popgo();
   }   

The message can be written as a comment to an sgf file using sgfdump().  
(k, l) can be -1 if they are not needed but if they are not the location of
(k, l) is included in the comment.

*/

int 
trymove(int i, int j, int color, const char *message, int k, int l)
{
  /* we could call legal() here, but that has to do
   * and then un-do all the stuff we want to re-do here.
   * (pushgo, updateboard, ...)  So we replicate its functionality
   * inline.
   */

  /* 1. The move must be inside the board. */
  assert(i>=0 && i<board_size && j>=0 && j<board_size);

  /* 2. The location must be empty. */
  if (p[i][j]!=EMPTY)
    return 0;

  /* 3. The location must not be the ko point. */
  if (i == ko_i && j == ko_j)
    if (((i >0) && (p[i-1][j] != color))
	|| ((i==0) && (p[i+1][j] != color)))
      {
	RTRACE("%m would violate the ko rule\n", i, j);
	return 0;
      }

  /* So far, so good.  Now push the move on the move stack. */
  stacki[stackp]=i;
  stackj[stackp]=j;
  move_color[stackp]=color;

  /* Check for stack overflow. */
  if (stackp >= MAXSTACK-2) {
    fprintf(stderr, 
	    "gnugo: Truncating search. This is beyond my reading ability!\n");
    return 0;
  }

  /* Ok, the trivial tests went well. Push the position on the stack, 
     and try to actually make the move. */
  pushgo();
  stats.nodes++;

  if (verbose==4)
    dump_stack();

  if (updateboard(i,j,color) == 0) {

    /* no trivial liberties : we need to run approxlib to
     * make sure we do actually have some liberties !
     */

    if (approxlib(i,j,color,1) == 0) {
      RTRACE("%m would be suicide\n", i, j);

      /* Almost do the work of popgo().  The only difference is that
	 we don't close the variations in the sgf file. */
      stackp--;
      if (showstack)
	gprintf("<=    *** STACK  after pop: %d\n", stackp);
   
      memcpy(p, stack[stackp], sizeof(p) );
      ko_i = ko_stacki[stackp];
      ko_j = ko_stackj[stackp];
      memcpy(&hashdata, &(hashdata_stack[stackp]), sizeof(hashdata) );
      black_captured = stackbc[stackp];
      white_captured = stackwc[stackp];

      return 0;
    }
  }
  /*  hashdata_set_tomove(&hashdata, OTHER_COLOR(color));*/

  if (sgf_dump) {
    if (k == -1) {
      if (color==BLACK)
	sgf_write_line("\n(;B[%c%c]C[%s (%d)\n]", 
		       'a'+j, 'a'+i, message, count_variations);
      if (color==WHITE)
	sgf_write_line("\n(;W[%c%c]C[%s (%d)\n]",
		       'a'+j, 'a'+i, message, count_variations);
    } else {
      char movename[4];
	
      if (l < 8)
	*movename=l+65;
      else
	*movename=l+66;
      sprintf(movename+1, "%d", board_size-k);
	
      if (color==BLACK)
	sgf_write_line("\n(;B[%c%c]C[%s at %s (%d)\n]",
		       'a'+j, 'a'+i, message, movename, count_variations);
      if (color==WHITE)
	sgf_write_line("\n(;W[%c%c]C[%s at %s (%d)\n]",
		       'a'+j, 'a'+i, message, movename, count_variations);
    }
  }
  if (count_variations)
    count_variations++;

  return(1);

}  

/* tryko pushes the position onto the stack, and makes a move
at (i, j) of color. The move is allowed even if it is an
illegal ko capture. It is to be imagined that (color) has
made an intervening ko threat which was answered and now
the continuation is to be explored.

*/

int 
tryko(int i, int j, int color, const char *message)
{
  assert (i>=0 && i<board_size && j>=0 && j<board_size);

  if (p[i][j]!=EMPTY)
    return 0;

  stacki[stackp]=i;
  stackj[stackp]=j;
  move_color[stackp]=color;

  if (stackp >= MAXSTACK-2) {
    fprintf(stderr, 
	    "gnugo: Truncating search. This is beyond my reading ability!\n");
    return 0;
  }

  pushgo();
  stats.nodes++;

  if (verbose==4)
    dump_stack();

  if (updateboard(i,j,color) == 0) {
    fprintf(stderr, "tryko: illegal move\n");
    verbose=4;
    dump_stack();
    abort();
  }

  if (sgf_dump) {
    if (color==BLACK)
      sgf_write_line("\n(;B[tt]C[tenuki (ko threat)]\n\
;W[tt]C[tenuki (answers ko threat)]\n;B[%c%c]C[%s (%d)\n]", 
		     'a'+j, 'a'+i, message, count_variations);
    else if (color==WHITE)
      sgf_write_line("\n(;W[tt]C[tenuki (ko threat)]\n\
;B[tt]C[tenuki (answers ko threat)]\n;B[%c%c]C[%s (%d)\n]",
		       'a'+j, 'a'+i, message, count_variations);
  }
  if (count_variations)
    count_variations++;

  return(1);

}  

/* dump_stack() for use under gdb prints the move stack. */

void
dump_stack(void)
{
  int n;

  for (n=0; n<stackp; n++) {
    gprintf("%o%s:%m ", move_color[n] == BLACK ? "B" : "W", stacki[n], stackj[n]);
  }
  if (count_variations)
    gprintf("%o (variation %d)", count_variations-1);
  gprintf("\n");
}


/* save_state() and restore_state() backup and restore the board
 * position without using the stack. They are used in counting
 * under the ascii interface by endgame().
 */
static board_t p_backup[MAX_BOARD][MAX_BOARD];
static int black_captured_backup, white_captured_backup;

void
save_state()
{
  black_captured_backup=black_captured;
  white_captured_backup=white_captured;
  memcpy(p_backup, p, sizeof(p));
}

void
restore_state()
{
  black_captured=black_captured_backup;
  white_captured=white_captured_backup;
  memcpy(p, p_backup, sizeof(p));
}


/* this fn underpins all the TRACE and DEBUG stuff.
 *  Accepts %c, %d and %s as usual. But it
 * also accepts %m, which takes TWO integers and writes a move
 * Nasty bodge : %o at start means outdent (ie cancel indent)
 */

static void 
vgprintf(FILE* outputfile, const char *fmt, va_list ap)
{
  if (fmt[0] == '%' && fmt[1] == 'o')
    fmt +=2;  /* cancel indent */
  else if (stackp > 0)
    fprintf(outputfile, "%.*s", stackp*2, "                                ");  /* "  " x (stackp * 2) */

  for ( ; *fmt ; ++fmt )
  {
    if (*fmt == '%')
    {
      switch(*++fmt)
      {
      case 'c':
      {
	int c = va_arg(ap, int);  /* rules of promotion => passed as int, not char */
	putc(c, outputfile);
	break;
      }
      case 'd':
      {
	int d = va_arg(ap, int);
	fprintf(outputfile, "%d", d);
	break;
      }
      case 's':
      {
	char *s = va_arg(ap, char*);
	assert( (int)*s >= board_size );  /* in case %s used in place of %m */
	fputs(s, outputfile);
	break;
      }
      case 'm':
      {
	char movename[4];
	int m = va_arg(ap, int);
	int n = va_arg(ap, int);
	assert( (m < board_size && n < board_size));
	if ((m<0) || (n<0))
	  fputs("??",outputfile);
	else {                       /* generate the move name */
	  if (n<8)
	    movename[0]=n+65;
	  else
	    movename[0]=n+66;
	  sprintf(movename+1, "%d", board_size-m);
	}
	fputs(movename, outputfile);
	break;
      }
      default:
	fprintf(outputfile, "\n\nUnknown format character '%c'\n", *fmt);
	break;
      }
    }
    else
      putc(*fmt, outputfile);
  }
}



/* required wrapper around vgprintf */
void 
gprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vgprintf(stderr,fmt, ap);
  va_end(ap);
}
/* required wrapper around vgprintf */
void
mprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vgprintf(stdout,fmt, ap);
  va_end(ap);
}





#ifndef __GNUC__  /* gnuc allows us to define these as macros in liberty.h */
void 
TRACE(const char *fmt, ...)
{
  va_list ap;

  if (!verbose)
    return;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void 
VTRACE(const char *fmt, ...)
{
  va_list ap;

  if (verbose < 3)
    return;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void 
RTRACE(const char *fmt, ...)
{
  va_list ap;

  if (verbose < 2)
    return;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void 
DEBUG(int level, const char *fmt, ...)
{
  va_list ap;

  if (!(debug & level))
    return;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}
#endif





/* if chain at m,n has no liberties, remove it from
 * the board. Return the number of stones captured.
 */

static char check_mx[MAX_BOARD][MAX_BOARD];
static char check_mark = -1;

static void
init_check_for_capture(void)
{
  /* set all pieces as unmarked */
  memset(check_mx, 0, sizeof(check_mx));
  check_mark = 1;
}

static int 
check_for_capture(int m, int n, int color)
{
  int i,j;
  int captured=0; /* number captured */

  ASSERT(p[m][n] == color, m,n);

  DEBUG(DEBUG_COUNT,"Checking %m for capture\n", m,n);

  check_mark++;
  if (check_mark == 0) { /* We have wrapped around, reinitialize. */
    init_check_for_capture();
  }

  if (count(m, n, color, check_mx, 1, check_mark) > 0) {
    return 0;
  }

  /* remove all the stones with mx[i][j] == 0,
   *  (since these are the ones count() touched)
   * - count also marks EMPTY stones, but there's
   * no harm in resetting them to EMPTY
   */

  for (i=0; i<board_size; ++i)
    for (j=0; j<board_size; ++j)
      if (check_mx[i][j] == check_mark) {
	p[i][j] = EMPTY;
	hashdata_invert_stone(&hashdata, i, j, color);
	++captured;
      }
  
  if (color == WHITE)
    white_captured += captured;
  else
    black_captured += captured;

  return captured;
}

/* legal(i, j, color) determines whether the move (color) at (i, j) is legal */

int 
legal(int i, int j, int color)
{
  int legit=0;

  /* 0. A pass move is always legal. */
  if (i == board_size && j == board_size)
    return 1;

  /* 1. The move must be inside the board. */
  assert(i>=0 && i<board_size && j>=0 && j<board_size);

  /* 2. The location must be empty. */
  if (p[i][j]!=EMPTY) 
    return 0;

  /* 3. The location must not be the ko point. */
  if (i == ko_i && j == ko_j)
    if (((i >0) && (p[i-1][j] != color))
	|| ((i==0) && (p[i+1][j] != color)))
      {
	RTRACE("%m would violate the ko rule\n", i, j);
	return 0;
      }

  /* Check for stack overflow. */
  if (stackp >= MAXSTACK-2) {
    fprintf(stderr, 
	    "gnugo: Truncating search. This is beyond my reading ability!\n");
    return 0;
  }

  /* Ok, the trivial tests went well. Push the position on the stack, 
     and try to actually make the move. */
  pushgo();
  updateboard(i, j, color);

  /* only care about 0 or >=1 */
  if (approxlib(i, j, color, 1)==0) {
    RTRACE("%m would be suicide\n", i, j);
    legit = 0;
  } else
    legit = 1;

  stackp--;
  if (showstack)
    gprintf("<=    *** STACK  after pop: %d\n", stackp);
   
  memcpy(p, stack[stackp], sizeof(p) );
  ko_i = ko_stacki[stackp];
  ko_j = ko_stackj[stackp];
  memcpy(&hashdata, &(hashdata_stack[stackp]), sizeof(hashdata) );
  black_captured=stackbc[stackp];
  white_captured=stackwc[stackp];

  return legit;
}



/* a wrapper around abort() which shows the state variables at the time of the problem
 * i, j are typically a related move, or -1, -1
 *
 */

void 
abortgo(const char *file, int line, const char *msg, int x, int y)
{
  int i=0;

  verbose=4;
  TRACE( "%o\n\n***assertion failure :\n%s:%d - %s near %m***\n\n",  file, line, msg, x, y);
  dump_stack();
  showboard(0);

#if 0
  while (stackp > 0)
  {
    popgo();
    showboard(0);
  }
#endif

  if(sgf_root)
    writesgf(sgf_root,"abortgo.sgf",get_seed());

  RTRACE( "lib=%d\nliberties:", lib);
  for (i=0; i<lib; ++i)
    RTRACE( "%m\n", libi[i], libj[i]);

  fprintf(stderr,"\n\
gnugo %s: You stepped on a bug.\n\
Please save this game as an sgf file \
and mail together with the file abortgo.sgf to \
gnugo@gnu.org\n\n",VERSION);

  abort();  /* cause core dump */
}




/*
 * Place a "color" on the board at i,j, and remove
 * any captured stones.  If (i, j) == (boardsize, boardsize),
 * the move is a pass, but that is OK too.
 *
 * Assumes that the only stones which can be captured are
 * those immediately adjacent to the piece just played.
 *
 * Returns non-zero if there are any "trivial" liberties, ie if
 * we capture any stones or there are any adjacent liberties.
 * A return value of 0 does not mean that the placed stone has no
 * liberties, though a return of >0 means it definately does.
 * (ie return of >0 is sufficient, though not necessary, test
 * for move not being a suicide.)
 *
 *  Globals affected : uses count(), so anything it affects
 */

int 
updateboard(int i, int j, int color)
{
  int other = OTHER_COLOR(color);
  int obvious_liberties=0;
  int captures = 0;
  int tot_captures = 0;

#if CHECK_HASHING
  {
    Hashposition pos;
    unsigned long hash;

    /* Check the hash table to see if we have had this position before. */
    board_to_position(p, ko_i, ko_j, /*color,*/ &pos);
    hash = board_hash(p, ko_i, ko_j/*, color*/);
    assert(hash == hashdata.hashval);
    assert(hashposition_compare(&pos, &(hashdata.hashpos)) == 0);
  }
#endif

  last_move_i=i;
  last_move_j=j;

  ko_i = -1;
  ko_j = -1;
  hashdata_remove_ko(&hashdata);

  /* If the move is a pass, we are done here. */
  if (i >= board_size && j >= board_size)
    return -1;

  assert(i>=0 && i<board_size && j>=0 && j<board_size);
  ASSERT( p[i][j] == EMPTY, i, j);

  if (stackp==0) 
    color_has_played|=color;

  p[i][j] = color;
  hashdata_invert_stone(&hashdata, i, j, color);

  DEBUG(DEBUG_BOARD, "Update board : %m = %d\n", i,j, color);

  /* Check to the top. */
  if (i>0) {
    if (p[i-1][j] == other) {
      tot_captures = check_for_capture(i-1, j, other);
      obvious_liberties = tot_captures;
    } else if (p[i-1][j] == EMPTY)
      ++obvious_liberties;
  }

  /* Check to the bottom. */
  if (i<board_size-1) {
    if (p[i+1][j]==other) {
      captures = check_for_capture(i+1, j, other);
      tot_captures += captures;
      obvious_liberties += captures;
    } else if (p[i+1][j] == EMPTY)
      ++obvious_liberties;
  }

  /* Check to the left. */
  if (j>0) {
    if (p[i][j-1]==other) {
      captures = check_for_capture(i, j-1, other);
      obvious_liberties += captures;
      tot_captures += captures;
    } else if (p[i][j-1]==EMPTY)
      ++obvious_liberties;
  }

  /* Check to the right. */
  if (j<board_size-1) {
    if (p[i][j+1]==other) {
      captures = check_for_capture(i, j+1, other);
      obvious_liberties += captures;
      tot_captures += captures;
    } else if (p[i][j+1]==EMPTY)
      ++obvious_liberties;
  }

  /* Check for possible ko and store it into the ko variables. */
  if (tot_captures == 1) {
    int  friends = 0;
    int  libs = 0;
    int  libertyi = -1;
    int  libertyj = -1;

    if (i > 0) {
      if (p[i-1][j] == color) 
	friends++;
      if (p[i-1][j] == EMPTY) {
	libs++;
	libertyi = i - 1;
	libertyj = j;
      }
    }
    if (i < board_size - 1) {
      if (p[i+1][j] == color) 
	friends++;
      if (p[i+1][j] == EMPTY) {
	libs++;
	libertyi = i + 1;
	libertyj = j;
      }
    }
    if (j > 0) {
      if (p[i][j-1] == color) 
	friends++;
      if (p[i][j-1] == EMPTY) {
	libs++;
	libertyi = i;
	libertyj = j - 1;
      }
    }
    if (j < board_size - 1) {
      if (p[i][j+1] == color) 
	friends++;
      if (p[i][j+1] == EMPTY) {
	libs++;
	libertyi = i;
	libertyj = j + 1;

      }
    }

    /* This test guarantees that the newly put stone has exactly one
     * liberty (the newly caught stone) and that it doesn't connect
     * to any friendly stones.  In the latter case it is called 
     * a snapback, and not a ko, which is ok.  
     */
    if (libs == 1 && friends == 0) {
      ko_i = libertyi;
      ko_j = libertyj;
      hashdata_set_ko(&hashdata, libertyi, libertyj);
    }
  }

#if CHECK_HASHING
  {
    Hashposition pos;
    unsigned long hash;

    /* Check the hash table to see if we have had this position before. */
    board_to_position(p, ko_i, ko_j, /*color,*/ &pos);
    hash = board_hash(p, ko_i, ko_j/*, color*/);
    assert(hashposition_compare(&pos, &(hashdata.hashpos)) == 0);
    assert(hash == hashdata.hashval);
  }
#endif

  return obvious_liberties;
}

/*initialize board*/
void
init_board()
{
   memset(p, EMPTY, sizeof(p));
   ko_i = -1;
   ko_j = -1;
   hashdata_init(&hashdata);
   last_move_i=last_move_j=-1;
   depth=DEPTH;
   backfill_depth=BACKFILL_DEPTH;
   fourlib_depth=FOURLIB_DEPTH;
   ko_depth=KO_DEPTH;
   init_moyo();
}

/* count (minimum) liberty of color piece at m, n
 * m,n may contain either that color, or be empty, in which
 * case we get what the liberties would be if we were to place
 * a piece there.
 * If the caller only cares about a minimum liberty count, it
 * should pass that minimum in, and then we *may* stop counting
 * if/when we reach that number. (We may still return a larger
 * number if we feel like it. (The current implementation doesn't.))
 *
 * Note that countlib(i,j,c) is defined as approxlib(i,j,c,99999)
 */

static char approxlib_ml[MAX_BOARD][MAX_BOARD];
static char approxlib_mark = -1;

static void
init_approxlib(void)
{
  /* set all pieces as unmarked */
  memset(approxlib_ml,0,sizeof(approxlib_ml));
  approxlib_mark = 1;
}
  

int 
approxlib(int m,     /* row number 0 to board_size-1 */
	  int n,     /* column number 0 to board_size-1 */
	  int color, /* BLACK or WHITE */
	  int maxlib /* count might stop when it reaches this */
	  )
{
  int i;

  ASSERT(m>=0 && m < board_size && n >= 0 && n < board_size,m,n);
  ASSERT(color != EMPTY,m,n);
  
  ASSERT(p[m][n] != OTHER_COLOR(color), m,n);

  approxlib_mark++;
  if (approxlib_mark == 0) { /* We have wrapped around, reinitialize. */
    init_approxlib();
  }

  /* count liberty of current piece */
  i = count(m, n, color, approxlib_ml, maxlib, approxlib_mark);

  /* trying to get rid of globals... */
  assert(i == lib);  /* return value should match global in this case */
  
  DEBUG(DEBUG_COUNT, "approxlib(%m,%d) returns %d\n", m,n,maxlib,i);
  
  return(i);
}






/* count (minimum) liberty of color piece at location i, j
   and return value in global variable lib. Return size of
   connected component in size.

   No check is made on entry that that square is occupied
   by that color : it is valid to use this to count the
   liberties that there would be if color was put into
   empty square at i,j.

   if k<lib, then libi[k],libj[k] points to one of the
   liberties of the string.

  ENTRY : mark is the current value of "mark"
  
          mx[][] contains "mark" for stones and empty intersections that
	  may not be counted. Typically "mark" is a value that at calling
	  time is absent from mx[][].

          maxlib : typically, it is not necessary to get the
          exact number of liberties : we can stop when it
          reaches some threshold (often 1 !).

  RETURNS : number of liberties found

  EXIT :  return num of liberties in global "lib"

          return positions of liberties in globals libi[] and libj[]
  
          return group size in global "size"

          updates mx[][], putting "mark" at all stones and empty
	  intersections visited

	  EITHER : return < maxlib, and mx[][] and l[][] fully updated
	  OR     : return == maxlib, and mx[][] and l[][] partially updated

  NOTE : intended as an internal helper for approxlib, but
         is exported as a general (though complex) function
         for efficiency.
*/

static int count_stacki[MAXCHAIN];
static int count_stackj[MAXCHAIN];

int 
count(int i,     /* row number 0 to board_size-1 */
      int j,     /* column number 0 to board_size-1 */
      int color, /* BLACK or WHITE */
      char mx[MAX_BOARD][MAX_BOARD],  /* workspace : mx[][]=0 means stone already visited */
      int maxlib,       /* often, we only care if liberties > some minimum (usually 1) */
      char mark) /* Value to mark visited points with. */
{
  int count_stackp;
  
  mx[i][j] = mark;
  count_stacki[0] = i;
  count_stackj[0] = j;
  count_stackp = 1;
  size = 0;
  lib = 0;
  
  while (count_stackp>0) {
    count_stackp--;
    i = count_stacki[count_stackp];
    j = count_stackj[count_stackp];
    size++;

    /* check West neighbor */
    if (j != 0 && mx[i][j-1] != mark)
    {
      if (p[i][j-1] == EMPTY)
      {
	mx[i][j-1] = mark;
	libi[lib] = i;
	libj[lib] = j-1;
	++lib;
	if (lib == maxlib)
	  return lib;
      }
      else if (p[i][j-1] == color) {
	mx[i][j-1] = mark;
	count_stacki[count_stackp] = i;
	count_stackj[count_stackp] = j-1;
	count_stackp++;
      }
    }
    
    while(1) {
      /* check North neighbor */
      if (i != 0 && mx[i-1][j] != mark)
      {
	if (p[i-1][j] == EMPTY)
	{
	  mx[i-1][j] = mark;
	  libi[lib] = i-1;
	  libj[lib] = j;
	  ++lib;
	  if (lib == maxlib)
	    return lib;
	} 
	else if (p[i-1][j] == color) {
	  mx[i-1][j] = mark;
	  count_stacki[count_stackp] = i-1;
	  count_stackj[count_stackp] = j;
	  count_stackp++;
	}
      }

      /* check South neighbor */
      if (i != board_size-1 && mx[i+1][j] != mark)
      {
	if (p[i+1][j] == EMPTY) 
	{
	  mx[i+1][j] = mark;
	  libi[lib] = i+1;
	  libj[lib] = j;
	  ++lib;
	  if (lib == maxlib)
	    return lib;
	}
	else if (p[i+1][j] == color) {
	  mx[i+1][j] = mark;
	  count_stacki[count_stackp] = i+1;
	  count_stackj[count_stackp] = j;
	  count_stackp++;
	}
      }

      /* check East neighbor */
      if (j == board_size-1 || mx[i][j+1] == mark)
	break;
      if (p[i][j+1] == EMPTY)
      {
	mx[i][j+1] = mark;
	libi[lib] = i;
	libj[lib] = j+1;
	++lib;
	if (lib == maxlib)
	  return lib;
	break;
      }
      else if (p[i][j+1] == color) {
	mx[i][j+1] = mark;
	size++;
	j++;
      }
      else
	break;
    }
  }

  return lib;
}  /* end count */


/*
 * Find the origin of a worm or a cavity, i.e. the point with smallest
 * i coordinate and in the case of a tie with smallest j coordinate.
 * The idea is to have a canonical reference point for a string.
 */

static int origin_stacki[MAXCHAIN];
static int origin_stackj[MAXCHAIN];
static char origin_mx[MAX_BOARD][MAX_BOARD];
static char origin_mark = -1;

void 
find_origin(int i, int j, int *origini, int *originj)
{
  int origin_stackp;
  int color = p[i][j];

  origin_mark++;
  if (origin_mark == 0) { /* We have wrapped around, reinitialize. */
    memset(origin_mx, 0, sizeof(origin_mx));
    origin_mark = 1;
  }
  
  *origini = i;
  *originj = j;
  
  origin_mx[i][j] = origin_mark;
  origin_stacki[0] = i;
  origin_stackj[0] = j;
  origin_stackp = 1;
  
  while (origin_stackp>0) {
    origin_stackp--;
    i = origin_stacki[origin_stackp];
    j = origin_stackj[origin_stackp];

    /* Is this point a possible origin? */
    if (i < *origini || (i == *origini && j < *originj)) {
      *origini = i;
      *originj = j;
    }
    
    /* check West neighbor */
    if (j != 0 && origin_mx[i][j-1] != origin_mark) {
      if (p[i][j-1] == color) {
	origin_mx[i][j-1] = origin_mark;
	origin_stacki[origin_stackp] = i;
	origin_stackj[origin_stackp] = j-1;
	origin_stackp++;
      }
    }
    
    while(1) {
      /* check North neighbor */
      if (i != 0 && origin_mx[i-1][j] != origin_mark)
      {
	if (p[i-1][j] == color) {
	  origin_mx[i-1][j] = origin_mark;
	  origin_stacki[origin_stackp] = i-1;
	  origin_stackj[origin_stackp] = j;
	  origin_stackp++;
	}
      }

      /* check South neighbor */
      if (i != board_size-1 && origin_mx[i+1][j] != origin_mark)
      {
	if (p[i+1][j] == color) {
	  origin_mx[i+1][j] = origin_mark;
	  origin_stacki[origin_stackp] = i+1;
	  origin_stackj[origin_stackp] = j;
	  origin_stackp++;
	}
      }

      /* check East neighbor */
      if (j == board_size-1 || origin_mx[i][j+1] == origin_mark)
	break;
      if (p[i][j+1] == color) {
	origin_mx[i][j+1] = origin_mark;
	j++;
      }
      else
	break;
    }
  }
}


/* See the Texinfo documentation (Dragon) for the definition of
 * strategic_distance.
 */

int 
strategic_distance_to(int color, int i, int j)
{
  if (color==BLACK)
    return (strategic_distance_to_black[i][j]);
  else if (color==WHITE)
    return (strategic_distance_to_white[i][j]);    
  else return (0);
}

/* see DRAGON for the definition of distance */

int 
distance_to(int color, int i, int j)
{
  if (color==BLACK)
    return (distance_to_black[i][j]);
  else if (color==WHITE)
    return (distance_to_white[i][j]);    
  else return (0);
}


/* The static evaluation function gives GNU Go's evaluation of the position. */

/*

int static_eval(int color)
{
  int other=OTHER_COLOR(color);
  int value=0;
  int m, n;
  
  for (m=0; m<board_size; m++)
    for (n=0; n<board_size; n++) {
      if (p[m][n] == EMPTY) {
	if ((strategic_distance_to(color, m, n) < 5) &&
	    (strategic_distance_to(other, m, n) > 10)) 
	  value += 5;
	else 
	if ((strategic_distance_to(other, m, n) < 5) &&
	    (strategic_distance_to(color, m, n) > 10)) 
	  value -= 5;
      }
      else if ((dragon[m][n].origini == m) && (dragon[m][n].originj == n)) {
	if (dragon[m][n].color==color) {
	  if (dragon[m][n].status==ALIVE) 
	    value += dragon[m][n].value;
	  else if (dragon[m][n].status==UNKNOWN)
	    value += (4*dragon[m][n].value/5);
	  else if (dragon[m][n].status==CRITICAL)
	    value += dragon[m][n].value/2;
	}
	else if (dragon[m][n].color==other) {
	  if (dragon[m][n].status==ALIVE) 
	    value -= dragon[m][n].value;
	  else if (dragon[m][n].status==UNKNOWN)
	    value -= (4*dragon[m][n].value/5);
	  else if (dragon[m][n].status==CRITICAL)
	    value -= dragon[m][n].value/2;
	}
      }
    }
  return value;
}

*/


void 
remove_stone(int i, int j)
{
    assert(p[i][j] != EMPTY);
    p[i][j]=EMPTY;
}

/* Removes the string at (i,j), treating it as captured. - moved here from play_ascii */

void 
remove_string(int i, int j)
{
  int color=p[i][j];

  assert(color != EMPTY);

  p[i][j]=EMPTY;
  if (color==BLACK)
    black_captured++;
  else
    white_captured++;
  if ((i>0) && (p[i-1][j]==color))
    remove_string(i-1, j);
  if ((i<board_size-1) && (p[i+1][j]==color))
    remove_string(i+1, j);
  if ((j>0) && (p[i][j-1]==color))
    remove_string(i, j-1);
  if ((j<board_size-1) && (p[i][j+1]==color))
    remove_string(i, j+1);
}

/* Removes the string at (i,j), treating it as captured. - moved here from play_ascii */

void 
change_dragon_status(int x, int y, int status)
{
  int i,j;
  int origini=dragon[x][y].origini;
  int originj=dragon[x][y].originj;

  for(i=0;i<board_size;i++)
      for(j=0;j<board_size;j++)
      {
          if( ( dragon[i][j].origini==origini )
              &&( dragon[i][j].originj==originj ))
              dragon[i][j].status=status;
      }
}


/* moved here from play_ascii - measure territory */

void 
count_territory( int *white, int *black)
{
  int i,j;

  *white=0;
  *black=0;
  make_worms();

  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
      if (p[i][j]==EMPTY) {
	if (worm[i][j].color==BLACK_BORDER)
	  ++*black;
	else if (worm[i][j].color==WHITE_BORDER)
	  ++*white;
      }
}

/*
 modified to use dragons instead of worms
 removes dead dragons before counting
 ATTENTION! the caller has to pushgo and popgo
*/

void 
evaluate_territory( int *white, int *black)
{
  int i,j;

  *white=0;
  *black=0;

  /*first we remove all dead dragons*/
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
    {
        if((dragon[i][j].status==DEAD)&&(p[i][j]!=EMPTY))
            remove_string(i,j);
    }

  hashdata_init(&hashdata);	/* Make hashdata correct after removals. */
  make_worms();
  make_dragons();
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
        switch(p[i][j])
        {
        case EMPTY:
            if (dragon[i][j].color==BLACK_BORDER)
	  ++*black;
            else if (dragon[i][j].color==WHITE_BORDER)
	  ++*white;
            break;
        case BLACK:
            break;
        case WHITE:
            break;
      }
}


/* change_defense(ai, aj, ti, tj) moves the point of defense of the
 * worm at (ai, aj) to (ti, tj).
 *
 * FIXME: At present can only set defend_code equal to 1 or 0.
 */

void
change_defense(int ai, int aj, int ti, int tj)
{

  assert (stackp==0);

  if (ti == -1)
    TRACE("Removed defense of %m (was %m).\n",
	  worm[ai][aj].origini, worm[ai][aj].originj,
	  worm[ai][aj].attacki, worm[ai][aj].attackj);
  else if (worm[ai][aj].attacki == -1)
    TRACE("Setting defense of %m to %m.\n",
	  worm[ai][aj].origini, worm[ai][aj].originj, ti, tj);
  else
    TRACE("Moved defense of %m from %m to %m.\n",
	  worm[ai][aj].origini, worm[ai][aj].originj,
	  worm[ai][aj].defendi, worm[ai][aj].defendj, ti, tj);

  assert (ti == -1 || p[ti][tj]==EMPTY);
  assert (p[ai][aj]!=EMPTY);

  if ((worm[ai][aj].attacki != -1) && (worm[ai][aj].defendi != -1)
      && (ti == -1)) {
    int m, n;

    for (m=0; m<board_size; m++)
      for (n=0; n<board_size; n++)
	if ((dragon[m][n].origini==dragon[ai][aj].origini) 
	    && (dragon[m][n].originj==dragon[ai][aj].originj))
	  dragon[m][n].status=DEAD;
  }
  worm[worm[ai][aj].origini][worm[ai][aj].originj].defendi=ti;
  worm[worm[ai][aj].origini][worm[ai][aj].originj].defendj=tj;
  if (ti != -1)
    worm[worm[ai][aj].origini][worm[ai][aj].originj].defend_code=1;
  else
    worm[worm[ai][aj].origini][worm[ai][aj].originj].defend_code=0;
  propagate_worm(worm[ai][aj].origini, worm[ai][aj].originj);
}


/* change_attack(ai, aj, ti, tj) moves the point of attack of the
 * worm at (ai, aj) to (ti, tj).
 *
 * FIXME: At present can only set attack_code to 1 or 0.
 */

void
change_attack(int ai, int aj, int ti, int tj)
{
  int ui, uj;
  int oldattacki=worm[ai][aj].attacki;

  assert(stackp==0);

  if ((ti == -1) && (worm[ai][aj].attacki != -1)) 
    TRACE("Removed attack of %m (was %m).\n",
	  worm[ai][aj].origini, worm[ai][aj].originj,
	  worm[ai][aj].attacki, worm[ai][aj].attackj);
  else if (worm[ai][aj].attacki == -1)
    TRACE("Setting attack of %m to %m.\n",
	  worm[ai][aj].origini, worm[ai][aj].originj, ti, tj);
  else
    TRACE("Moved attack of %m from %m to %m.\n",
	  worm[ai][aj].origini, worm[ai][aj].originj,
	  worm[ai][aj].attacki, worm[ai][aj].attackj, ti, tj);

  assert (ti == -1 || p[ti][tj]==EMPTY);
  assert (p[ai][aj]!=EMPTY);

  worm[worm[ai][aj].origini][worm[ai][aj].originj].attacki=ti;
  worm[worm[ai][aj].origini][worm[ai][aj].originj].attackj=tj;
  if (ti != -1)
    worm[worm[ai][aj].origini][worm[ai][aj].originj].attack_code=1;
  else
    worm[worm[ai][aj].origini][worm[ai][aj].originj].attack_code=0;
  if ((oldattacki == -1) && (ti != -1) && find_defense(ai, aj, &ui, &uj))
    change_defense(ai, aj, ui, uj);

  propagate_worm(worm[ai][aj].origini, worm[ai][aj].originj);
}


/* 
 * connection_value() assigns a value to the connection of two dragons. 
 *
 * The move at (ti,tj) is the connecting move. It is checked whether
 * this move will result in a dragon having two eyes.
 */


int 
connection_value(int ai, int aj, int bi, int bj, int ti, int tj)
{
  int vala, valb, cv;
  ASSERT(p[ai][aj] != 0, ai, aj);
  ASSERT(p[bi][bj] == p[ai][aj], ai, aj);

  if (0) gprintf("considering %m-%m connection\n", ai, aj, bi, bj);

  if ((dragon[ai][aj].origini == dragon[bi][bj].origini) &&     /* already */
      (dragon[ai][aj].originj == dragon[bi][bj].originj))       /* connected */
    return (0);

  if ((dragon[ai][aj].status == ALIVE) && (dragon[bi][bj].status == ALIVE))
    return (0);
  vala=dragon[ai][aj].value;
  valb=dragon[bi][bj].value;
  if ((dragon[ai][aj].status == DEAD) && (dragon[bi][bj].status == DEAD)) {
    if (dragon[ai][aj].genus + dragon[bi][bj].genus == 0) {
      return (0);
    }
    else if (dragon[ai][aj].genus + dragon[bi][bj].genus == 1) {
      if (dragon[ai][aj].heyes + dragon[bi][bj].heyes == 0)
	return 0;
      else if (dragon[ai][aj].heyes + dragon[bi][bj].heyes == 1) {
	if ((dragon[ai][aj].heyei != ti || dragon[ai][aj].heyej != tj)
	    && (dragon[bi][bj].heyei != ti || dragon[bi][bj].heyej != tj))
	  return 0;
      }
    }
    if (0)
      gprintf("%m - %m connection is worth %d---vala=%d, valb=%d\n", ai, aj, bi, bj, vala+valb, vala, valb);
    return vala+valb;
  }
  if ((dragon[ai][aj].status == ALIVE) && ((dragon[bi][bj].status == DEAD) || (dragon[bi][bj].safety == CRITICAL)))
    cv = valb;
  else if ((dragon[bi][bj].status == ALIVE) && ((dragon[ai][aj].status == DEAD) || (dragon[ai][aj].safety == CRITICAL)))
     cv = vala;
  else if (dragon[ai][aj].status == ALIVE) {
    if (dragon[bi][bj].escape_route<5)
      cv = valb;
    else 
      cv = 2*valb/3;
  }
  else if (dragon[bi][bj].status == ALIVE) {
    if (dragon[ai][aj].escape_route<5)
      cv = vala;
    else 
      cv = 2*vala/3;
  }
  else if (dragon[ai][aj].vitality > 0) {
    if (dragon[bi][bj].escape_route<5)
      cv = valb;
    else 
      cv = 3*valb/4;
  }
  else if (dragon[bi][bj].vitality > 0) {
    if (dragon[ai][aj].escape_route<5)
      cv = vala;
    else 
      cv = 3*vala/4;
  }
  else if (dragon[ai][aj].status == DEAD)
    cv = vala;
  else if (dragon[bi][bj].status == DEAD)
    cv = valb;
  else cv = min(vala, valb);            /* both unknown */
  if (0)
    gprintf("%m - %m connection is worth %d---vala=%d, valb=%d\n", ai, aj, bi, bj, cv,vala,valb);
  return (cv);
}

/* Check whether a move at (ti,tj) stops the enemy from playing at (ai,aj). */
int defend_against(int ti, int tj, int color, int ai, int aj)
{
  if (trymove(ti, tj, color, "defend_against", -1, -1)) {
    if (!safe_move(ai, aj, OTHER_COLOR(color))) {
      popgo();
      return 1;
    }
    popgo();
  }
  return 0;
}


/* Returns true if color can cut at (i,j). This information is
 * collected by find_cuts(), using the B patterns in the connections
 * database.
 */

int
cut_possible(int i, int j, int color)
{
  if (color == WHITE)
    return black_eye[i][j].cut;
  else
    return white_eye[i][j].cut;
}



/* does_attack(ti, tj, ai, aj) returns true if the move at (ti, tj)
 * attacks (ai, aj). This means that it captures the string, and that
 * (ai, aj) is not already dead. As currently written, this function
 * assumes stackp==0, though this could be easily changed.
 */

int
does_attack(int ti, int tj, int ai, int aj)
{
  int color=p[ai][aj];
  int other=OTHER_COLOR(color);
  int result=0;
  int dcode=0;

  assert (stackp==0);
  if ((worm[ai][aj].attacki != -1) && (worm[ai][aj].defendi == -1))
    return 0;

  if (trymove(ti, tj, other, "does_attack-A", ai, aj)) {
    if (!p[ai][aj]) {
      popgo();
      return 1;
    }
    dcode=find_defense(ai, aj, NULL, NULL);
    if (dcode != 1) {
      if (dcode==0)
	result=1;
      else if (dcode==2)
	result=3;
      else if (dcode==3)
	result=4;
      depth++;
      backfill_depth++;
      fourlib_depth++;
      ko_depth++;
      if (result && (worm[ai][aj].defendi != -1) && 
	  (trymove(worm[ai][aj].defendi, worm[ai][aj].defendj, color, "does_attack-B", ai, aj))) {
	int acode=attack(ai, aj, NULL, NULL);
	if (p[ai][aj] && (acode != 1))
	  result=0;
	popgo();
      }
      depth--;
      backfill_depth--;
      fourlib_depth--;
      ko_depth--;
    }
    popgo();
  }
  if (result>worm[ai][aj].attack_code)
    result=0;
  return result;
}

/* does_defend(ti, tj, ai, aj) returns true if the move at (ti, tj)
 * defends (ai, aj). this means that it defends the string, and that
 * (ai, aj) can be captured if no defense is made. As currently
 * written, this function assumes stackp==0, though this could be
 * easily changed.
 */

int
does_defend(int ti, int tj, int ai, int aj)
{
  int color=p[ai][aj];
  int other=OTHER_COLOR(color);
  int result=0;
  int acode, dcode;

  assert (stackp==0);
  if (worm[ai][aj].attacki == -1)
    return 0;

  if (trymove(ti, tj, color, "does_defend-A", ai, aj)) {
    if (p[ai][aj]==EMPTY) {
      popgo();
      return 0;
    }
    acode=attack(ai, aj, NULL, NULL);
    if (acode == 0)
      result=1;
    else if (acode == 1)
      result=0;
    else if (acode == 2)
      result=3;
    else if (acode == 3)
      result=0;
    depth++;
    backfill_depth++;
    fourlib_depth++;
    ko_depth++;
    if (result && (worm[ai][aj].attacki != -1) && 
	trymove(worm[ai][aj].attacki, worm[ai][aj].attackj, other, "does_defend-B", ai, aj)) {
      if (p[ai][aj]==0) {
	result=0;
	popgo();
      } else {
	dcode=find_defense(ai, aj, NULL, NULL);
	if (p[ai][aj] && (dcode != 1))
	  result=0;
	popgo();
      }
    }
    depth--;
    backfill_depth--;
    fourlib_depth--;
    ko_depth--;
    popgo();
  }
  return result;
}




/* The function play_attack_defend_n() plays a sequence of moves,
 * alternating between the players and starting with color. After
 * having played through the sequence, the last coordinate pair gives
 * a target to attack or defend, depending on the value of do_attack.
 * If there is no stone present to attack or defend, it is assumed
 * that it has already been captured. If one or more of the moves to
 * play turns out to be illegal for some reason, the rest of the
 * sequence is played anyway, and attack/defense is tested as if
 * nothing special happened.
 *
 * A typical use for these functions is to set up a ladder in an
 * autohelper and see whether it works or not. */
   
int
play_attack_defend_n(int color, int do_attack, int num_moves, ...)
{
  va_list ap;
  int mcolor = color;
  int success = 0;
  int i;
  int played_moves = 0;
  
  va_start(ap, num_moves);

  /* Do all the moves with alternating colors. */
  /* FIXME: We should really not have to push a board for each move. 
            There should be a trymove() without the call to pushgo(). */
  for (i = 0; i < num_moves; i++) {
    int ai, aj;

    ai=va_arg(ap, int);
    aj=va_arg(ap, int);

    if (trymove(ai, aj, mcolor, "play_attack_defend_n", -1, -1))
      played_moves++;
    mcolor = OTHER_COLOR(mcolor);
  }

  /* Now do the real work. */
  if (i == num_moves)  {
    int zi, zj;

    zi=va_arg(ap, int);
    zj=va_arg(ap, int);
    if (do_attack) {
      if (!p[zi][zj] || attack(zi, zj, NULL, NULL))
	success = 1;
    } else {
      if (p[zi][zj]
	  && (!attack(zi, zj, NULL, NULL)
	      || find_defense(zi, zj, NULL, NULL)))
	success = 1;
    }
  }
  
  /* Pop all the moves we could successfully play. */
  for (i = 0; i < played_moves; i++) {
    popgo();
  }

  va_end(ap);
  return success;
}


/* find_lunch(m, n, *wi, *wj, *ai, *aj) looks for a worm adjoining the
 * string at (m,n) which can be easily captured. Whether or not it can
 * be defended doesn't matter.
 *
 * Returns the location of the string in (*wi, *wj), and the location
 * of the attacking move in (*ai, *aj).
 */
	
int
find_lunch(int m, int n, int *wi, int *wj, int *ai, int *aj)
{
  int i, j, vi, vj, ki, kj;

  ASSERT(p[m][n] != 0, m, n);
  ASSERT(stackp==0, m, n);

  vi = -1;
  vj = -1;
  for (i=0; i<board_size; i++)
    for (j=0; j<board_size; j++)
      if (p[i][j]==OTHER_COLOR(p[m][n]))
	if (((i>0) && worm[i-1][j].origini==worm[m][n].origini 
	     && worm[i-1][j].originj ==worm[m][n].originj)
	    || ((i<board_size-1) && worm[i+1][j].origini==worm[m][n].origini 
		&& worm[i+1][j].originj==worm[m][n].originj)
	    || ((j>0) && worm[i][j-1].origini==worm[m][n].origini
		&& worm[i][j-1].originj==worm[m][n].originj)
	    || ((j<board_size-1) && worm[i][j+1].origini==worm[m][n].origini
		&& worm[i][j+1].originj==worm[m][n].originj))
	  if (((stackp==0) && (worm[i][j].attacki != -1) 
	       && (!worm[i][j].ko))
	      || ((stackp>0) && attack(i, j, &ki, &kj))) {
/*
 * If several adjacent lunches are found, we pick the juiciest.
 * First maximize cutstone, then minimize liberties. We can only
 * do if the worm data is available, i.e. if stackp==0.
 */
	    if ((vi) == -1 || 
		(stackp==0 && (worm[i][j].cutstone > worm[vi][vj].cutstone ||
			       (worm[i][j].cutstone == worm[vi][vj].cutstone &&
				worm[i][j].liberties < worm[vi][vj].liberties)))) {
	      vi = worm[i][j].origini;
	      vj = worm[i][j].originj;
	      if (stackp>0) {
		if (ai) *ai = ki;
		if (aj) *aj = kj;
	      }
	      else 
		{
		  if (ai) *ai=worm[i][j].attacki;
		  if (aj) *aj=worm[i][j].attackj;
		}
	    }
	  }
  if (vi != -1) {
    if (wi) *wi=vi;
    if (wj) *wj=vj;
    return (1);
  }
  else
    return (0);
}




/* return true if the move (i,j) by (color) is a ko capture (whether capture
 * is legal on this move or not) */

int
is_ko(int i, int j, int color)
{
  int other=OTHER_COLOR(color);
  int captures=0;

  if (i>0) {
    if (p[i-1][j] != other)
      return 0;
    if (approxlib(i-1, j, other, 2)==1) {
      if (!singleton(i-1, j))
	return 0;
      captures++;
    }
  }

  if (i<board_size-1) {
    if (p[i+1][j] != other)
      return 0;
    if (approxlib(i+1, j, other, 2)==1) {
      if (!singleton(i+1, j))
	return 0;
      captures++;
    }
  }
  if (j>0) {
    if (p[i][j-1] != other) 
      return 0;
    if (approxlib(i, j-1, other, 2)==1) {
      if (!singleton(i, j-1))
	return 0;
      captures++;
    }
  }
  if (j<board_size-1) {
    if (p[i][j+1] != other)
      return 0;
    if (approxlib(i, j+1,other, 2)==1) {
      if (!singleton(i, j+1))
	return 0;
      captures++;
    }
  }
  if (captures==1)
    return 1;
  else 
    return 0;
}


/* returns true if there is an isolated stone (string of size 1) at (i, j) */

int
singleton(int i, int j)
{
  if (p[i][j]==EMPTY)
    return 0;
  if ((i>0) && (p[i-1][j]==p[i][j]))
    return 0;
  if ((i<board_size-1) && (p[i+1][j]==p[i][j]))
    return 0;
  if ((j>0) && (p[i][j-1]==p[i][j]))
    return 0;
  if ((j<board_size-1) && (p[i][j+1]==p[i][j]))
    return 0;
  return 1;
}


/* This function will detect some blunders. Returns 1 if a move by (color)
 * at (i,j) does not diminish the safety of any worm.
 */

int
confirm_safety(int i, int j, int color, int value)
{
  int libs=countlib(i, j, color);
  int other=OTHER_COLOR(color);
  int issafe=1;
  int m, n;

  TRACE("Checking safety of a move at %m\n", i, j);
  
  if (((i>0) && (p[i-1][j] == color) && (libs<worm[i-1][j].liberties))
       || ((i<board_size-1) && (p[i+1][j] == color) && (libs<worm[i+1][j].liberties))
       || ((j>0) && (p[i][j-1] == color) && (libs<worm[i][j-1].liberties))
       || ((j<board_size-1) && (p[i][j+1] == color) && (libs<worm[i][j+1].liberties)))
    if (trymove(i, j, color, NULL, -1, -1)) {
      for (m=0; m<board_size; m++)
	for (n=0; n<board_size; n++)
	  if (issafe && p[m][n] && (worm[m][n].origini==m) && (worm[m][n].originj==n))
	    if (((p[m][n]==color)
		 && (worm[m][n].attacki == -1) 
		 && (worm[m][n].value > value)
		 && attack(m, n, NULL, NULL))
		|| ((p[m][n]==other)
		    && (worm[m][n].attacki != -1)
		    && (worm[m][n].defendi == -1)
		    && (worm[m][n].value > value)
		    && find_defense(m, n, NULL, NULL)))
	      issafe = 0;
      popgo();
    }
  return issafe;
}
      
		
    




/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
