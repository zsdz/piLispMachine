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

/*
 * This file, together with engine/hash.c implements hashing of go positions
 * using a method known as Zobrist hashing.  See the Texinfo documentation
 * (Reading/Hashing) for more information.  
 */


#ifndef _HASH_H_
#define _HASH_H_


/* for testing: Enables a lot of checks. */
#define CHECK_HASHING 0

/* Dump (almost) all read results. */
#define TRACE_READ_RESULTS 0


/*
 * We define a special compact representation of the board for the 
 * positions.  In this representation each location is represented
 * by 2 bits with 0 meaning EMPTY, 1 meaning WHITE and 2 meaning 
 * BLACK as defined in liberty.h.
 *
 * COMPACT_BOARD_SIZE is the size of such a compact representation
 * for the maximum board size allowed.
 */

#define COMPACT_BOARD_SIZE ((MAX_BOARD) * (MAX_BOARD) / 16 + 1)

/*
 * A go position consists of the board, possibly a ko point but NOT of
 * which player is to move.  The ko point is defined as the point where,
 * on the last move, one stone was captured.  It is illegal for the 
 * player to move to place a stone on this point.  To do so would either
 * be suicide, which is illegal anyhow, or a violation of the ko rule.
 * If there is no ko going on, ko_i == -1;
 */

typedef struct hashposition_t {
  unsigned long  board[COMPACT_BOARD_SIZE];
  int            ko_i;
  int            ko_j;
  /*  int            to_move;   DISABLED  */
} Hashposition;


/*
 * This struct is maintained by the machinery that updates the board
 * to provide incremental hashing. Examples: updateboard(), legal(), ...
 */

typedef struct {
  unsigned long  hashval;
  Hashposition   hashpos;
} Hash_data;


/*
 * This struct contains the attack / defense point and the result.
 * It is kept in a linked list, and each position has a list of 
 * these.
 */

typedef struct read_result_t {
  unsigned int routine_i_j_stackp;	/* Interpret this as the following: */
				        /* (packed for search speed)       */
  /*
  unsigned char  routine;	// If this routine (num 0..255)... 
  unsigned char  i;		// ...tries to attack / defend the string 
  unsigned char  j;		//    at (i, j)... 
  unsigned char  stackp;	// ...at this reading depth... 
  */

  int result_ri_rj;		/* ...then this was the result. */
  /*
  unsigned char  result;
  unsigned char  ri;
  unsigned char  rj;
  */

  struct read_result_t  * next;
} Read_result;


/* Get parts of a Read_result identifying the routine and position. */
#define rr_get_routine(rr)    (((rr).routine_i_j_stackp >> 24) & 0xff)
#define rr_get_pos_i(rr)      (((rr).routine_i_j_stackp >> 16) & 0xff)
#define rr_get_pos_j(rr)      (((rr).routine_i_j_stackp >>  8) & 0xff)
#define rr_get_stackp(rr)     (((rr).routine_i_j_stackp >>  0) & 0xff)

/* Set corresponding parts. */
#define rr_set_routine_i_j_stackp(rr, routine, i, j, stackp) \
	(rr).routine_i_j_stackp \
	    = (((((((routine) << 8) | (i)) << 8) | (j)) << 8) | (stackp))

/* Get parts of a Read_result constituting the result of a search. */
#define rr_get_result(rr)   (((rr).result_ri_rj >> 16) & 0xff)
#define rr_get_result_i(rr) (((rr).result_ri_rj >>  8) & 0xff)
#define rr_get_result_j(rr) (((rr).result_ri_rj >>  0) & 0xff)

/* Set corresponding parts. */
#define rr_set_result_ri_rj(rr, result, ri, rj) \
	(rr).result_ri_rj \
	    = (((((result) << 8) | ((ri) & 0xff)) << 8) | ((rj) & 0xff))

/*
 * The hash table consists of hash nodes.  Each hash node consists of
 * The hash value for the position it holds, the position itself and
 * the actual information which is purpose of the table from the start.
 *
 * There is also a pointer to another hash node which is used when
 * the nodes are sorted into hash buckets (see below).
 */

typedef struct hashnode_t {
  unsigned int         hashval;	/* The hash value... */
  Hashposition         position; /* ...for this position. */
  Read_result        * results;	/* And here are the results of previous */
				/*    readings */

  struct hashnode_t  * next;
} Hashnode;


/*
 * The hash table consists of three parts:
 * - The hash table proper: a number of hash buckets with collisions
 *   being handled by a linked list.
 * - The hash nodes.  These are allocated at creation time and are 
 *   never removed or reallocated in the current implementation.
 * - The results of the searches.  Since many different searches can
 *   be done in the same position, there should be more of these than
 *   hash nodes.
 */

typedef struct hashtable {
  int            hashtablesize;	/* Number of hash buckets */
  Hashnode    ** hashtable;	/* Pointer to array of hashnode lists */

  int            num_nodes;	/* Total number of hash nodes */
  Hashnode     * all_nodes;	/* Pointer to all allocated hash nodes. */
  int            free_node;	/* Index to next free node. */

  int            num_results;	/* Total number of results */
  Read_result  * all_results;	/* Pointer to all allocated results. */
  int            free_result;	/* Index to next free result. */
} Hashtable;



void          hash_init(void);
unsigned long board_hash(board_t board[MAX_BOARD][MAX_BOARD], 
			 int koi, int koj /*, int to_move */);
void          board_to_position(board_t board[MAX_BOARD][MAX_BOARD], 
				int koi, int koj, /* int to_move, */
				Hashposition *pos);

int           hashposition_compare(Hashposition *pos1, 
				   Hashposition *pos2);
void          hashposition_dump(Hashposition *pos, FILE *outfile);

void hashdata_init(Hash_data *hd);
void hashdata_set_ko(Hash_data *hd, int i, int j);
void hashdata_remove_ko(Hash_data *hd);
void hashdata_invert_stone(Hash_data *hd, int i, int j, int color);
void hashdata_set_tomove(Hash_data *hd, int to_move);

void read_result_dump(Read_result *result, FILE *outfile);


int         hashtable_init(Hashtable *table, 
			   int tablesize, int num_nodes, int num_results);
Hashtable * hashtable_new(int tablesize, int num_nodes, int num_results);
void        hashtable_clear(Hashtable *table);

Hashnode *  hashtable_enter_position(Hashtable *table,
				     Hashposition *pos, unsigned long hash);
void        hashtable_delete(Hashtable *table,
			     Hashposition *pos, unsigned long hash);
Hashnode *  hashtable_search(Hashtable *table, 
			     Hashposition *pos, unsigned long hash);
void        hashtable_dump(Hashtable *table, FILE *outfile);

Read_result *  hashnode_search(Hashnode *node, int routine, int i, int j);
Read_result *  hashnode_new_result(Hashtable *table, Hashnode *node, 
				   int routine, int ri, int rj);
void           hashnode_dump(Hashnode *node, FILE *outfile);

#endif


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
