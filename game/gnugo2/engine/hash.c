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
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "liberty.h"
#include "hash.h"


/*
 * This file, together with engine/hash.h implements hashing of go positions
 * using a method known as Zobrist hashing.  See the Texinfo documentation
 * (Reading/Hashing) for more information.  
 */

/* Forward declarations of all static functions to make gcc happy. */

static int  is_initialized = 0;

/* Random values for the hash function.  For stones and ko position. */
static unsigned long  white_hash[MAX_BOARD][MAX_BOARD];	
static unsigned long  black_hash[MAX_BOARD][MAX_BOARD];	
static unsigned long  ko_hash[MAX_BOARD][MAX_BOARD];

/* Bit patterns for white stones in the compact board representation. */
static unsigned long white_patterns[] = {
  0x00000001, 0x00000004, 0x00000010, 0x00000040, 
  0x00000100, 0x00000400, 0x00001000, 0x00004000,
  0x00010000, 0x00040000, 0x00100000, 0x00400000,
  0x01000000, 0x04000000, 0x10000000, 0x40000000,
};

/* Bit patterns for black stones in the compact board representation. */
static unsigned long black_patterns[] = {
  0x00000002, 0x00000008, 0x00000020, 0x00000080, 
  0x00000200, 0x00000800, 0x00002000, 0x00008000,
  0x00020000, 0x00080000, 0x00200000, 0x00800000,
  0x02000000, 0x08000000, 0x20000000, 0x80000000,
};



/*
 * Initialize the entire hash system.
 */

void
hash_init()
{
  int  i;
  int  j;

#if !TRACE_READ_RESULTS
#ifdef HAVE_RANDOM
  srandom(time(0));
#else
  srand(time(0));
#endif
#else
#ifdef HAVE_RANDOM
  srandom(0);
#else
  srand(0);
#endif
#endif
  for (i = 0; i < MAX_BOARD; ++i)
    for (j = 0; j < MAX_BOARD; ++j) {

#ifdef HAVE_RANDOM
      white_hash[i][j] = random();
      black_hash[i][j] = random();
      ko_hash[i][j]    = random();
#else
      white_hash[i][j] = rand();
      black_hash[i][j] = rand();
      ko_hash[i][j]    = rand();
#endif
    }
  is_initialized = 1;
}


/*
 * Take a go position consisting of the board and a possible k,
 * and generate a hash value from it.
 * 
 * See the Texinfo documentation (Reading/Hashing) for more information.
 */

unsigned long
board_hash(board_t board[MAX_BOARD][MAX_BOARD], 
	   int koi, int koj)
{
  unsigned int  hash;
  int           i;
  int           j;

  /* If the hash system is not initialized, do so now. */
  if (!is_initialized)
    hash_init();

  /* Get the hash value for all the stones on the board. */
  hash = 0;
  for (i = 0; i < board_size; ++i)
    for (j = 0; j < board_size; ++j) {
      switch (board[i][j]) {
      case WHITE:
	hash = hash ^ white_hash[i][j];
	break;
      case BLACK:
	hash = hash ^ black_hash[i][j];
	break;
      default:
	break;
      }
    }

  /* If there is a ko going on, take this into consideration too. */
  if (koi != -1) 
    hash ^= ko_hash[koi][koj];

  return hash;
}


/*
 * Take a go position consisting of the board and a possible ko,
 * and store this in a compact form into *pos.
 */

void
board_to_position(board_t board[MAX_BOARD][MAX_BOARD], 
		  int koi, int koj,  Hashposition *pos)
{
  int            index;
  int            subindex;
  unsigned long  bits;
  int            i, j;

  /* Initialize the compact board. */
  for (i = 0; i < COMPACT_BOARD_SIZE; i++)
    pos->board[i] = 0;

  /* Put 16 locations into one long using 2 bits per location. */
  index = 0;
  subindex = 0;
  bits = 0;
  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++) {
      if (board[i][j] != EMPTY) {
	if (board[i][j] == WHITE)
	  bits |= white_patterns[subindex];
	else if (board[i][j] == BLACK)
	  bits |= black_patterns[subindex];
      }

      if (++subindex == 16) {
	pos->board[index++] = bits;
	bits = 0;
	subindex = 0;
      }
    }
  /* Store the last long into the compact board. 
     We only need to do this if we didn't just store it inside the loop. */
  if (subindex != 0)
    pos->board[index] = bits;

  /* The rest of the info, beside the board. */
  pos->ko_i    = koi;
  pos->ko_j    = koj;
}


/*
 * Initialize a Hash data to the empty state.  This means an
 * empty board and no ko.
 */

void
hashdata_init(Hash_data *hd)
{
  int  i;

  hd->hashval = board_hash(p, ko_i, ko_j);
  board_to_position(p, ko_i, ko_j, &(hd->hashpos));
  return;

  for (i = 0; i < COMPACT_BOARD_SIZE; ++i)
    hd->hashpos.board[i] = 0;
  hd->hashpos.ko_i = -1;
  hd->hashpos.ko_j = -1;
}


/*
 * Remove any ko from the hash value and hash position.
 */
void
hashdata_remove_ko(Hash_data *hd)
{
  if (hd->hashpos.ko_i != -1) {
    hd->hashval ^= ko_hash[hd->hashpos.ko_i][hd->hashpos.ko_j];

    hd->hashpos.ko_i = -1;
    hd->hashpos.ko_j = -1;
  }
}


void
hashdata_set_ko(Hash_data *hd, int i, int j)
{
  hd->hashval ^= ko_hash[i][j];

  hd->hashpos.ko_i = i;
  hd->hashpos.ko_j = j;
}


/*
 * Set or remove a stone of COLOR at (I, J) in a Hash_data.
 */

void
hashdata_invert_stone(Hash_data *hd, int i, int j, int color)
{
  int  index = (i * board_size + j) / 16;
  int  subindex = (i * board_size + j) % 16;

  if (color == BLACK) {
    hd->hashval ^= black_hash[i][j];
    hd->hashpos.board[index] ^= black_patterns[subindex];
  } else if (color == WHITE) {
    hd->hashval ^= white_hash[i][j];
    hd->hashpos.board[index] ^= white_patterns[subindex];
  }
}


/*
 * Dump an ASCII representation of the contents of a Hashposition onto
 * the FILE outfile. 
 */

void
hashposition_dump(Hashposition *pos, FILE *outfile)
{
  int  i;

  fprintf(outfile, "Board:  ");
  for (i = 0; i < COMPACT_BOARD_SIZE; ++i)
     fprintf(outfile, " %lx", pos->board[i]);

  if (pos->ko_i == -1)
    fprintf(outfile, "  No ko");
  else
    fprintf(outfile, "  Ko position: (%d, %d)", pos->ko_i, pos->ko_j);

}


/*
 * Dump an ASCII representation of the contents of a Read_result onto
 * the FILE outfile. 
 */

void
read_result_dump(Read_result *result, FILE *outfile)
{
  fprintf(outfile, "Rutin %d, (%d, %d), djup: %d ",
	  ((result->routine_i_j_stackp) >> 24) & 0xff,
	  ((result->routine_i_j_stackp) >> 16) & 0xff,
	  ((result->routine_i_j_stackp) >> 8) & 0xff,
	  ((result->routine_i_j_stackp) >> 0) & 0xff);
  fprintf(outfile, "Resultat: %d, (%d, %d)\n",
	  ((result->result_ri_rj) >> 16) & 0xff,
	  ((result->result_ri_rj) >> 8) & 0xff,
	  ((result->result_ri_rj) >> 0) & 0xff);
}


/* Return 0 if *pos1 == *pos2, otherwise return 1.
 * This adheres (almost) to the standard compare function semantics 
 * which are used e.g. by the comparison functions used in qsort().
 */

int
hashposition_compare(Hashposition *pos1, Hashposition *pos2)
{
  int  i;

  if (pos1->ko_i != pos2->ko_i
      || pos1->ko_j != pos2->ko_j)
    return 1;

  /* We need only compare to board_size.  MAX_BOARD is not necessary. */
  for (i = 0; i < COMPACT_BOARD_SIZE; i++)
    if (pos1->board[i] != pos2->board[i])
      return 1;

  return 0;
}


/*
 * Dump an ASCII representation of the contents of a Hashnode onto
 * the FILE outfile. 
 */

void
hashnode_dump(Hashnode *node, FILE *outfile)
{
  Read_result  * result;

  /* Data about the node itself. */
  fprintf(outfile, "Hash value: %d\n", (int) node->hashval);
  hashposition_dump(&(node->position), outfile);

  result = node->results;
  while (result) {
    read_result_dump(result, outfile);
    result = result->next;
  }
  /* FIXME later: Dump contents of data also. */
}



/*
 * Initialize a hash table for a given total size and size of the
 * hash table.
 *
 * Return 0 if something went wrong.  Just now this means that there
 * wasn't enough memory available.
 */

int
hashtable_init(Hashtable *table, 
	       int tablesize, int num_nodes, int num_results)
{
  /* If the hash system is not initialized, do so now. */
  if (!is_initialized)
    hash_init();

  /* Allocate memory for the pointers in the hash table proper. */
  table->hashtablesize = tablesize;
  table->hashtable = (Hashnode **) malloc(tablesize * sizeof(Hashnode *));
  if (table->hashtable == NULL) {
    free(table);
    return 0;
  }

  /* Allocate memory for the nodes. */
  table->num_nodes = num_nodes;
  table->all_nodes = (Hashnode *) malloc(num_nodes * sizeof(Hashnode));
  if (table->all_nodes == NULL) {
    free(table->hashtable);
    free(table);
    return 0;
  }

  /* Allocate memory for the results. */
  table->num_results = num_results;
  table->all_results = (Read_result *) malloc(num_results 
					      * sizeof(Read_result));
  if (table->all_results == NULL) {
    free(table->hashtable);
    free(table->all_nodes);
    free(table);
    return 0;
  }

  /* Initialize the table and all nodes to the empty state . */
  hashtable_clear(table);

  return 1;
}


/*
 * Allocate a new hash table and return a pointer to it. 
 *
 * Return NULL if there is insufficient memory.
 */

Hashtable *
hashtable_new(int tablesize, int num_nodes, int num_results)
{
  Hashtable  * table;

  /* If the hash system is not initialized, do so now. */
  if (!is_initialized)
    hash_init();

  /* Allocate the hashtable struct. */
  table = (Hashtable *) malloc(sizeof(Hashtable));
  if (table == NULL)
    return NULL;

  /* Initialize the table. */
  if (!hashtable_init(table, tablesize, num_nodes, num_results)) {
    free(table);
    return NULL;
  }

  return table;
}


/*
 * Clear an existing hash table.  
 */

void
hashtable_clear(Hashtable *table)
{
  int  i;

  /* Initialize all hash buckets to the empty list. */
  for (i = 0; i < table->hashtablesize; ++i)
    table->hashtable[i] = NULL;

  table->free_node = 0;
  table->free_result = 0;
}


/*
 * Enter a position with a given hash value into the table.  Return 
 * a pointer to the hash node where it was stored.  If it is already
 * there, don't enter it again, but return a pointer to the old one.
 */

Hashnode *
hashtable_enter_position(Hashtable *table, 
			 Hashposition *pos, unsigned long hash)
{
  Hashnode  * node;
  int         bucket;

  /* If the position is already in the table, return a pointer to it. */
  node = hashtable_search(table, pos, hash);
  if (node != NULL) {
    return node;
  }

  /* If the table is full, return NULL */
  if (table->free_node == table->num_nodes)
    return NULL;

  /* It wasn't there and there is still room.  Allocate a new node for it... */
  node = &(table->all_nodes[table->free_node++]);
  node->hashval = hash;
  node->position = *pos;
  node->results = NULL;

  /* ...and enter it into the table. */
  bucket = hash % table->hashtablesize;
  node->next = table->hashtable[bucket];
  table->hashtable[bucket] = node;

  stats.position_entered++;
  return node;
}


/*
 * Currently we can't remove single nodes from the table.
 */

#if 0
void
hashtable_delete(Hashtable *table,
		 board_t board[MAX_BOARD][MAX_BOARD])
{
}
#endif


/* 
 * Given a Hashposition and a Hash value, find the hashnode which contains
 * this very position with the given hash value.  
 *
 * We could compute the hash value within this functions, but later
 * when we have incremental calculation of the hash function, this 
 * would be dumb. So we demand the hash value from outside from the 
 * very beginning.
 */

Hashnode *
hashtable_search(Hashtable *table, Hashposition *pos, unsigned long hash)
{
  Hashnode     * node;
  int            bucket;

  bucket = hash % table->hashtablesize;
  node = table->hashtable[bucket];
  while (node != NULL
	 && (node->hashval != hash
	     || hashposition_compare(pos, &(node->position)) != 0))
    node = node->next;

  return node;
}


/*
 * Dump an ASCII representation of the contents of a Hashtable onto
 * the FILE outfile. 
 */

void
hashtable_dump(Hashtable *table, FILE *outfile)
{
  int  i;
  Hashnode * hn;

  /* Data about the table itself. */
  fprintf(outfile, "Dump of hashtable\n");
  fprintf(outfile, "Total size: %d\n", table->num_nodes);
  fprintf(outfile, "Size of hash table: %d\n", table->hashtablesize);
  fprintf(outfile, "Number of positions in table: %d\n", table->free_node);

  /* Data about the contents. */
  for (i = 0; i < table->hashtablesize; ++i) {
    fprintf(outfile, "Bucket %5d: ", i);
    hn = table->hashtable[i];
    if (hn == NULL)
      fprintf(outfile, "empty");
    else
      while (hn) {
	hashnode_dump(hn, outfile);
	hn = hn->next;
      }
    fprintf(outfile, "\n");
  }
}


/* 
 * Search the result list in a hash node for a particular result. This
 * result is from ROUTINE (e.g. readlad1) at (i, j) and reading depth
 * stackp.
 *
 * All these numbers must be unsigned, and 0<= x <= 255).
 */

Read_result *
hashnode_search(Hashnode *node, int routine, int i, int j)
{
  Read_result  * result;
  unsigned int   search_for;

  search_for = (((((routine << 8) | i) << 8) | j) << 8) | stackp;

  result = node->results;
  while (result != NULL
	 && (result->routine_i_j_stackp != search_for))
    result = result->next;

  return result;
}


/*
 * Enter a new Read_result into a Hashnode.
 * We already have the node, now we just want to enter the result itself.
 * We will fill in the result itself later, so we only need the routine
 * number for now.
 */

Read_result *
hashnode_new_result(Hashtable *table, Hashnode *node, 
		    int routine, int i, int j)
{
  Read_result  * result;

  /* If the table is full, return NULL */
  if (table->free_result == table->num_results)
    return NULL;

  /* There is still room. Allocate a new node for it... */
  result = &(table->all_results[table->free_result++]);

  /* ...and enter it into the table. */
  result->next = node->results;
  node->results = result;

  /* Now, put the routine number into it. */
  result->routine_i_j_stackp = ((((((routine << 8) | i) << 8) | j) << 8)
				| stackp);

  stats.read_result_entered++;
  return result;

}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
