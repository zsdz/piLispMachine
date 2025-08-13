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





#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "sgf_utils.h"
#include "ttsgf.h"
#include "ttsgf_read.h"
#include "sgf_properties.h"

#include "../engine/liberty.h"
#include "../interface/interface.h"


/*
 * Convert a 2 character (move) string to its short value.
 * Primarily used for the switch() over the SGF tags.
 */

short
str2short(char *str)
{
  return (str[0] | str[1] << 8);
}


/* 
 * Return the integer X move.
 */

int
get_moveX(SGFPropertyP property)
{
  if (!property->value[1])
    return board_size;

  return toupper(property->value[1]) - 'A';
}


/* 
 * Return the integer Y move.
 */

int
get_moveY(SGFPropertyP property)
{
  if (!property->value[0])
    return board_size;

  return toupper(property->value[0]) - 'A';
}


/*
 * Test if the move is a pass or not.  Return 1 if it is.
 */

int
is_pass(int i, int j)
{
  /* check for FF3 style pass: [tt] if boardsize <=19... */
  if (get_boardsize()<=19)
    if (i==19 || j==19)
      return 1;

  /* Use an || here just in case... (should really be &&) */
  if (i==get_boardsize() || j==get_boardsize())
    return 1;

  return 0;
}



/* 
 * Debugging function to print properties as they are traversed.
 */

int
show_sgf_properties(SGFNodeP node)
{
  SGFPropertyP sgf_prop;
  int propcount;

  propcount=0;

  printf("P: ");
  if(!node->prop) {
    printf("None\n");
    return propcount;
  }
  else {
    sgf_prop = node->prop;
    while (sgf_prop) {
      printf("%c%c ",sgf_prop->name & 0x00FF, (sgf_prop->name & 0xFF00)>>8);
      sgf_prop=sgf_prop->next;
      propcount++;
    }

    printf("(%d) ",propcount);
    if(node->next) printf("n");
    if(node->child) printf("c");
    printf("\n");
  }

  return propcount;
}


/*
 * Recursively traverse each node showing all properties.
 */

int
show_sgf_tree(SGFNodeP node)
{
  int n=0; /* number of nodes */
  
  n++;
  show_sgf_properties(node);

  /* must search depth first- siblings are equal! */
  if (node->child)
    n += show_sgf_tree(node->child);

  if (node->next)
    n += show_sgf_tree(node->next);
  
  return n;
}


/*
 * Determine if a node has a mark property in it.
 */

int
is_markup_node(SGFNodeP node)
{
  SGFPropertyP sgf_prop;
  
  /* If the node has no properties, there's nothing to do.
     This should have been checked by the caller, but it can't hurt. */
  if(!node->prop)
    return 0;
  else {
    sgf_prop = node->prop;
    while (sgf_prop) {
      switch(sgf_prop->name) {
      case SGFCR : 
      case SGFSQ : /* Square */
      case SGFTR : /* Triangle */
      case SGFMA : /* Mark */
      case SGFBM : /* bad move */
      case SGFDO : /* doubtful move */
      case SGFIT : /* interesting move */
      case SGFTE : /* good move */
	return 1;
	break;
      default :
	break;
      }
      sgf_prop=sgf_prop->next;
    }
  }

  /* No markup property found. */
  return 0;
}


/*
 * Determine if the node has a move in it.
 */

int
is_move_node(SGFNodeP node)
{
  SGFPropertyP sgf_prop;
  
  /* If the node has no properties, there's nothing to do.
     This should have been checked by the caller, but it can't hurt. */
  if (!node->prop)
    return 0;
  else {
    sgf_prop = node->prop;
    while (sgf_prop) {
      switch(sgf_prop->name) {
      case SGFB : 
      case SGFW : 
	return 1;
	break;
      default :
	break;
      }
      sgf_prop=sgf_prop->next;
    }
  }

  return 0;
}


/*
 * Determine whose move is in the node.
 */

int
find_move(SGFNodeP node)
{
  SGFPropertyP sgf_prop;
  
  /* If the node has no properties, there's nothing to do.
     This should have been checked by the caller, but it can't hurt. */
  if (!node->prop)
    return 0;
  else {
    sgf_prop = node->prop;
    while (sgf_prop) {
      switch(sgf_prop->name) {
      case SGFB : 
	return BLACK;
	break;
      case SGFW : 
	return WHITE;
	break;
      default :
	break;
      }
      sgf_prop=sgf_prop->next;
    }
  }
  return EMPTY;
}


/*
 * Look for a testmode=... comment in the top node of the sgf file
 * and, if found, return it.
 */

enum testmode
guess_mode_from_sgf_comment(SGFNodeP sgf)
{
  SGFPropertyP prop;
  for (prop = sgf->prop ; prop ; prop=prop->next) {
    if (prop->name == SGFC) {
      /* a comment; Is it  testmode=... ? */
      if (strncmp(prop->value, "testmode=", 9) != 0)
	continue;

      /* Yes it was!  Now find out which testmode it is. */
      if (strncmp(prop->value + 9, "game", 4) == 0)
	return GAME;
      if (strncmp(prop->value + 9, "both", 4) == 0)
	return BOTH;
      if (strncmp(prop->value + 9, "move", 4) == 0)
	return MOVE_ONLY;
      if (strncmp(prop->value + 9, "annotation", 10) == 0)
	return ANNOTATION_ONLY;
    }
  }

  return UNKNOWN_TESTMODE;
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
