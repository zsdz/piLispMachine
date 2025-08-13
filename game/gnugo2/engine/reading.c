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

#define FIND_DEFENSE    0
#define DEFEND1         1
#define DEFEND2         2
#define DEFEND3         3
#define DEFEND4         7 

#define ATTACK          4
#define ATTACK2         5
#define ATTACK3         6

#define UNUSED(x)  x=x

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "liberty.h"
#include "hash.h"
#include <assert.h>


/*
 * The functions in reading.c are used to read whether groups 
 * can be captured or not. See the Texinfo documentation 
 * (Reading) for more information.
 *
 * NULL POINTERS: Many functions in this file can use pointers
 * to return the locations of recommended plays. These can be
 * set NULL in which case these values are not returned.  */



static void chain(int i, int j, char ma[MAX_BOARD][MAX_BOARD],
		  char ml[MAX_BOARD][MAX_BOARD], int *adj, int adji[MAXCHAIN],
		  int adjj[MAXCHAIN], int adjsize[MAXCHAIN], 
		  int adjlib[MAXCHAIN], char mark);
static int relative_break_chain(int, int, int *, int *, int, int);
static int special_rescue(int si, int sj, int ai, int aj, int *ti, int *tj);
static void order3(int *, int *, int *, int *, int *, int *, int , int );
static int naive_ladder_defense(int si, int sj, int ai, int aj,
				int bi, int bj, int color, int other);
static int naive_ladder_break_through(int si, int sj, int ai, int aj,
				      int color, int other);

/* ================================================================ */

#if TRACE_READ_RESULTS

#define TRACE_CACHED_RESULT(rr) \
      gprintf("%o%s %m %d %d %d %d (cached) ", read_function_name, \
	      qi, qj, stackp, \
	      rr_get_result(rr), \
	      rr_get_result_i(rr), \
	      rr_get_result_j(rr)); \
      dump_stack();

#define SETUP_TRACE_INFO(name, si, sj) \
  const char *read_function_name = name; \
  int qi, qj; \
  find_origin(si, sj, &qi, &qj);

#else

#define TRACE_CACHED_RESULT(rr)
#define SETUP_TRACE_INFO(name, si, sj)

#endif


/*
 * Return a Read_result for the current position, routine and location.
 * For performance, the location is changed to the origin of the string.
 */

static int
get_read_result(/*int color, */int routine, int *si, int *sj, 
		Read_result **read_result)
{
  Hashnode      * hashnode;
  int             retval;

#if HASHING
#if CHECK_HASHING
  Hashposition    pos;
  unsigned long   hash;

  UNUSED(color);

  /* Check the hash table to see if we have had this position before. */
  board_to_position(p, ko_i, ko_j, /*color,*/ &pos);
  hash = board_hash(p, ko_i, ko_j/*, color*/);
  assert(hash == hashitem.hashval);
  assert(hashposition_compare(&pos, &(hashitem.hashpos)) == 0);

  /* Find this position in the table.  If it wasn't found, enter it. */
  hashnode = hashtable_search(movehash, &pos, hash);
  if (hashnode != NULL) {
    stats.position_hits++;
    RTRACE("We found position %d in the hash table...\n", hash);
  } else
    hashnode = hashtable_enter_position(movehash, &pos, hash);
#else
  /* Find this position in the table.  If it wasn't found, enter it. */
  hashnode = hashtable_search(movehash, &(hashdata.hashpos), hashdata.hashval);
  if (hashnode != NULL) {
    stats.position_hits++;
    RTRACE("We found position %d in the hash table...\n", hashdata.hashval);
  } else {
    hashnode = hashtable_enter_position(movehash,
					&(hashdata.hashpos), hashdata.hashval);
    if (hashnode)
      RTRACE("Created position %d in the hash table...\n", hashdata.hashval);
  }
#endif

#else  /* HASHING */
  hashnode = NULL;
#endif

  retval = 0;
  if (hashnode == NULL) {
    /* No hash node, so we can't enter a result into it. */
    *read_result = NULL;

  } else {

    /* We found it!  Now see if we can find a previous result.
     * First, find the origin of the string containing (si, sj),
     * in order to make the caching of read results work better.
     */
    find_origin(*si, *sj, si, sj);

    *read_result = hashnode_search(hashnode, routine, *si, *sj);

    if (*read_result != NULL) {
      stats.read_result_hits++;
      retval = 1;
    } else {
      RTRACE("...but no previous result for routine %d and (%m)...",
	     routine, *si, *sj);

      /* Only store the result if stackp <= depth. Above that, there
	 is no branching, so we won't gain anything. */
      if (stackp > depth) 
	*read_result = NULL;
      else {
	*read_result = hashnode_new_result(movehash, hashnode, 
					   routine, *si, *sj);
	if (*read_result == NULL)
	  RTRACE("%o...and unfortunately there was no room for one.\n");
	else
	  RTRACE("%o...so we allocate a new one.\n");
      } 
    }
  }

  return retval;
}


/* ================================================================ */


/*
 * These macros should be used in all the places where we want to
 * return a result from a reading function and where we want to
 * store the result in the hash table at the same time.
 */
#if !TRACE_READ_RESULTS

#define READ_RETURN0(read_result) \
  do { \
    if (read_result) \
      rr_set_result_ri_rj(*(read_result), 0, 0, 0); \
    return 0; \
  } while (0)

#define READ_RETURN(read_result, pointi, pointj, resulti, resultj, value) \
  do { \
    if ((value) != 0 && (pointi) != 0) *(pointi)=(resulti); \
    if ((value) != 0 && (pointj) != 0) *(pointj)=(resultj); \
    if (read_result) \
      rr_set_result_ri_rj(*(read_result), (value), (resulti), (resultj)); \
    return (value); \
  } while (0)

#else

#define READ_RETURN0(read_result) \
  do { \
    if (read_result) \
      rr_set_result_ri_rj(*(read_result), 0, 0, 0); \
    gprintf("%o%s %m %d 0 0 0 ", read_function_name, qi, qj, stackp); \
    dump_stack(); \
    return 0; \
  } while (0)

#define READ_RETURN(read_result, pointi, pointj, resulti, resultj, value) \
  do { \
    if ((value) != 0 && (pointi) != 0) *(pointi)=(resulti); \
    if ((value) != 0 && (pointj) != 0) *(pointj)=(resultj); \
    if (read_result) \
      rr_set_result_ri_rj(*(read_result), (value), (resulti), (resultj)); \
    gprintf("%o%s %m %d %d %d %d ", read_function_name, qi, qj, stackp, \
	    (value), (resulti), (resultj)); \
    dump_stack(); \
    return (value); \
  } while (0)

#endif
  
/* =================== Defensive functions ==================== */


/* find_defense(m, n, *i, *j) attempts to find a move that will save
 * The string at (m,n). It returns true if such a move is found, with
 * (*i, *j) the location of the saving move, unless (*i, *j) are
 * null pointers. It is not checked that tenuki defends, so this may 
 * give an erroneous answer if !attack(m,n).
 * 
 * Returns 2 or 3 if the result depends on ko. Returns 2 if the
 * string can be defended provided (color) is willing to ignore
 * any ko threat. Returns 3 if (color) has a ko threat which must
 * be answered.  */

int 
find_defense(int m, int n, int *i, int *j)
{
  int di, dj;
  int can_save;
  int mylib=approxlib(m, n, p[m][n], 5);
  int             found_read_result;
  Read_result   * read_result;
  
  SETUP_TRACE_INFO("find_defense", m, n);
  
  RTRACE("Can we rescue %m?\n", m, n);

  /* We first check if the number of liberties is larger than four. In
   * that case we don't cache the result and to avoid needlessly
   * storing the position in the hash table, we must do this test
   * before we look for cached results.
   */

  if (mylib>4)   /* no need cache result in this case */
    return (1);

  
  if ((stackp <= depth) && (hashflags & HASH_FIND_DEFENSE)) {
    found_read_result = get_read_result(/*color, */FIND_DEFENSE,
					&m, &n, &read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, FIND_DEFENSE, m, n, stackp);
    }
  } else
    read_result = NULL;


  if (mylib==1) {
    can_save=(defend1(m, n, &di, &dj));
    READ_RETURN(read_result, i, j, di, dj, can_save);
  }
  else if (mylib==2) {
    can_save=defend2(m, n, &di, &dj);
    if (can_save) {
      RTRACE("saving move for %m found at %m!\n", m, n, di, dj);
      READ_RETURN(read_result, i, j, di, dj, can_save);
    }
  }
  else if (mylib==3) {
    if (stackp > depth) 
      return 1;
    if (defend3(m, n, &di, &dj)) {
      RTRACE("saving move for %m found at %m!\n", m, n, di, dj);
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
  }
  else if (mylib==4) {
    if (stackp > depth) 
      return 1;
    if (defend4(m, n, &di, &dj)) {
      RTRACE("saving move for %m found at %m!\n", m, n, di, dj);
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
  }

  READ_RETURN0(read_result);
}


/* If si, sj points to a string with exactly one liberty defend1 
 * determines whether it can be saved by extending or capturing
 * a boundary chain having one liberty. The function returns 1 if the string
 * can be saved, otherwise 0. It returns 2 or 3 if it can be saved, conditioned
 * on ko. Returns 2 if it can be saved provided (color) is willing to
 * ignore any ko threat. Returns 3 if it can be saved if (color) has
 * a ko threat which must be answered.
 *
 * The pair defend1-attack2 call each other recursively to
 * read situations such as ladders. They read all ladders to the end.
 * If the reading ply (stackp) is deeper than the deep-reading cutoff
 * parameter depth, whose default value DEPTH is defined in liberty, then a
 * string is assumed alive if it can get 3 liberties. When 
 * fourlib_depth < stackp < depth, a string is considered alive if it can get
 * four liberties. When stackp < fourlib_depth, it is considered alive
 * if it can get 5 liberties.
 *
 */

int 
defend1(int si, int sj, int *i, int *j) 
{
  int color;
  int di, dj;
  int bcode;
  int savei=-1, savej=-1;
  int saveresult=0;
  int liberties, can_catch;
  int             found_read_result;
  Read_result   * read_result;

  SETUP_TRACE_INFO("defend1", si, sj);
  
  DEBUG(DEBUG_LADDER, "defend1(%m)\n", si, sj);
  assert(p[si][sj]!=EMPTY);
  if (approxlib(si,sj,p[si][sj],2)!=1) {
    verbose=4;
    dump_stack();
    abort();
  }

  RTRACE("try to escape atari on %m.\n",  si, sj);

  color=p[si][sj];

  if ((stackp <= depth) && (hashflags & HASH_DEFEND1)) {
  
    found_read_result = get_read_result(/*color, */DEFEND1, &si, &sj,
					&read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, DEFEND1, si, sj, stackp);
    }
  } else
    read_result = NULL;

  /* If we can capture a bounding chain and live, then do so. */
  bcode=break_chain(si, sj, &di, &dj, NULL, NULL);
  if (bcode) {
    if (bcode != 1) {
      savei = di;
      savej = dj;
      saveresult=bcode;
    }
    else {
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
  }
  
  /* (di, dj) will be the liberty of the string. */
  approxlib(si, sj, color, 2);
  ASSERT(lib==1, si, sj);
  di=libi[0];
  dj=libj[0];

  /* Try to extend along our own liberty. */
  RTRACE("extending to %m.\n", di, dj);
  if (trymove(di, dj, color, "defend1-A", si, sj)) {
    liberties=approxlib(di, dj, color, 5);

    /* If we are still in atari, or have no liberties at all, we lose. */

    if (liberties < 2) {
      if (savei == -1) {
	RTRACE("still atari : it dies!\n");
	popgo();
	READ_RETURN0(read_result);
      }
      else {
	RTRACE("saved by ko at %m\n", savei, savej);
	popgo();
	READ_RETURN(read_result, i, j, savei, savej, saveresult);
      }
    }
    
    /* If we are still within the deep reading cutoff, then read further
     * even if the group has 3 liberties. 
     */

    if ((stackp <= depth) && (liberties==3)) {

      /* If the group can be killed in spite of the 3 liberties, return 0. */

      if (attack3(si, sj, NULL, NULL)) {
	if (savei == -1) {
	  RTRACE("it dies!\n");
	  popgo();
	  READ_RETURN0(read_result);
	}
	else {
	  RTRACE("saved by ko at %m\n", savei, savej);
	  popgo();
	  READ_RETURN(read_result, i, j, savei, savej, saveresult);
	}
      } else {
	RTRACE("it lives at %m!\n", di, dj);
	popgo();
	READ_RETURN(read_result, i, j, di, dj, 1);
      }
    }
    else if ((stackp <= fourlib_depth) && (liberties==4)) {
      if (attack4(si, sj, NULL, NULL)) {
	if (savei == -1) {
	  RTRACE("it dies!\n");
	  popgo();
	  READ_RETURN0(read_result);
	}
	else {
	  RTRACE("saved by ko at %m\n", savei, savej);
	  popgo();
	  READ_RETURN(read_result, i, j, savei, savej, saveresult);
	}
      } else {
	RTRACE("it lives at %m!\n", di, dj);
	popgo();
	READ_RETURN(read_result, i, j, di, dj, 1);
      }
    }

    /* If we are here and liberties is >2 (must be 3), then stackp must
     * be bigger than depth, and then we consider 3 liberties to live.  
     */

    if (liberties>2) {
      RTRACE("it lives at %m!\n", di, dj);
      popgo();
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
    
    /* If we are here, then liberties must be 2 and we don't care if
       stackp is greater than depth. We continue the ladder anyway. */
    ASSERT(liberties==2, si, sj);
    
    can_catch=attack2(si, sj, NULL, NULL);
    if (can_catch==1) {
      if (savei == -1) {
	RTRACE("DEAD!\n");
	popgo();
	READ_RETURN0(read_result);
      }
      else {
	popgo();
	READ_RETURN(read_result, i, j, savei, savej, saveresult);
      }
    } else if (can_catch==2) {
      RTRACE("START KO FIGHT at %m!\n", di, dj);
      popgo();
      READ_RETURN(read_result, i, j, di, dj, 3);
    } else if (can_catch==3) {
      RTRACE("START KO FIGHT at %m!\n", di, dj);
      popgo();
      READ_RETURN(read_result, i, j, di, dj, 2);
    } else {
      RTRACE("ALIVE at %m!\n", di, dj);
      popgo();
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
  }

  /* If we get here, then trymove() failed => the ladder works. */
  RTRACE( "Cannot extend : die in ladder\n");

  if (savei == -1) {
    READ_RETURN0(read_result);
  }
  else
    READ_RETURN(read_result, i, j, savei, savej, saveresult);
}


/* If si, sj points to a group with two liberties, defend2 determines
 * whether the group can be saved by extending, or by capturing part of
 * its surrounding chain. A group is considered safe if either part of
 * the surrounding chain may be captured, or if it can get 3
 * liberties. It is presumed that the opponent could kill if tenuki.
 * If both extensions work, it prefers the one which maximizes 
 * liberties.
 *
 * i and j return the move to save the stones. Can be NULL if caller
 * is not interested in the details.
*/

int 
defend2(int si, int sj, int *i, int *j)
{
  int color, other, ai, aj, bi, bj, ci, cj, firstlib, secondlib;
  int savei=-1, savej=-1;
  int savecode=0;
  int acount=0, bcount=0;
  int bc=0;
  int             found_read_result;
  Read_result   * read_result;

  SETUP_TRACE_INFO("defend2", si, sj);
  
  RTRACE("trying to rescue %m\n",  si, sj);
  color=p[si][sj];
  other=OTHER_COLOR(color);

  assert(p[si][sj]!=EMPTY);
  assert(approxlib(si,sj,p[si][sj],3)==2);

  if ((stackp <= depth) && (hashflags & HASH_DEFEND2)) {
  
    found_read_result = get_read_result(/*color, */DEFEND2,
					&si, &sj, &read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, DEFEND2, si, sj, stackp);
    }
  } else
    read_result = NULL;

  /* It is best to defend by capturing part of the surrounding chain
   * if this is possible. But simply moving the break_chain tests to
   * the front results in thrashing. As a compromise, we try to
   * capture part of the surrounding chain first if stackp==0, 
   * otherwise, we try this last.
   */

  bc=break_chain(si, sj, &ci, &cj, NULL, NULL);
  if (bc == 1)
    READ_RETURN(read_result, i, j, ci, cj, 1);

  if (bc == 2) {
    savei=ci;
    savej=cj;
  }
  approxlib(si, sj, color, 3);
  ASSERT(lib==2, si, sj);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];

  /* if (bi, bj) looks more promising we wish to switch the two liberties.
     We check whether (bi, bj) is adjacent to more open liberties than (ai,aj).
  */

  if ((ai>0) && p[ai-1][aj]==EMPTY)
    acount++;
  if ((ai<board_size-1) && p[ai+1][aj]==EMPTY)
    acount++;
  if ((aj>0) && p[ai][aj-1]==EMPTY)
    acount++;
  if ((aj<board_size-1) && p[ai][aj+1]==EMPTY)
    acount++;
  if ((bi>0) && p[bi-1][bj]==EMPTY)
    bcount++;
  if ((bi<board_size-1) && p[bi+1][bj]==EMPTY)
    bcount++;
  if ((bj>0) && p[bi][bj-1]==EMPTY)
    bcount++;
  if ((bj<board_size-1) && p[bi][bj+1]==EMPTY)
    bcount++;

  if (bcount>acount) {
    ai=libi[1];
    aj=libj[1];
    bi=libi[0];
    bj=libj[0];
  }

  RTRACE("trying extension to %m\n", ai, aj);
  if (trymove(ai, aj, color, "defend2-A", si, sj)) {
    firstlib=approxlib(si, sj, color, 5);
    if ((firstlib==2) && (attack2(si, sj, NULL, NULL))) firstlib=0;
    if ((stackp <= depth) && (firstlib==3) 
	&& (attack3(si, sj, NULL, NULL))) firstlib=0;
    if ((stackp <= fourlib_depth) && (firstlib==4) 
	&& (attack4(si, sj, NULL, NULL))) firstlib=0;
    popgo();
    if (firstlib>1) {
      RTRACE("%m rescues\n", ai, aj);
      READ_RETURN(read_result, i, j, ai, aj, 1);
    }
    else 
      RTRACE("%m does not rescue. Is other move better?\n", ai, aj);
  } else {
    RTRACE("%m isn't a legal move!\n", ai, aj);
    firstlib=0;
  }
  if (firstlib<2 || i) {
    RTRACE("trying extension to %m\n", bi, bj);
    if (trymove(bi, bj, color, "defend2-B", si, sj)) {
      secondlib=approxlib(si, sj, color, 5);
      if ((firstlib>=secondlib)&&(firstlib>2))
	{
	  RTRACE("%m is not superior to first move\n",  bi, bj);
	  popgo();
	  READ_RETURN(read_result, i, j, ai, aj, 1);
	}
      if ((secondlib==2) && (attack2(si, sj, NULL, NULL))) secondlib=0;
      if ((stackp <= depth) && (secondlib==3) 
	  && (attack3(si, sj, NULL, NULL))) secondlib=0;
      if ((stackp <= fourlib_depth) && (secondlib==4) 
	  && (attack4(si, sj, NULL, NULL))) secondlib=0;
      if ((secondlib>=firstlib) && (secondlib>1)) {
	if (firstlib>1)
	  RTRACE("%m also rescues, we'll use it\n", bi, bj);
	else
	  RTRACE("%m rescues, we'll use it\n",  bi, bj);
	popgo();
	READ_RETURN(read_result, i, j, bi, bj, 1);
      }
      if (firstlib>1) {
	RTRACE("%m doesn't work, use first move\n", bi, bj);
	popgo();
	READ_RETURN(read_result, i, j, ai, aj, 1);
      }
      popgo();
    }
  } 

  /* This used to be conditioned on stackp < depth, but let's
   * try it unconditionally for a while. This fixes incident 64.
   */

  if (1) {
    int bcode=break_chain2(si, sj, &ci, &cj);
    if (bcode) {
      READ_RETURN(read_result, i, j, ci, cj, bcode);
    }
  }
  else {
    if (double_atari_chain2(si, sj, &ci, &cj))
      READ_RETURN(read_result, i, j, ci, cj, 1);
  }
  
  {
    int xi, xj;

    if ((stackp <= depth) && (special_rescue(si, sj, ai, aj, &xi, &xj))) {
      READ_RETURN(read_result, i, j, xi, xj, 1);
    }
  
    if ((stackp <= depth) && (special_rescue(si, sj, bi, bj, &xi, &xj))) {
      READ_RETURN(read_result, i, j, xi, xj, 1);
    }
  }

  /* In a situation like this:
   *       
   *   OOXXXX     the following code can find the
   *   .OXOOX     defensive move at 'c'.
   *   .cO.OX
   *   .X.OOX
   *   ------
   */

  if ((stackp <= backfill_depth) 
      && (approxlib(ai, aj, other, 1)==0) 
      && (approxlib(ai, aj, color, 3)==2)) {
    if ((libi[0] != bi) || (libj[0] != bj)) {
      ci=libi[0];
      cj=libj[0];
    } else {
      ci=libi[1];
      cj=libj[1];
    }
    if (trymove(ci, cj, color, "defend2-C", si, sj)) {
      int acode = attack(si, sj, NULL, NULL);
      if (acode != 1) {
	if (acode == 0) {
	  popgo();
	  RTRACE("ALIVE!!\n");
	  READ_RETURN(read_result, i, j, ci, cj, 1);
	}
	else if (acode == 2)
	  savecode=3;
	else if (acode == 3)
	  savecode=2;
	savei=ci;
	savej=cj;
      }
      popgo();
    }
  }
  if ((stackp <= backfill_depth) 
      && (approxlib(bi, bj, other, 1)==0) 
      && (approxlib(bi, bj, color, 3)==2)) {
    if ((libi[0] != ai) || (libj[0] != aj)) {
      ci=libi[0];
      cj=libj[0];
    } else {
      ci=libi[1];
      cj=libj[1];
    }
    if (trymove(ci, cj, color, "defend2-C", si, sj)) {
      int acode = attack(si, sj, NULL, NULL);
      if (acode != 1) {
	if (acode == 0) {
	  RTRACE("ALIVE!!\n");
	  popgo();
	  READ_RETURN(read_result, i, j, ci, cj, 1);
	}
	else if (acode == 2)
	  savecode=3;
	else if (acode == 3)
	  savecode=2;
	savei=ci;
	savej=cj;
      }
      popgo();
    }
  }


  if (savei != -1)
    READ_RETURN(read_result, i, j, savei, savej, savecode);

  RTRACE("failed to find rescuing move.\n");
  READ_RETURN0(read_result);
}


/* defend3(si, sj, *i, *j) attempts to find a move rescuing the 
 * string at (si, sj) with 3 liberties.  If such a move can be found,
 * it returns true, and if the pointers i, j are not NULL, 
 * then it returns the saving move in (*i, *j).
 */

int 
defend3(int si, int sj, int *i, int *j)
{
  int color, ai, aj, bi, bj, ci, cj;
  int savei=-1, savej=-1;
  int bc=0;
  int             found_read_result;
  Read_result   * read_result;


  SETUP_TRACE_INFO("defend3", si, sj);

  RTRACE("trying to rescue %m\n",  si, sj);
  color=p[si][sj];

  assert(p[si][sj]!=EMPTY);
  assert(approxlib(si,sj,p[si][sj],4)==3);

  /* If we can capture a surrounding string, this is the best way to
     defend. */

  if ((stackp <= depth) && (hashflags & HASH_DEFEND3)) {
    found_read_result = get_read_result(/*color, */DEFEND3, 
					&si, &sj, &read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, DEFEND3, si, sj, stackp);
    }
  } else
    read_result = NULL;

  bc=break_chain(si, sj, &ai, &aj, &ci, &cj);
  if (bc == 1)
    READ_RETURN(read_result, i, j, ai, aj, 1);

  /* We prefer not to sacrifice unnecessarily. If the attacking
   * move works but can itself be captured, we cache its location
   * in (savei, savej). Then if we cannot find a better way, we 
   * use it at the end.
   */       

  if (bc == 2) {
    savei=ai;
    savej=aj;
  }

  /* Get the three liberties into (ai, aj), (bi, bj) and (ci, cj). */
  approxlib(si, sj, color, 4);
  ASSERT(lib==3, si, sj);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];
  ci=libi[2];
  cj=libj[2];
  order3(&ai, &aj, &bi, &bj, &ci, &cj, si, sj);

  /* before trying seriously, check if there is a simple solution. */

  if (stackp > fourlib_depth) {
    if (approxlib(ai, aj, color, 4) > 3)
      READ_RETURN(read_result, i, j, ai, aj, 1);
    if (approxlib(bi, bj, color, 4) > 3) 
      READ_RETURN(read_result, i, j, bi, bj, 1);
    if (approxlib(ci, cj, color, 4) > 3)
      READ_RETURN(read_result, i, j, ci, cj, 1);
  }
  else {
    if (approxlib(ai, aj, color, 5) > 4)
      READ_RETURN(read_result, i, j, ai, aj, 1);
    if (approxlib(bi, bj, color, 5) > 4)
      READ_RETURN(read_result, i, j, bi, bj, 1);
    if (approxlib(ci, cj, color, 5) > 4)
      READ_RETURN(read_result, i, j, ci, cj, 1);
  }

  /* Nope, no quick solution available. Try reading further instead. */

  RTRACE("trying extension to %m\n", ai, aj);
  if (trymove(ai, aj, color, "defend3-A", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, ai, aj, 1);
    }
    popgo();
  } 

  RTRACE("trying extension to %m\n", bi, bj);
  if (trymove(bi, bj, color, "defend3-B", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, bi, bj, 1);
    }
    popgo();
  } 

  RTRACE("trying extension to %m\n", ci, cj);
  if (trymove(ci, cj, color, "defend3-C", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, ci, cj, 1);
    }
    popgo();
  } 

  if ((stackp <= backfill_depth) && break_chain2(si, sj, &ai, &aj))
    READ_RETURN(read_result, i, j, ai, aj, 1);

  if (savei != -1)
    READ_RETURN(read_result, i, j, savei, savej, 1);

  RTRACE("failed to find rescuing move.\n");
  READ_RETURN0(read_result);
}



/* defend4(si, sj, *i, *j) attempts to find a move rescuing the 
 * string at (si, sj) with 4 liberties.  If such a move can be found,
 * it returns true, and if the pointers i, j are not NULL, 
 * then it returns the saving move in (*i, *j).
 */

int 
defend4(int si, int sj, int *i, int *j)
{
  int color, ai, aj, bi, bj, ci, cj, di, dj;
  int savei=-1, savej=-1;
  int bc=0;
  int             found_read_result;
  Read_result   * read_result;
  
  SETUP_TRACE_INFO("defend4", si, sj);

  RTRACE("trying to rescue %m\n",  si, sj);
  color=p[si][sj];

  assert(p[si][sj]!=EMPTY);
  assert(approxlib(si,sj,p[si][sj],5)==4);

  /* If we can capture a surrounding string, this is the best way to
     defend. */

  if ((stackp <= depth) && (hashflags & HASH_DEFEND4)) {
    found_read_result = get_read_result(/*color, */DEFEND4, 
					&si, &sj, &read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, DEFEND4, si, sj, stackp);
    }
  } else
    read_result = NULL;

  bc=break_chain(si, sj, &ai, &aj, &ci, &cj);
  if (bc == 1)
    READ_RETURN(read_result, i, j, ai, aj, 1);

  /* We prefer not to sacrifice unnecessarily. If the attacking
   * move works but can itself be captured, we cache its location
   * in (savei, savej). Then if we cannot find a better way, we 
   * use it at the end.
   */       

  if (bc == 2) {
    savei=ai;
    savej=aj;
  }

  approxlib(si, sj, color, 4);
  ASSERT(lib==4, si, sj);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];
  ci=libi[2];
  cj=libj[2];
  di=libi[3];
  dj=libj[3];

  /* before trying seriously, check if there is a simple solution. */

  if (stackp > fourlib_depth) {
    if (approxlib(ai, aj, color, 4) > 3)
      READ_RETURN(read_result, i, j, ai, aj, 1);
    if (approxlib(bi, bj, color, 4) > 3) 
      READ_RETURN(read_result, i, j, bi, bj, 1);
    if (approxlib(ci, cj, color, 4) > 3)
      READ_RETURN(read_result, i, j, ci, cj, 1);
    if (approxlib(di, dj, color, 4) > 3)
      READ_RETURN(read_result, i, j, di, dj, 1);
  }
  else {
    if (approxlib(ai, aj, color, 5) > 4)
      READ_RETURN(read_result, i, j, ai, aj, 1);
    if (approxlib(bi, bj, color, 5) > 4)
      READ_RETURN(read_result, i, j, bi, bj, 1);
    if (approxlib(ci, cj, color, 5) > 4)
      READ_RETURN(read_result, i, j, ci, cj, 1);
    if (approxlib(di, dj, color, 5) > 4)
      READ_RETURN(read_result, i, j, di, dj, 1);
  }

  /* Nope, no quick solution available. Try reading further instead. */

  RTRACE("trying extension to %m\n", ai, aj);
  if (trymove(ai, aj, color, "defend4-A", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, ai, aj, 1);
    }
    popgo();
  } 

  RTRACE("trying extension to %m\n", bi, bj);
  if (trymove(bi, bj, color, "defend4-B", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, bi, bj, 1);
    }
    popgo();
  } 

  RTRACE("trying extension to %m\n", ci, cj);
  if (trymove(ci, cj, color, "defend4-C", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, ci, cj, 1);
    }
    popgo();
  } 

  RTRACE("trying extension to %m\n", di, dj);
  if (trymove(di, dj, color, "defend4-D", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      READ_RETURN(read_result, i, j, di, dj, 1);
    }
    popgo();
  } 

  if ((stackp <= backfill_depth) && break_chain2(si, sj, &ai, &aj))
    READ_RETURN(read_result, i, j, ai, aj, 1);

  if (savei != -1)
    READ_RETURN(read_result, i, j, savei, savej, 1);

  RTRACE("failed to find rescuing move.\n");    
  READ_RETURN0(read_result);
}




/*
 * special_rescue(si, sj, ai, aj, *ti, *tj) is called with (si, sj) a
 * string having a liberty at (ai, aj). The saving move is returned
 * in (*ti, *tj).
 *
 * This checks whether there is a rescuing move in the following shape:
 *
 *   .
 *  O.*
 *   O
 *
 * This will occasionally save a string where no other move will.
 */

static int
special_rescue(int si, int sj, int ai, int aj, int *ti, int *tj)
{
  int n=0;
  int other=OTHER_COLOR(p[si][sj]);
  int ui=-1, uj=-1, vi=-1, vj=-1;

  /* Not applicable at the edge. */
  if ((ai==0) 
      || (ai==board_size-1) 
      || (aj==0)
      || (aj==board_size-1))
    return 0;

  /* See which neighbours to the liberty (ai, aj) are of the same color 
     as (si, sj).  Count these.  There must be no stones of other color
     and the stones of the own color must add up to 2. After this, 
     (ui, uj) and (vi, vj) will be the two liberties which we
     can try later to see if the special saving move works. */
  if (p[ai-1][aj]==other)
    return 0;
  else if (p[ai-1][aj]==p[si][sj])
    n++;
  else {
    if (ui == -1) {
      ui=ai-1;
      uj=aj;
    }
    else {
      vi=ai-1;
      vj=aj;
    }
  }

  if (p[ai+1][aj]==other)
    return 0;
  else if (p[ai+1][aj]==p[si][sj])
    n++;
  else {
    if (ui == -1) {
      ui=ai+1;
      uj=aj;
    }
    else {
      vi=ai+1;
      vj=aj;
    }
  }

  if (p[ai][aj-1]==other)
    return 0;
  else if (p[ai][aj-1]==p[si][sj])
    n++;
  else {
    if (ui == -1) {
      ui=ai;
      uj=aj-1;
    }
    else {
      vi=ai;
      vj=aj-1;
    }
  }

  if (p[ai][aj+1]==other)
    return 0;
  else if (p[ai][aj+1]==p[si][sj])
    n++;
  else {
    if (ui == -1) {
      ui=ai;
      uj=aj+1;
    }
    else {
      vi=ai;
      vj=aj+1;
    }
  }

  /* If wrong number of friendly neighbours, the pattern does not fit. */
  if (n != 2)
    return 0;

  /* We now have two candidates for the saving move. Try each of them. */
  if (trymove(ui, uj, p[si][sj], "special_rescue2-A", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      if (ti) *ti=ui;
      if (tj) *tj=uj;
      return 1;
    }
    popgo();
  }
  if (trymove(vi, vj, p[si][sj], "special_rescue2-B", si, sj)) {
    if (!attack(si, sj, NULL, NULL)) {
      popgo();
      if (ti) *ti=vi;
      if (tj) *tj=vj;
      return 1;
    }
    popgo();
  }

  return 0;
}






/* =================== Aggressive functions ==================== */


/* attack(m, n, *i, *j) determines if the string at (m, n) can be 
 * attacked, and if so, (*i, *j) returns the attacking move, unless
 * (*i, *j) are null pointers. Use null pointers if you are interested
 * in the result of the attack but not the attacking move itself.
 *
 * Return 1 if the attack succeeds, otherwise 0. Returns 2 or 3
 * if the result depends on ko: returns 2 if the attack succeeds
 * provided attacker is willing to ignore any ko threat. Returns 3
 * if attack succeeds provided attacker has a ko threat which must be 
 * answered.  
 */

int
attack(int m, int n, int *i, int *j)
{
  int color = p[m][n];
  int other = OTHER_COLOR(color);
  int xi, xj;
  int libs;
  int result;
  int             found_read_result;
  Read_result   * read_result;

  SETUP_TRACE_INFO("attack", m, n);

  ASSERT(color != 0, m, n);

  if (color == 0)      /* if assertions are turned off, silently fails */
    return (0);

  if ((stackp <= depth) && (hashflags & HASH_ATTACK)) {
    found_read_result = get_read_result(/*color, */ATTACK, &m, &n, 
					&read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, ATTACK, m, n, stackp);
    }
  } else
    read_result = NULL;

  /* Treat the attack differently depending on how many liberties the 
     string at (m, n) has. */
  libs=approxlib(m, n, p[m][n], 5);
  xi=libi[0];
  xj=libj[0];

  if (libs == 1) {

    /* if the move is an illegal ko capture, attack should return 3 */

    if (!legal(xi, xj, other) && is_ko(xi, xj, other))
      READ_RETURN(read_result, i, j, xi, xj, 3);

    /* (m,n) has one liberty. We can capture it, but it's a
     * waste of time if it is a snapback.
     * It is bad to rely on this, but snapback returns 1 if
     * the move at i,j is illegal, so net result will
     * be return of 0 if is illegal, which is what we want.
     */

    if (singleton(m, n))
      result = !snapback(xi, xj, xi, xj, other);
    else
      result = 1;
    READ_RETURN(read_result, i, j, xi, xj, result);
  } 
  else if (libs == 2) {

    /* attack2() can return 0, 1 or 2. */
    result = attack2(m, n, &xi, &xj);
    if (result)
      READ_RETURN(read_result, i, j, xi, xj, result);

  } else if (libs == 3) {

    result =  attack3(m, n, &xi, &xj);
    if (result)
      READ_RETURN(read_result, i, j, xi, xj, result);

  } else if ((stackp <= fourlib_depth) && (libs == 4)) {
    result =  attack4(m, n, &xi, &xj);
    if (result)
      READ_RETURN(read_result, i, j, xi, xj, result);
  }

  /* More than 4 liberties.  The attack fails. 
   * (This is an oversimplification, but it's the one we use here.)
   */
  READ_RETURN0(read_result);
}


/* If si, sj points to a group with exactly two liberties
 * attack2 determines whether it can be captured in ladder or net.
 * If yes, *i, *j is the killing move. i & j may be null if caller 
 * is only interested in whether it can be captured.
 *  
 * Returns 2 or 3 if it can be killed conditioned on ko. Returns
 * 2 if it can be killed provided (other) is willing to ignore
 * any ko threat. Returns 3 if (other) has a ko threat which
 * must be answered.
 *
 * See the comment before defend1 about ladders and reading depth.
 */

int 
attack2(int si, int sj, int *i, int *j) 
{
  int color, other;
  int ai, aj, bi, bj, ci, cj;
  int di, dj, gi, gj, hi, hj;
  int liberties, r, can_save;
  int acount = 0, bcount = 0;
  int adj, adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  int savei = -1, savej = -1;
  int savecode=0;
  int             found_read_result;
  Read_result   * read_result;

  SETUP_TRACE_INFO("attack2", si, sj);
  find_origin(si, sj, &si, &sj);
  assert(p[si][sj]!=EMPTY);
  assert(approxlib(si,sj,p[si][sj],3)==2);
  DEBUG(DEBUG_LADDER, "attack2(%m)\n", si, sj);

  RTRACE("checking attack on %m with 2 liberties\n", si, sj);

  color=p[si][sj];
  other=OTHER_COLOR(color);

  if ((stackp <= depth) && (hashflags & HASH_ATTACK2)) {
  
    found_read_result = get_read_result(/*color, */ATTACK2, &si, &sj,
					&read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, ATTACK2, si, sj, stackp);
    }
  } else
    read_result = NULL;

  /* The attack may fail if a boundary string is in atari and cannot 
   * be defended.  First we must try defending such a string. 
   *
   * We start by just trying to extend the boundary string at
   * (adji[r], adji[r]) to (hi, hj). If that doesn't work, then we 
   * try to find a boundary string to (adji[r], adjj[r]) which is in 
   * atari, and we try capturing at (gi, gj).
   */

  chainlinks(si, sj, &adj, adji, adjj, adjsize, adjlib);
  for (r=0; r<adj; r++) {
    if (adjlib[r]!=1) 
      continue;

    /* if stackp > depth and any boundary chain is in atari, assume safe */
    if (stackp > depth)
      READ_RETURN0(read_result);

    if (relative_break_chain(adji[r], adjj[r], &gi, &gj, si, sj)) {
      if (is_ko(gi, gj, other))
	READ_RETURN(read_result, i, j, gi, gj, 2);
      else
	READ_RETURN(read_result, i, j, gi, gj, 1);
    }

    approxlib(adji[r], adjj[r], p[adji[r]][adjj[r]], 2);
    hi=libi[0];
    hj=libj[0];
    if (trymove(hi, hj, other, "attack2-A", si, sj)) {
      if (attack(si, sj, NULL, NULL)) {
	int dcode=find_defense(si, sj, NULL, NULL);
	if (dcode != 1) {
	  if (dcode == 0) {
	    popgo();
	    READ_RETURN(read_result, i, j, hi, hj, 1);
	  }
	  else if (dcode == 2)
	    savecode=3;
	  else if (dcode == 3)
	    savecode=2;
	  savei=hi;
	  savej=hj;
	}
      }
      popgo();
    }
    else if (is_ko(hi, hj, other) 
	     && (stackp <= ko_depth)
	     && (tryko(hi, hj, other, "attack2-B"))) {
      if (attack(si, sj, NULL, NULL)) {
	savecode=3;
	savei=-1;
	savej=-1;
      }
    }
  }

  /* Get the two liberties of (si, sj) into (ai, aj) and (bi, bj). */ 
  liberties=approxlib(si, sj, color, 3);
  ASSERT(liberties==2, si, sj);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];

  /* if (bi, bj) looks more promising we wish to switch the two liberties.
     We check whether (bi, bj) is adjacent to more open liberties than (ai,aj).
  */

  if ((ai>0) && p[ai-1][aj]==EMPTY)
    acount++;
  if ((ai<board_size-1) && p[ai+1][aj]==EMPTY)
    acount++;
  if ((aj>0) && p[ai][aj-1]==EMPTY)
    acount++;
  if ((aj<board_size-1) && p[ai][aj+1]==EMPTY)
    acount++;
  if ((bi>0) && p[bi-1][bj]==EMPTY)
    bcount++;
  if ((bi<board_size-1) && p[bi+1][bj]==EMPTY)
    bcount++;
  if ((bj>0) && p[bi][bj-1]==EMPTY)
    bcount++;
  if ((bj<board_size-1) && p[bi][bj+1]==EMPTY)
    bcount++;

  if (bcount>acount) {
    ai=libi[1];
    aj=libj[1];
    bi=libi[0];
    bj=libj[0];
  }

  RTRACE("considering atari at %m\n", ai, aj);

  /* we only want to consider the move at (ai,aj) if:
     stackp <= backfill_depth                     -or-
     stackp <= depth and it is an isolated stone  -or-
     it is not in immediate atari
  */

  if ((stackp <= backfill_depth) 
      || ((stackp <= depth) 
	  && ((ai==0) || (p[ai-1][aj] != other))
	  && ((ai==board_size-1) || (p[ai+1][aj] != other))
	  && ((aj==0) || (p[ai][aj-1] != other))
	  && ((aj==board_size-1) || (p[ai][aj+1] != other)))
      || approxlib(ai, aj, other, 2) >1 )
    if (trymove(ai, aj, other, "attack2-C", si, sj))
      {
	can_save=defend1(si, sj, &di, &dj);
	if (can_save==0) {
	  popgo();
	  RTRACE("%m captures !!\n", ai, aj);
	  READ_RETURN(read_result, i, j, ai, aj, 1);
	} else if (can_save==2) {
	  savei=ai;
	  savej=aj;
	  savecode=3;
	} else if (can_save==3) {
	  savei=ai;
	  savej=aj;
	  savecode=2;
	}
	popgo();
      }

  /* try backfilling if atari is impossible */
  if ((stackp <= backfill_depth) && approxlib(ai, aj, other, 2)==1) {
    int ui=-1, uj=-1;

    ui=libi[0];
    uj=libj[0];
    if (trymove(ui, uj, other, "attack2-D", si, sj))  {
      if (attack(si, sj, NULL, NULL) && 
	  !find_defense(si, sj, NULL, NULL)) {
	savei=ui;
	savej=uj;
	savecode=1;
      }
      popgo();
    } else if (is_ko(ui, uj, other) && (savecode==0)) {
      savei=ai;
      savej=aj;
      savecode=3;
    }
  }
  
  RTRACE("first atari didn't work - try %m\n", bi, bj);
  
  /* we only want to consider the move at (bi,bj) if
     either it is not in immediate atari, or else
     stackp <= depth and it is an isolated stone. */
  
  if (((stackp <= depth) 
       && ((bi==0) || (p[bi-1][bj]!=other))
       && ((bi==board_size-1) || (p[bi+1][bj]!=other))
       && ((bj==0) || (p[bi][bj-1]!=other))
       && ((bj==board_size-1) || (p[bi][bj+1]!=other)))
      || approxlib(bi, bj, other, 2) >1) {
    if (trymove(bi, bj, other, "attack2-E", si, sj))
      {
	can_save=defend1(si, sj, &di, &dj);
	if (can_save==0) {
	  RTRACE("%m captures !!\n", bi, bj);
	  popgo();
	  READ_RETURN(read_result, i, j, bi, bj, 1);
	}
	else if ((can_save==2) && (savecode==0)) {
	  savei=bi;
	  savej=bj;
	  savecode=3;
	} if ((can_save==3) 
	      && ((savecode==0) || (savecode==3))) {
	  savei=bi;
	  savej=bj;
	  savecode=2;
	}
	popgo();
      } else if (is_ko(bi, bj, other) && (savecode==0)) {
	savei=bi;
	savej=bj;
	savecode=3;
      }
  }
  
  /* try backfilling if atari is impossible */
  if ((stackp <= backfill_depth) && (approxlib(bi, bj, other, 2)==1)) {
    int ui=-1, uj=-1;
      
    ui=libi[0];
    uj=libj[0];
    if (trymove(ui, uj, other, "attack2-F", si, sj)) {
      if (attack(si, sj, NULL, NULL) 
	  && !find_defense(si, sj, NULL, NULL)) {
	savei=ui;
	savej=uj;
	savecode=1;
      }
      popgo();
    }
  } else
    RTRACE("second atari at %m illegal\n", bi, bj);

  /* The simple ataris didn't work. Try something more fancy. */
  {
    int  xi, xj;

    if (find_cap2(si, sj, &xi, &xj))
      READ_RETURN(read_result, i, j, xi, xj, 1);
  }

  /* In a situation like this:
   *       
   * -----        the code that
   * cO.OX        follows can find
   * XXOOX        the attacking move
   * XO.OX        at 'c=(ci,cj)'.
   * XOOOX
   * XXXXX
   *
   */

  if ((stackp <= backfill_depth) 
      && (approxlib(ai, aj, other, 1)==0) 
      && (approxlib(ai, aj, color, 3)==2)) {
    if ((libi[0] != bi) || (libj[0] != bj)) {
      ci=libi[0];
      cj=libj[0];
    } else {
      ci=libi[1];
      cj=libj[1];
    }
    if ((approxlib(ci, cj, other, 2)>1) 
	&& trymove(ci, cj, other, "attack2-G", si, sj)) {
      if (attack(si, sj, NULL, NULL)) {
	int dcode=find_defense(si, sj, NULL, NULL);
	if (dcode != 1) {
	  if (dcode == 0) {
	    RTRACE("ALIVE!!\n");
	    popgo();
	    READ_RETURN(read_result, i, j, ci, cj, 1);
	  }
	  else if (dcode == 2)
	    savecode=3;
	  else if (dcode == 3)
	    savecode=2;
	  savei=ci;
	  savej=cj;
	}
      }
      popgo();
    }
  }
  if ((stackp <= backfill_depth) 
      && (approxlib(bi, bj, other, 1)==0) 
      && (approxlib(bi, bj, color, 3)==2)) {
    if ((libi[0] != ai) || (libj[0] != aj)) {
      ci=libi[0];
      cj=libj[0];
    } else {
      ci=libi[1];
      cj=libj[1];
    }
    if ((approxlib(ci, cj, other, 2)>1) 
	&& trymove(ci, cj, other, "attack2-H", si, sj)) {
      if (attack(si, sj, NULL, NULL)) {
	int dcode=find_defense(si, sj, NULL, NULL);
	if (dcode != 1) {
	  if (dcode == 0) {
	    RTRACE("ALIVE!!\n");
	    popgo();
	    READ_RETURN(read_result, i, j, ci, cj, 1);
	  }
	  else if (dcode == 2)
	    savecode=3;
	  else if (dcode == 3)
	    savecode=2;
	  savei=ci;
	  savej=cj;
	}
      }
      popgo();
    }
  }

  if (savei == -1) {
    RTRACE("ALIVE!!\n");
    READ_RETURN0(read_result);
  }
  READ_RETURN(read_result, i, j, savei, savej, savecode);
}


/* attack3(ti, tj, *i, *j) is used when (ti, tj) points to a group with
 * three liberties. It returns true if it finds a way to kill the group.
 *
 * Return code is 2 if the group can be killed if the attacker is 
 * willing to ignore any ko threat.
 *
 * Return code is 3 if the group can be killed if the attacker is 
 * able to find a ko threat which must be answered.
 *
 * If non-NULL (*i, *j) will be set to the move which makes the
 * attack succeed.
 */

int 
attack3(int ti, int tj, int *i, int *j)
{
  int color=p[ti][tj];
  int other=OTHER_COLOR(color);
  int ai, aj, bi, bj, ci, cj;
  int adj, adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  int r, gi, gj;
  int dcode=0;
  int mx[MAX_BOARD][MAX_BOARD];    /* mark moves already tried */
  int             found_read_result;
  int result=0;
  Read_result   * read_result;

  SETUP_TRACE_INFO("attack3", ti, tj);

  assert(p[ti][tj]!=EMPTY);

  if ((stackp <= depth) && (hashflags & HASH_ATTACK3)) {
    found_read_result = get_read_result(/*color, */ATTACK3,
					&ti, &tj, &read_result);
    if (found_read_result)
    {
      TRACE_CACHED_RESULT(*read_result);
      if (rr_get_result(*read_result) != 0) {
	if (i) *i = rr_get_result_i(*read_result);
	if (j) *j = rr_get_result_j(*read_result);
      }

      return rr_get_result(*read_result);
    }

    /* This data should always be recorded. */
    if (read_result) {
      rr_set_routine_i_j_stackp(*read_result, ATTACK3, ti, tj, stackp);
    }
  } else
    read_result = NULL;

  if (stackp > depth)
    READ_RETURN0(read_result);

  memset(mx, 0, sizeof(mx));

  chainlinks(ti, tj, &adj, adji, adjj, adjsize, adjlib);
  for (r=0; r<adj; r++)
    if (adjlib[r]==1) {
      approxlib(adji[r], adjj[r], p[adji[r]][adjj[r]], 2);
      
      /* If a component of the surrounding chain has just
       * one liberty, we try to defend it.
       */

      gi=libi[0]; /* liberty of the jeapardized component */
      gj=libj[0];
      if (!mx[gi][gj] && (approxlib(gi, gj, other, 2) > 1)) {
	if (trymove(gi, gj, other, "attack3-A", ti, tj)) {
	  mx[gi][gj]=1;
	  if (attack(ti, tj, NULL, NULL)) {
	    dcode=find_defense(ti, tj, NULL, NULL);
	    if (dcode != 1) {
	      if (dcode==0) 
		result=1;
	      else if (dcode==2)
		result=3;
	      else if (dcode==3)
		result=2;
	      popgo();
	      READ_RETURN(read_result, i, j, gi, gj, result);
	    }
	  }
	  popgo();
	}
      }
      { /* try to rescue the boundary piece by capturing */
	int xi, xj;
	
	result = relative_break_chain(adji[r], adjj[r], &xi, &xj, ti, tj);
	if (result)
	  READ_RETURN(read_result, i, j, xi, xj, result);
      }
    } else if ((stackp <= fourlib_depth) && adjlib[r]==2) {
      if (attack2(adji[r], adjj[r], NULL, NULL) 
	  && find_defense(adji[r], adjj[r], &gi, &gj)) {
	if (trymove(gi, gj, other, "attack3-B", ti, tj)) {
	  if (attack(ti, tj, NULL, NULL)) {
	    dcode=find_defense(ti, tj, NULL, NULL);
	    if (dcode != 1) {
	      if (dcode==0) 
		result=1;
	      else if (dcode==2)
		result=3;
	      else if (dcode==3)
		result=2;
	      popgo();
	      READ_RETURN(read_result, i, j, gi, gj, result);
	    }
	  }
	  popgo();
	}
      }
    }

  RTRACE("checking attack on %m with 3 liberties\n", ti, tj);
  approxlib(ti, tj, color, 4);
  ASSERT (lib==3, ti, tj);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];
  ci=libi[2];
  cj=libj[2];
  order3(&ai, &aj, &bi, &bj, &ci, &cj, ti, tj);
  if (!mx[ai][aj] && trymove(ai, aj, other, "attack3-C", ti, tj)) {
    mx[ai][aj]=1;
    RTRACE("try attacking at %m ...\n", ai, aj);
    dcode=find_defense(ti, tj, NULL, NULL);
    if ((dcode != 1) && attack(ti, tj, NULL, NULL)) {
      if (dcode == 0)
	result = 1;
      else if (dcode == 2)
	result = 3;
      else if (dcode == 3)
	result = 2;
      popgo();
      READ_RETURN(read_result, i, j, ai, aj, result);
    }
    popgo();
  }
  if (!mx[bi][bj] && trymove(bi, bj, other, "attack3-D", ti, tj)) {
    mx[bi][bj]=1;
    RTRACE("try attacking at %m ...\n", bi, bj);
    dcode=find_defense(ti, tj, NULL, NULL);
    if ((dcode != 1) && attack(ti, tj, NULL, NULL)) {
      if (dcode == 0)
	result = 1;
      else if (dcode == 2)
	result = 3;
      else if (dcode == 3)
	result = 2;
      popgo();
      READ_RETURN(read_result, i, j, bi, bj, result);
    }
    popgo();
  }
  if (!mx[ci][cj] && trymove(ci, cj, other, "attack3-E", ti, tj)) {
    RTRACE("try attacking at %m ...\n", ci, cj);
    dcode=find_defense(ti, tj, NULL, NULL);
    if ((dcode != 1) && attack(ti, tj, NULL, NULL)) {
      if (dcode == 0)
	result = 1;
      else if (dcode == 2)
	result = 3;
      else if (dcode == 3)
	result = 2;
      popgo();
      READ_RETURN(read_result, i, j, ci, cj, result);
    }
    popgo();
  }

  READ_RETURN0(read_result);
}


/* attack4 tries to capture a string with 4 liberties. This function
 * is not hashed.
 */

int 
attack4(int i, int j, int *ti, int *tj)
{
  int color=p[i][j];
  int other=OTHER_COLOR(color);
  int ai, aj, bi, bj, ci, cj, di, dj, gi, gj, r;
  int mx[MAX_BOARD][MAX_BOARD];  
  int adj, adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];

  assert(p[i][j]!=EMPTY);
  
  if (stackp > depth)
    return (0);

 /* The attack may fail if a boundary string is in atari and cannot be
  * defended. First we must try defending such a string.
  */
  
  memset(mx, 0, sizeof(mx));

  chainlinks(i, j, &adj, adji, adjj, adjsize, adjlib);
  for (r=0; r<adj; r++)
    if (adjlib[r]==1) {
      approxlib(adji[r], adjj[r], p[adji[r]][adjj[r]], 2);
      gi=libi[0];
      gj=libj[0];

      /* If moving out results in more than one liberty,
         we resume the attack. */
      if (!mx[gi][gj] && (approxlib(gi, gj, other, 2) > 1)) {
	if (trymove(gi, gj, other, "attack4-A", i, j)) {
	  mx[gi][gj]=1;
	  if (attack(i, j, NULL, NULL) && !find_defense(i, j, NULL, NULL)) {
	    popgo();
	    if (ti) *ti=gi;
	    if (tj) *tj=gj;
	    return (1);
	  }
	  popgo();
	}
      }
      if (relative_break_chain(adji[r], adjj[r], &gi, &gj, i, j)) {
	if (ti) *ti=gi;
	if (tj) *tj=gj;
	return 1;
      }
    }
  RTRACE("checking attack on %m with 3 liberties\n", i, j);
  approxlib(i, j, color, 5);
  ASSERT (lib==4, i, j);
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];
  ci=libi[2];
  cj=libj[2];
  di=libi[3];
  dj=libj[3];
  
  if (trymove(ai, aj, other, "attack4-B", i, j)) {
    RTRACE("try attacking at %m ...\n", ai, aj);
    if ((approxlib(i, j, color, 3)==3) 
	&& !find_defense(i, j, NULL, NULL)
	&& attack(i, j, NULL, NULL) 
	&& !attack(ai, aj, NULL, NULL)) {
      if (ti) *ti=ai;
      if (tj) *tj=aj;
      popgo();
      return (1);
    }
    popgo();
  }
  if (trymove(bi, bj, other, "attack4-C", i, j)) {
    RTRACE("try attacking at %m ...\n", bi, bj);
    if ((approxlib(i, j, color, 3)==3) 
	&& !find_defense(i, j, NULL, NULL)
	&& attack(i, j, NULL, NULL) 
	&& !attack(bi, bj, NULL, NULL)) {
      if (ti) *ti=bi;
      if (tj) *tj=bj;
      popgo();
      return (1);
    }
    popgo();
  }
  if (trymove(ci, cj, other, "attack4-D", i, j)) {
    RTRACE("try attacking at %m ...\n", ci, cj);
    if ((approxlib(i, j, color, 3)==3) 
	&& !find_defense(i, j, NULL, NULL)
	&& attack(i, j, NULL, NULL) 
	&& !attack(ci, cj, NULL, NULL)) {
      if (ti) *ti=ci;
      if (tj) *tj=cj;
      popgo();
      return (1);
    }
    popgo();
  }
  if (trymove(di, dj, other, "attack4-E", i, j)) {
    RTRACE("try attacking at %m ...\n", di, dj);
    if ((approxlib(i, j, color, 3)==3) 
	&& !find_defense(i, j, NULL, NULL)
	&& attack(i, j, NULL, NULL) 
	&& !attack(di, dj, NULL, NULL)) {
      if (ti) *ti=di;
      if (tj) *tj=dj;
      popgo();
      return (1);
    }
    popgo();
  }
  return (0);
}


/* If (m,n) points to a string with 2 liberties, find_cap2(m,n,&i,&j)
 * looks for a configuration of the following type:
 *
 *  O.
 *  .*
 *
 * where O is an element of the string in question. It tries the
 * move at * and returns true this move captures the string, leaving
 * (i,j) pointing to *. 
*/

int
find_cap2(int m, int n, int *i, int *j)
{
  int ai, aj, bi, bj;
  int ti=-1, tj=-1;

  /* Get the liberties into (ai, aj) and (bi, bj). */
  if (approxlib(m, n, p[m][n], 4) != 2)
    return 0;
  ai=libi[0];
  aj=libj[0];
  bi=libi[1];
  bj=libj[1];

  /* Check if the two liberties are located like the figure above. */
  if (((ai==bi+1) || (ai==bi-1)) 
      && ((aj==bj+1) || (aj==bj-1))) 
  {
    /* Which of the two corner points should we use? One of them is 
       always occupied by the string at (m, n), the other one is either
       free or occupied by something else. */
    if (p[bi][aj]==EMPTY) {
      ti=bi;
      tj=aj;
    }
    if (p[ai][bj]==EMPTY) {
      ti=ai;
      tj=bj;
    }

    /* If we didn't find a free intersection, we couldn't make the move. */
    if (ti==-1)
      return 0;

    /* Ok, we found the spot. Now see if the move works. */
    RTRACE("trying to capture %m with capping move at %m\n", m, n, ti, tj);
    if (trymove(ti, tj, OTHER_COLOR(p[m][n]),"find_cap2", m, n)) {
      if (defend2(m, n, NULL, NULL)) {
	RTRACE("cap failed!\n", m, n, ti, tj);
	popgo();
	return 0;
      }
      RTRACE("cap succeeded!\n", m, n, ti, tj);
      popgo();
      if (i) *i=ti;
      if (j) *j=tj;
      return 1;
    }
  }

  return 0;
}    




/* ============== Break_chain and its derivatives ================= */



/* chainlinks returns (in adji, adjj arrays) the chains surrounding
 * the group at i, j. Adapted from count. Marks only one stone on
 * each link. If stackp <= depth, these are sorted by size (largest
 * first).
 */

static char chainlinks_ma[MAX_BOARD][MAX_BOARD];
static char chainlinks_ml[MAX_BOARD][MAX_BOARD];
static char chainlinks_mark = -1;

static void
init_chainlinks(void)
{
  /* set all pieces as unmarked */
  memset(chainlinks_ma, 0, sizeof(chainlinks_ma));
  memset(chainlinks_ml, 0, sizeof(chainlinks_ml));
  chainlinks_mark = 1;
}

void 
chainlinks(int m, int n, int *adj, int adji[MAXCHAIN], int adjj[MAXCHAIN],
	   int adjsize[MAXCHAIN], int adjlib[MAXCHAIN])
{
  chainlinks_mark++;
  if (chainlinks_mark == 0) { /* We have wrapped around, reinitialize. */
    init_chainlinks();
  }
  
  (*adj)=0;
  
  chain(m, n, chainlinks_ma, chainlinks_ml, adj, adji, adjj, adjsize, adjlib,
	chainlinks_mark);
}


/* worker fn for chainlinks.
 * ma and ml store state about where we have been.
 * ma stores which elements of the surrounded chain have been visited.
 * ml (via count()) stores which surrounding stones have been visited.
 *
 * Algorithm :
 *   Mark ma[i][j] as done.
 *   explore the stones surrounding i,j :
 *    - for any which are the same color and not yet visited,
 *      save them in an internal stack for later visit.
 *    - any which are the opposite colour, make a note if not visited,
 *      and use count() to mark them all as visited.
 */

static int seedi[MAXCHAIN];
static int seedj[MAXCHAIN];

static void 
chain(int i, int j, char ma[MAX_BOARD][MAX_BOARD],
      char ml[MAX_BOARD][MAX_BOARD], int *adj, int adji[MAXCHAIN],
      int adjj[MAXCHAIN], int adjsize[MAXCHAIN], int adjlib[MAXCHAIN],
      char mark)
{
  int k;
  int seedp;
  int color=p[i][j];
  int other=OTHER_COLOR(color);

  seedi[0] = i;
  seedj[0] = j;
  seedp = 1;

  while (seedp>0) {
    seedp--;
    i = seedi[seedp];
    j = seedj[seedp];

    /* We may already have visited here. */
    if (ma[i][j] == mark)
      continue;
    ma[i][j] = mark;

    /* check North neighbor */
    if (i != 0) {
      if ((p[i-1][j] == other) && ma[i-1][j] != mark && ml[i-1][j] != mark) {
	adjlib[(*adj)] = count(i-1, j, other, ml, 99999, mark);
	for(k=0; k<lib; k++)
	  ml[libi[k]][libj[k]] = mark-1;
	adji[(*adj)] = i-1;
	adjj[(*adj)] = j;
	adjsize[(*adj)] = size;
	++(*adj);
	ma[i-1][j] = mark;
      } 
      else {
	if ((p[i-1][j] == color) && ma[i-1][j] != mark) {
	  seedi[seedp] = i-1;
	  seedj[seedp] = j;
	  seedp++;
	}
      }
    }
    /* check South neighbor */
    if (i != board_size-1) {
      if ((p[i+1][j] == other) && ma[i+1][j] != mark && ml[i+1][j] != mark) {
	adjlib[(*adj)] = count(i+1, j, other, ml, 99999, mark);
	for(k = 0; k < lib; k++)
	  ml[libi[k]][libj[k]] = mark-1;
	adji[(*adj)] = i+1;
	adjj[(*adj)] = j;
	adjsize[(*adj)] = size;
	++(*adj);
	ma[i+1][j] = mark;
      }
      else {
	if ((p[i+1][j] == color) && ma[i+1][j] != mark) {
	  seedi[seedp] = i+1;
	  seedj[seedp] = j;
	  seedp++;
	}
      }
    }
    /* check West neighbor */
    if (j != 0) {
      if ((p[i][j-1] == other) && ma[i][j-1] != mark && ml[i][j-1] !=mark) {
	adjlib[(*adj)] = count(i, j-1, other, ml, 99999, mark);
	for(k = 0; k < lib; k++)
	    ml[libi[k]][libj[k]] = mark-1;
	adji[(*adj)] = i;
	adjj[(*adj)] = j-1;
	adjsize[(*adj)] = size;
	++(*adj);
	ma[i][j-1] = mark;
      }
      else {
	if ((p[i][j-1] == color) && ma[i][j-1] != mark) {
	  seedi[seedp] = i;
	  seedj[seedp] = j-1;
	  seedp++;
	}
      }
    }
    /* check East neighbor */
    if (j != board_size-1) {
      if ((p[i][j+1] == other) && ma[i][j+1] != mark && ml[i][j+1] != mark) {
	adjlib[(*adj)] = count(i, j+1, other, ml, 99999, mark);
	for(k = 0; k < lib; k++)
	  ml[libi[k]][libj[k]] = mark-1;
	adji[(*adj)] = i;
	adjj[(*adj)] = j+1;
	adjsize[(*adj)] = size;
	++(*adj);
	ma[i][j+1] = mark;
	}
      else {
	if ((p[i][j+1] == color) && ma[i][j+1] != mark) {
	  seedi[seedp] = i;
	  seedj[seedp] = j+1;
	  seedp++;
	}
      }
    }
  }
} 



/* 
 * (si, sj) points to a string. 
 * 
 * break_chain(si, sj, *i, *j, *k, *l) returns 1 if
 *   part of some surrounding string is in atari, and if capturing
 *   this string results in a live string at (si, sj). 
 *
 * Returns 2 if the capturing string can be taken (as in a snapback),
 *   or the the saving move depends on ignoring a ko threat;
 * 
 * Returns 3 if the saving move requires making a ko threat and winning
 *   the ko.
 *
 * (i,j), if not NULL, are left pointing to the appropriate defensive move.
 * (k,l), if not NULL, are left pointing to the boundary string which
 *   is in atari.
 */

int
break_chain(int si, int sj, int *i, int *j, int *k, int *l)
{
  int color;
  int r;
  int ai, aj, ci, cj;
  int adj, adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  int savei = -1, savej = -1, savek=-1, savel=-1, savecode=0;
  
  RTRACE("in break_chain at %m\n", si, sj);
  color=p[si][sj];
  chainlinks(si, sj, &adj, adji, adjj, adjsize, adjlib);
  if (verbose) {
    RTRACE("chain surrounding %m (liberties): ", si, sj);
    for (r=0; r<adj; r++) {
      ai=adji[r];
      aj=adjj[r];
      RTRACE("%o%m(%d) ", ai, aj, adjlib[r]);  /* %o cancels indent */
    }
    RTRACE("%o\n");
  }
  for (r=0; r<adj; r++) {
    ai=adji[r];
    aj=adjj[r];
    if (approxlib(ai, aj, p[ai][aj], 2)==1) {
      ci=libi[0];
      cj=libj[0];
      if (trymove(ci, cj, color, "break_chain-A", si, sj)) {
	int acode=attack(si, sj, NULL, NULL);
	if (acode != 1) {
	  RTRACE("%m found to defend %m by attacking %m\n", 
		 ci, cj, si, sj, ai, aj);
	  if ((acode != 0) || attack(ci, cj, NULL, NULL)) {
	    savei=ci;
	    savej=cj;
	      savek=ai;
	      savel=aj;
	      if (acode==0)
		savecode=1;
	      else if (acode==2)
		savecode=3;
	      else if (acode==3)
		savecode=2;
	      popgo();
	  }
	  else {
	    if (i) *i=ci;
	    if (j) *j=cj;
	    if (k) *k=ai;
	    if (l) *l=aj;
	    popgo();
	    return (1);
	  }
	}
	else popgo();
      } else {
	if ((stackp <= ko_depth) && (savecode==0) && is_ko(ci, cj, color)) {
	  if (tryko(ci, cj, color, "break_chain-B")) {
	    if (attack(si, sj, NULL, NULL) != -1) {
	      savei=ci;
	      savej=cj;
	      savek=ai;
	      savel=aj;
	      savecode=3;
	    }
	    popgo();
	  }
	}
      }
    }
  }
  if (savei != -1) {
    if (i) *i=savei;
    if (j) *j=savej;
    if (k) *k=savek;
    if (l) *l=savel;
    return savecode;
  }
  return(0);
}

/*
 * si, sj points to a group. break_chain2(si, sj, *i, *j)
 * returns 1 if there is a string in the surrounding chain having
 * exactly two liberties whose attack leads to the rescue of
 * (si, sj). Then *i, *j points to the location of the attacking move.
 * 
 * Returns 2 if the attacking stone can be captured, 1 if it cannot.
 */

int 
break_chain2(int si, int sj, int *i, int *j)
{
  int color, other;
  int r;
  int u=0, v;
  int ai, aj;
  int saveti=-1, savetj=-1;
  int adj;
  int adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  int ti[MAXCHAIN], tj[MAXCHAIN];
  int mw[MAX_BOARD][MAX_BOARD];
  int savecode=0;

  memset(mw, 0, sizeof(mw));

  RTRACE("in break_chain2 at %m\n", si, sj);
  color=p[si][sj];
  other=OTHER_COLOR(color);
  chainlinks(si, sj, &adj, adji, adjj, adjsize, adjlib);
  RTRACE("chain around %m (liberties): ", si, sj);
  for (r=0;r<adj;r++) {
    ai=adji[r];
    aj=adjj[r];
    /* We make a list in the (adji, adjj) array of the liberties
     * of boundary strings having exactly two liberties. We mark
     * each liberty in the mw array so that we do not list any
     * more than once.
     */
    if (adjlib[r] == 2) {
      int bi, bj, ci, cj, di, dj;
      int bcount=0;
      int ccount=0;

      approxlib(ai, aj, p[ai][aj], 3);
      bi=libi[0];
      bj=libj[0];
      ci=libi[1];
      cj=libj[1];

      /* If the c liberty looks more promising, we switch them.
       * We want the first liberty to be the one adjacent to the
       * most empty intersections.
       */
      if ((bi>0) && p[bi-1][bj]==EMPTY)
	bcount++;
      if ((bi<board_size-1) && p[bi+1][bj]==EMPTY)
	bcount++;
      if ((bj>0) && p[bi][bj-1]==EMPTY)
	bcount++;
      if ((bj<board_size-1) && p[bi][bj+1]==EMPTY)
	bcount++;
      if ((ci>0) && p[ci-1][cj]==EMPTY)
	ccount++;
      if ((ci<board_size-1) && p[ci+1][cj]==EMPTY)
	ccount++;
      if ((cj>0) && p[ci][cj-1]==EMPTY)
	ccount++;
      if ((cj<board_size-1) && p[ci][cj+1]==EMPTY)
	ccount++;
      
      if (ccount>bcount) {
	bi=libi[1];
	bj=libj[1];
	ci=libi[0];
	cj=libj[0];
      }

      if (!mw[bi][bj]) {
	mw[bi][bj]=1;
	ti[u]=bi;
	tj[u]=bj;
	u++;
      }
      if (!mw[ci][cj]) {
	mw[ci][cj]=1;
	ti[u]=ci;
	tj[u]=cj;
	u++;
      }
      if ((stackp <= fourlib_depth)
	  && break_chain(adji[r], adjj[r], &di, &dj, NULL, NULL)
	  && !mw[di][dj]) {
	mw[di][dj]=1;
	ti[u]=di;
	tj[u]=dj;
	u++;
      }
    }
    RTRACE("%o%m(%d) ", ai, aj, adjlib[r]);
  }
  RTRACE("%o\n");

/* we do not wish to consider the move if it can be 
 * immediately recaptured, unless stackp <= backfill_depth.
 */

  for (v=0; v<u; v++) {
    if (approxlib(ti[v], tj[v], color, 2)>1) {
      if (trymove(ti[v], tj[v], color, "break_chain2-A", si, sj)) {
	int acode=attack(si, sj, i, j);
	if (acode != 1) {
	  if (acode==0) {
	    if (i) *i=ti[v];
	    if (j) *j=tj[v];
	    popgo();
	    return 1;
	  }
	  else if (acode==2)
	    savecode=3;
	  else if (acode==3)
	    savecode=2;
	  saveti=ti[v];
	  savetj=tj[v];
	}
	popgo();
      }
    }
    else if ((lib==1) && (stackp <= backfill_depth)) {
      int bi=libi[0];
      int bj=libj[0];
      int try_harder=0;

      if (trymove(ti[v], tj[v], color, "break_chain2-B", si, sj)) {
	if (trymove(bi, bj, other, "break_chain2-C", si, sj)) {
	  if (p[si][sj] != EMPTY) {
	    int dcode=find_defense(si, sj, NULL, NULL);
	    if (dcode) {
	      if (dcode == 1) {
		if (i) *i=ti[v];
		if (j) *j=tj[v];
		popgo();
		popgo();
		return 1;
	      }
	      else if ((dcode==2) || (savecode==0)) {
		  savecode=dcode;
		  saveti=ti[v];
		  savetj=tj[v];
	      }
	    }
	  }
	  if (approxlib(bi, bj, other,2)==1)
	    try_harder=1;
	  popgo();
	}
	if (try_harder) {
	  int acode=attack(si, sj, i, j);
	  if (acode != 1) {
	    if (acode==0) {
	      if (i) *i=ti[v];
	      if (j) *j=tj[v];
	      popgo();
	      return 1;
	    }
	    else if (acode==2)
	      savecode=3;
	    else if (acode==3)
	      savecode=2;
	    saveti=ti[v];
	    savetj=tj[v];
	  }
	}
	popgo();
      }
    }
  }
  if (saveti != -1) {
    if (i) *i=saveti;
    if (j) *j=savetj;
    return (savecode);
  }
  return(0);
}

/*
 * If si, sj points to a group, double_atari_chain2(si, sj, *i, *j)
 * returns 1 if there is move which makes a double atari on 
 * a string in the surrounding chain, and if this move rescues
 * the string. This is a faster, more specific function than
 * break_chain2, and substitutes for it while reading ladders
 * when stackp > depth.
 * 
 * If not NULL, *i, *j points to the location of the attacking move.
 * 
 */

int 
double_atari_chain2(int si, int sj, int *i, int *j)
{
  int r;
  int color=p[si][sj];
  int ai, aj;
  int adj;
  int adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  int mw[MAX_BOARD][MAX_BOARD];
  int w=0;
  int v;
  int ti[MAXCHAIN], tj[MAXCHAIN]; /* list of double atari moves */

  memset(mw, 0, sizeof(mw));

  color=p[si][sj];
  chainlinks(si, sj, &adj, adji, adjj, adjsize, adjlib);
  RTRACE("chain around %m (liberties): ", si, sj);
  for (r=0;r<adj;r++) {
    ai=adji[r];
    aj=adjj[r];
    if (adjlib[r] == 2) {
      approxlib(ai, aj, p[ai][aj], 3);
      mw[libi[0]][libj[0]]++;
      mw[libi[1]][libj[1]]++;      
      if (mw[libi[0]][libj[0]]>1) {
	/* found a double atari */
	ti[w]=libi[0];
	tj[w]=libj[0];
	w++;
      }
      if (mw[libi[1]][libj[1]]>1) {
	/* found a double atari */
	ti[w]=libi[1];
	tj[w]=libj[1];
	w++;
      }
    }
  }
  for (v=0; v<w; v++)
    if (trymove(ti[v], tj[v], color, "double_atari_chain2", si, sj)) {
      if (!attack(si, sj, NULL, NULL)) {
	popgo();
	if (i) *i=ti[v];
	if (j) *j=tj[v];
	return 1;
      }
      else popgo();
    }
  return 0;
}


/*
 * si, sj points to a group. break_chain2(si, sj, *i, *j)
 * returns 1 if there is a string in the surrounding chain having
 * exactly two liberties whose attack leads to the rescue of
 * (si, sj). Then *i, *j points to the location of the attacking move.
 * 
 * Returns 2 if the attacking stone can be captured, 1 if it cannot.
 */


/* 
 * relative_break_chain(si, sj, *i, *j, ti, tj) is a variant of
 * break_chain. The strings (si, sj) and (ti, tj) are of
 * opposite color, and (ti, tj) is under attack. This function
 * looks for a boundary string to (si, sj) which is in atari,
 * and asks whether capturing it will result in the capture
 * of (ti, tj).
 */

static int
relative_break_chain(int si, int sj, int *i, int *j, int ti, int tj)
{
  int color;
  int r;
  int ai, aj, ci, cj;
  int adj, adji[MAXCHAIN], adjj[MAXCHAIN], adjsize[MAXCHAIN], adjlib[MAXCHAIN];
  
  if (stackp > depth)
    return (0);

  RTRACE("in relative_break_chain at %m (%m)\n", si, sj, ti, tj);
  color=p[si][sj];
  chainlinks(si, sj, &adj, adji, adjj, adjsize, adjlib);
  if (verbose) {
    RTRACE("chain surrounding %m (liberties): ", si, sj);
    for (r=0; r<adj; r++) {
      ai=adji[r];
      aj=adjj[r];
      RTRACE("%o%m(%d) ", ai, aj, adjlib[r]);  /* %o cancels indent */
    }
    RTRACE("%o\n");
  }
  for (r=0; r<adj; r++) {
    ai=adji[r];
    aj=adjj[r];
    if (approxlib(ai, aj, p[ai][aj], 2)==1) {
      ci=libi[0];
      cj=libj[0];
      if (trymove(ci, cj, color, "relative_break_chain", si, sj)) {
	if (attack(ti, tj, NULL, NULL) && !find_defense(ti, tj, NULL, NULL)) {
	  RTRACE("%m found to attack %m by defending %m\n", 
		 ci, cj, ti, tj, si, sj);
	  if (i) *i=ci;
	  if (j) *j=cj;
	  popgo();
	  return (1);
	}
	else popgo();
      }
    }
  }
  return(0);
}


/* ============== Prioritization ================= */

/* This function is applied when (*ai,*aj), etc. contain three liberties
   of the string at (si, sj). It attempts to reorder them according 
   according to the likelihood of being an effective attacking or
   defensive move.
*/

static void
order3(int *ai, int *aj, int *bi, int *bj, int *ci, int *cj, int si, int sj)
{
  int color=p[si][sj];
  int alib, blib, clib;
  int ti, tj, tlib;

  alib=approxlib(*ai, *aj, color, 4);
  blib=approxlib(*bi, *bj, color, 4);
  clib=approxlib(*ci, *cj, color, 4);
  if (blib>alib) {
    ti=*ai;
    tj=*aj;
    tlib=alib;
    *ai=*bi;
    *aj=*bj;
    alib=blib;
    *bi=ti;
    *bj=tj;
    blib=tlib;
  }
  if (clib>alib) {
    ti=*ai;
    tj=*aj;
    tlib=alib;
    *ai=*ci;
    *aj=*cj;
    alib=clib;
    *ci=ti;
    *cj=tj;
    clib=tlib;
  }
  if (clib>blib) {
    ti=*bi;
    tj=*bj;
    tlib=blib;
    *bi=*ci;
    *bj=*cj;
    blib=clib;
    *ci=ti;
    *cj=tj;
    clib=tlib;
  }
}


/* ============== Reading utilities ================= */



/* snapback(si, sj, i, j, color) considers a move by color at (i, j)
 * and returns true if the move is a snapback.
 *
 * Algorithm: It removes dead pieces of the other color, then returns 1 
 * if the stone at (si, sj) has <2 liberties. The purpose of this test 
 * is to avoid snapbacks. 
 *
 * (i, j) and (si, sj) may be either same or different. Also returns 1
 * if the move at (i, j) is illegal, with the trace message
 * "ko violation" which is the only way I think this could happen.
 *
 * It is not a snapback if the capturing stone can
 * be recaptured on its own, eg
 *
 *   XXOOOOO
 *   X*XXXXO
 *   -------
 *
 * O capturing at * is in atari, but this is not a snapback.
 *
 * Use with caution: you may want to condition the test on
 * the string being captured not being a singleton. For example
 *
 *   XXXOOOOOOOO
 *   XO*XXXXXXXO
 *   -----------
 *
 * is rejected as a snapback, yet O captures more than it gives up.
 *
 */


int 
snapback(int si, int sj, int i, int j, int color)
{

  /* approxlib writes the size of the group to 'size'. If approxlib returns
   * 1, we know it has done an exhaustive search, and therefore size is correct
   */

  if (is_ko(i, j, color))
    return 0;

  if (trymove(i, j, color, "snapback", si, sj)) {
    if (approxlib(si, sj, p[si][sj], 3)<2  &&  (size > 1) ) {
      RTRACE("SNAPBACK at %m!\n", i, j);
      popgo();
      return 1;
    } else {
      popgo();
      return 0;
    }
  } 
  RTRACE("KO VIOLATION at %m!\n", i, j);
  return 1;
}




static int safe_move_cache[MAX_BOARD][MAX_BOARD][2];
static int safe_move_cache_when[MAX_BOARD][MAX_BOARD][2];

void
clear_safe_move_cache(void)
{
  int i,j;
  for (i=0;i<MAX_BOARD;i++)
    for (j=0;j<MAX_BOARD;j++) {
      safe_move_cache_when[i][j][0] = -1;
      safe_move_cache_when[i][j][1] = -1;
    }
}

/* safe_move(i, j, color) checks whether a move at (i, j) is illegal
 * or can immediately be captured. If stackp==0 the result is cached.
 * If the move only can be captured by a ko, it's considered safe.
 * This may or may not be a good convention.
 */

int 
safe_move(int i, int j, int color)
{
  int safe=0;

  if (stackp == 0 && safe_move_cache_when[i][j][color==BLACK] == movenum)
    return safe_move_cache[i][j][color==BLACK];

  if (trymove(i, j, color, "safe_move", -1, -1)) {
    int acode = attack(i, j, NULL, NULL);
    if (acode != 1)
      safe = 1;
    popgo();
  }
  
  if (stackp == 0) {
    safe_move_cache_when[i][j][color==BLACK] = movenum;
    safe_move_cache[i][j][color==BLACK] = safe;
  }
  return safe;
}


/* naive_ladder(si, sj, &i, &j) tries to capture a string (si, sj)
 * with exactly two liberties under simplified assumptions, which are
 * adequate in a ladder. The rules are as follows:
 *
 * 1. The attacker is allowed to play at each of the two liberties.
 *    If the move was legal, the string now has exactly one
 *    liberty.
 * 2. Define the last stone of the string to be the stone of the
 *    string adjacent to the last liberty. The defender is allowed to
 *    try the following moves:
 *    - extend the string by playing on the liberty
 *    - try to capture the last stone played by the attacker
 *    - try to capture any stone that was put into atari by the
 *      previous move. It is conjectured that it's sufficient to look
 *      for such stones at a distance two from the last liberty.
 *    We only consider captures that can be done immediately.
 * 3. Depending on the resulting number of liberties of the string, we
 *    value each node as follows:
 *    3 or more liberties: the attack has failed
 *    2 liberties:         recurse
 *    1 liberty:           the attack has succeeded
 *    illegal move for the defender: successful attack
 *    illegal move for the attacker: failed attack
 *
 * Return codes are as usual 0 for failure and 1 for success. If the
 * attack was successful, (*i, *j) contains the attacking move, unless
 * i and j are null pointers.
 *
 * The differences compared to the attack2()/defend1() combination for
 * reading ladders is that this one really always reads them to the
 * very end and that it is faster (because it doesn't call
 * break_chain()). In contrast to attack2() though, this function can
 * only be used for capturing in ladders.
 *
 * FIXME: The ladder capture may depend on ko. Need to add the ko
 *        return codes.
 */

int
naive_ladder(int si, int sj, int *i, int *j)
{
  int color, other;
  int ai, aj, bi, bj;
  int acount = 0, bcount = 0;
  int liberties;
  
  assert(p[si][sj] != EMPTY);
  assert(approxlib(si, sj, p[si][sj], 3)==2);
  DEBUG(DEBUG_LADDER, "naive_ladder(%m)\n", si, sj);

  RTRACE("checking ladder attack on %m with 2 liberties\n", si, sj);

  color = p[si][sj];
  other = OTHER_COLOR(color);

  /* Get the two liberties of (si, sj) into (ai, aj) and (bi, bj). */ 
  liberties=approxlib(si, sj, color, 3);
  ASSERT(liberties==2, si, sj);
  ai = libi[0];
  aj = libj[0];
  bi = libi[1];
  bj = libj[1];

  /* if (bi, bj) looks more promising we wish to switch the two liberties.
   * We check whether (bi, bj) is adjacent to more open liberties than
   * (ai, aj).
   * FIXME: This is taken from attack2(). Not sure whether it is good here too.
   */

  if ((ai>0) && p[ai-1][aj]==EMPTY)
    acount++;
  if ((ai<board_size-1) && p[ai+1][aj]==EMPTY)
    acount++;
  if ((aj>0) && p[ai][aj-1]==EMPTY)
    acount++;
  if ((aj<board_size-1) && p[ai][aj+1]==EMPTY)
    acount++;
  if ((bi>0) && p[bi-1][bj]==EMPTY)
    bcount++;
  if ((bi<board_size-1) && p[bi+1][bj]==EMPTY)
    bcount++;
  if ((bj>0) && p[bi][bj-1]==EMPTY)
    bcount++;
  if ((bj<board_size-1) && p[bi][bj+1]==EMPTY)
    bcount++;

  if (bcount>acount) {
    ai=libi[1];
    aj=libj[1];
    bi=libi[0];
    bj=libj[0];
  }

  RTRACE("considering atari at %m\n", ai, aj);
  
  if (trymove(ai, aj, other, "naive_ladder-A", si, sj)) {
    if (!naive_ladder_defense(si, sj, ai, aj, bi, bj, color, other)) {
      popgo();
      if (i) *i = ai;
      if (j) *j = aj;
      return 1;
    }
    popgo();
  }
	  
  if (trymove(bi, bj, other, "naive_ladder-B", si, sj)) {
    if (!naive_ladder_defense(si, sj, bi, bj, ai, aj, color, other)) {
      popgo();
      if (i) *i = bi;
      if (j) *j = bj;
      return 1;
    }
    popgo();
  }

  /* Neither move worked. */
  return 0;
}


/* Try to save the one-liberty string (si, sj) from being caught in a
 * ladder. (ai, aj) is the last played attacking stone and (bi, bj) is
 * the last remaining liberty.
 */

static int
naive_ladder_defense(int si, int sj, int ai, int aj, int bi, int bj,
		     int color, int other) {
  int liberties;
  
  /* Try to capture the just played stone. */
  if (naive_ladder_break_through(si, sj, ai, aj, color, other))
    return 1;

  /* Try to run away by extending on the last liberty. */
  if (trymove(bi, bj, color, "naive_ladder_defense", si, sj)) {
    liberties = approxlib(si, sj, color, 3);
    if (liberties >= 3
	|| (liberties == 2
	    && !naive_ladder(si, sj, NULL, NULL))) {
      popgo();
      return 1;
    }
    popgo();
  }
  
  /* Try to capture a string at distance two (Manhattan metric) from
   * the last liberty.
   */
  if (naive_ladder_break_through(si, sj, bi-2, bj, color, other))
    return 1;

  if (naive_ladder_break_through(si, sj, bi-1, bj-1, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi, bj-2, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi+1, bj-1, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi+2, bj, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi+1, bj+1, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi, bj+2, color, other))
    return 1;
  
  if (naive_ladder_break_through(si, sj, bi-1, bj+1, color, other))
    return 1;
  
  /* Nothing worked. */
  return 0;
}


/* Try to break out of the ladder by capturing (ai, aj). We must first
 * verify that there is an opponent stone there and that it is in
 * atari so we can capture it immediately. After the capture we count
 * liberties for (si, sj) to see if the ladder is decided yet.
 */
static int
naive_ladder_break_through(int si, int sj, int ai, int aj,
			   int color, int other)
{
  int liberties;
  
  if (ai < 0
      || ai >= board_size
      || aj < 0
      || aj >= board_size)
    return 0;

  if (p[ai][aj] != other)
    return 0;
  
  if (approxlib(ai, aj, other, 2) != 1)
    return 0;

  if (trymove(libi[0], libj[0], color, "naive_ladder_break_through", si, sj)) {
    liberties = approxlib(si, sj, color, 3);
    if (liberties >= 3
	|| (liberties == 2
	    && !naive_ladder(si, sj, NULL, NULL))) {
      popgo();
      return 1;
    }
    popgo();
  }
  
  return 0;
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
