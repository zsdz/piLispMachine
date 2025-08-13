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

#include "interface.h"
#include "../sgf/sgf.h"
#include "../sgf/sgf_utils.h"
#include "../sgf/ttsgf.h"
#include "../sgf/ttsgf_read.h"
#include "../sgf/sgf_properties.h"
#include "../sgf/sgfana.h"
#include "../engine/liberty.h"


#undef DEBUG_TRAVERSE_SGF


static int parse_sgf_properties(SGFNodeP, enum testmode mode);
static int parse_sgf_tree(SGFNodeP, enum testmode mode);
static int test_move(int, int, SGFPropertyP);

/* move not found, found, and quality */
enum quality {
  NF=0,
  FOUND, /* genmove matches board move */
  CONSIDERED,  /* game mode only : we at least considered the board move */
  BAD,
  OK,
  GOOD
};


/* --------------------------------------------------------------*/
/* test moves */
/* --------------------------------------------------------------*/
void play_test(SGFNodeP sgf_head, enum testmode mode)
{
  int tmpi;
  float tmpf;
  char *tmpc=NULL;

  SGFNodeP sgf_ptr;

  /* start the move numbering at 0... */
  /* when playing with gmp it starts at 0 /NE */
  set_movenumber(0);


   /* a debug tool to dump out the entire SGF tree.. useful!! */
   if(0) {sgf_ptr=sgf_head; printf("Nodes: %d\n\n",show_sgf_tree(sgf_ptr));}


   if (!sgfGetIntProperty(sgf_head, "SZ", &tmpi))
   {
     printf("Couldn't find the size (SZ) attribute!\n");
     exit(EXIT_FAILURE);
   }
   set_boardsize(tmpi);
   
   if(sgfGetIntProperty(sgf_head, "HA", &tmpi))
   {
     set_handicap(tmpi=sethand(tmpi));
     
   }
   if(sgfGetFloatProperty(sgf_head, "KM", &tmpf))
     set_komi(tmpf);

   if (!get_opt_quiet())
   {
     if(sgfGetCharProperty(sgf_head, "RU", &tmpc))
       printf("Ruleset:      %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "GN", &tmpc))
       printf("Game Name:    %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "DT", &tmpc))
       printf("Game Date:    %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "GC", &tmpc))
       printf("Game Comment: %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "US", &tmpc))
       printf("Game User:    %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "PB", &tmpc))
       printf("Black Player: %s\n", tmpc);
     if(sgfGetCharProperty(sgf_head, "PW", &tmpc))
       printf("White Player: %s\n", tmpc);
     
     print_ginfo();
   }

   sgf_write_game_info(get_boardsize(), get_handicap(), 
		       get_komi(), get_seed(), "test variations");         
   
   
   /* Now to actually run through the file.  This is the interesting part.
    * We need to traverse the SGF tree, and every time we encounter a node
    * we need to check what move GNUgo would make, and see if it is OK. */
   sgfSetLastNode(sgf_head);
   parse_sgf_tree(sgf_head, mode);
   
   sgf_close_file();
   
}

/* return 1 if the property value is move i,j */

static int test_move(int i, int j, SGFPropertyP prop)
{
  int x, y;
  
  x=get_moveX(prop);
  y=get_moveY(prop);
  
  return (i==x) && (j==y);
}



/* Action this node, ie parse the properties in the node, handle moves, added stones,
 * etc. and test for the goodness of moves
 */

static int parse_sgf_properties(SGFNodeP node, enum testmode testmode)
{
  SGFPropertyP sgf_prop;  /* iterate over properties of the node */
  SGFPropertyP move_prop = NULL; /* remember if we see a move property */
  int needtotest;
  int move;    /* color of move to be made at this node - EMPTY means don't move */
  
  int is_markup=0;

  /* We make several passes over the properties.
   *  i)  add stones
   *  ii) make and test the computer move, if necessary
   *  iii) make the board move
   */

  /* first pass : action any AB / AW properties, and note presence of move / markup properties */

  for (sgf_prop = node->prop ; sgf_prop ; sgf_prop = sgf_prop->next)
  {
    DEBUG(DEBUG_LOADSGF, "%c%c[%s]\n", sgf_prop->name & 0xff, (sgf_prop->name >> 8), sgf_prop->value);
    switch(sgf_prop->name)
    {
    case SGFAB : 
      /* add black */
      put_stone(get_moveX(sgf_prop), get_moveY(sgf_prop), BLACK);
      break;
    case SGFAW : 
      /* add white */
      put_stone(get_moveX(sgf_prop), get_moveY(sgf_prop), WHITE);
      break;
    case SGFB:
    case SGFW:
      move_prop = sgf_prop;  /* remember it for later */
      break;
    case SGFCR : 
    case SGFSQ : 
    case SGFTR : 
    case SGFMA : 
    case SGFBM :
    case SGFDO :
    case SGFIT :
    case SGFTE :
      is_markup = 1;
      break;
    }
  }



  /* now figure out what tests to perform at this node */

  move=EMPTY;
  needtotest=1;

  /* if we are only testing one type of node... if BOTH, no check
   * is needed here */
  if ( !move_prop && (testmode==MOVE_ONLY || testmode==GAME))
    needtotest=0;

  if( !is_markup && testmode==ANNOTATION_ONLY)
    needtotest=0;

  /* if we have found a marked up node, then generate a computer
   * move and test if it is OK 
   * The move that is made in the node (if any) is also
   * considered good
   */

  if(needtotest) 
  {
    /* figure out whose move it is */

    if (move_prop)
      move = move_prop->name == SGFW ? WHITE : BLACK;
    else
    {
      printf("Node has no move... checking child node for move color\n");
      if(node->child)
      {
	move = find_move(node->child);
	if(move!=WHITE && move!=BLACK) 
	{
	  printf("Child node has no move... Unable to determine move color\n");
	  printf("Not testing this node\n");
	}
      }
    }
  }


  /* now, if we are on for a move, make it, and figure out the quality */

  if (move != EMPTY)
  {
      enum quality nq=NF;       /* node quality (ie quality of board move, eg GM, BM, ...) */
      enum quality mq=NF;       /* move quality (eg generated move was on a TR[], CR[], MA[] ) */

      int i, j;
      int move_from_file_x=0;
      int move_from_file_y=0;

      /* get a move from the engine for color 'move' */
      get_test_move(&i, &j, move);


      if (testmode == GAME)
      {
	/* look to see if our move was considered */
	assert(move_prop);  /* we should not get here otherwise */
	move_from_file_x=get_moveX (move_prop);
	move_from_file_y=get_moveY (move_prop);

	if ((i==move_from_file_x)&&(j==move_from_file_y))
	  {
	    mq = FOUND;
	  }
	else 
	  {
	    if ((i >= 0) && (i < board_size) && (j >= 0) && (j < board_size))
	      {
		if (potential_moves[move_from_file_x][move_from_file_y] > 0)
		  {
		    mq = CONSIDERED;
		  }
		else
		  {
		    /* The move at (move_from_file_x,move_from_file_y) was not considered by GNU Go. */
		    int xx,yy;
		    printf ("// Start\n");
		    for (xx=0;xx<board_size;xx++)
		      {
			printf ("//  ");
			for (yy=0;yy<board_size;yy++)
			  {
			    if ((xx==move_from_file_x)&&(yy==move_from_file_y))
			      {
				if (move==BLACK)
				  {
				    printf ("x");
				  }
				else
				  {
				    printf ("o");
				  }
			      }
			    else
			      {
				switch (p[xx][yy])
				  {
				  case EMPTY:
				    printf (".");
				    break;
				    
				  case BLACK:
				    printf ("X");
				    break;
				    
				  case WHITE:
				    printf ("O");
				    break;
				    
				  default:
				    printf ("?");
				  }
			      }
			  }
			printf ("\n");
		      }
		    printf ("// End\n");
		  }
	      }
	  }
      }
      else
      {
	/* iterate over the properties to test the quality */

	for (sgf_prop = node->prop ; sgf_prop ; sgf_prop = sgf_prop->next)
	{
	  switch(sgf_prop->name) {
	  case SGFN  : /* node name */
	    if (!get_opt_quiet()) printf("Node Name: %s\n",sgf_prop->value);
	    break;
	  case SGFC  : /* comment */
	    if (!get_opt_quiet()) printf("Comment: %s\n",sgf_prop->value);
	    break;
	    
	  case SGFBM : /* Bad move */
	  case SGFDO : /* doubtful move */
	    nq = BAD;
	    break;
	  case SGFTE : /* Good move (tesuki) */
	    nq= GOOD;
	    break;
#if 0  /* I don't think we can make judgements about interesting moves ! */
	  case SGFIT : /* interesting move */
	    nq= FOUND;
	    break;
#endif
	    
	  case SGFB : 
	  case SGFW : 
	    move_from_file_x=get_moveX (sgf_prop);
	    move_from_file_y=get_moveY (sgf_prop);
	    /* if the move has already matched an annotation, ignore */
	    if (mq == NF && test_move(i,j,sgf_prop))
	      mq=FOUND;  /* genmove matches board move */
	    break;
	    
	    /* ANNOTATION PROPERTIES:
	     * Circle (CR):   Good Move
	     * Triangle (TR): OK move
	     * Square (SQ , MA):   Bad move   (Mark too... cgoban1.9.3 writes MA instead of SQ)
	     */
	    
	  case SGFCR : 
	    if (test_move(i,j,sgf_prop))
	      mq = GOOD;
	    break;
	    
	  case SGFSQ : 
	  case SGFMA : 
	    if (test_move(i,j,sgf_prop))
	      mq=BAD;
	    break;
	    
	  case SGFTR : 
	    if (test_move(i,j,sgf_prop))
	      mq=OK;
	    break;
	  }
	}  /* iterate over properties */
      } /* test mode */
	
      /* now report on how well the computer generated the move */

      printf("Move %d (%s): ",
	     get_movenumber()+1,
	     move == WHITE ? "White" : "Black");
      
      printf ("GNU Go plays ");
      if((i==get_boardsize()) && (j==get_boardsize()))
	printf("PASS. ");
      else
	mprintf("%m. ",i,j);

      printf ("Game move is ");
      if((move_from_file_x>=get_boardsize()) && (move_from_file_y>=get_boardsize()))
	printf("PASS. ");
      else
	mprintf("%m. ",move_from_file_x,move_from_file_y);
      
      /* if the generated move matches the board move and we have no other
       * information, move quality is node quality except in game mode (?)
       */
      
      if (testmode != GAME && mq == FOUND)
	mq = nq;

      printf ("Result: ");
      switch(mq) {
      case BAD:   printf("BAD");break;
      case OK:    printf("OK");break;
      case GOOD:  printf("GOOD");break;
      case FOUND: printf("FOUND");break;
      case CONSIDERED: printf("CONSIDERED");break;
      case NF:    printf("NOT FOUND");break;
      default:    printf("UNKNOWN!!??!!");break;
      }
      printf("\n");
  }  /* if testmove */

  /* finally, if this node contains a move, apply it to the board */

  if (move_prop)
  {
    int m,n;
    switch(move_prop->name)
    {
    case SGFB : 
      /* black move */
      m=get_moveX(move_prop);
      n=get_moveY(move_prop);
      if(!is_pass(m,n)) put_move(m,n, BLACK);
      inc_movenumber();
      break;
    case SGFW : 
      /* white move */
      m=get_moveX(move_prop);
      n=get_moveY(move_prop);
      if(!is_pass(m,n)) put_move(m,n, WHITE);
      inc_movenumber();
      break;
    }
    sgfSetLastNode(node);
    if(needtotest) sgfAnalyzer(0); /*from where do I get the moveval*/

  }
  return 1;
}



	/* recursively traverse each node showing all properties */
static int parse_sgf_tree(SGFNodeP node, enum testmode mode)
{
  board_t **board=NULL;
#ifdef DEBUG_TRAVERSE_SGF
  static int next_level=0;
  static int child_level=0;
#endif

  /* if there is a different variation to be taken, this is the point to
   * save a copy of the board
   * if, however, there are many 'next' nodes branching off from the same
   * node, then only one board needs to be saved.  Fortunately, this
   * functionality is inherent in the design of the recursive traversal
   */

  if((node->next)&&(!analyzerflag))  /*do not test variations when in analyzer mode (because analyzer mode adds variations)*/
  {
    board= make_board();
    get_board(board);
  }
  
#ifdef DEBUG_TRAVERSE_SGF
  /* show the properties for this node */
  show_sgf_properties(node);
#endif

  parse_sgf_properties(node, mode);

  /* must search depth first- siblings are equal level, i.e. different
   * lines in the game, not the continuation of one line.
   * Children are along the same game line
   */

  if(node->child) 
  {
#ifdef DEBUG_TRAVERSE_SGF
    printf("Child Level: %d\n",++child_level);
#endif
    parse_sgf_tree(node->child, mode);
#ifdef DEBUG_TRAVERSE_SGF
    child_level--;
#endif
  }

  /* every time you are done with child nodes (meaning moves added
   * to the same variation) decrement the move counter
   */

  if(is_move_node(node))
    dec_movenumber();

  /* if there is a next node at the same level
   * next nodes are different game lines
   * next BRANCH... so we really need to restore the board to the
   * parent's board state, and kill the copy
   */

  if((node->next)&&(!analyzerflag))  /*do not test variations when in analyzer mode (because analyzer mode adds variations)*/
  {
#ifdef DEBUG_TRAVERSE_SGF
    printf("Next Level: %d\n",++next_level);
#endif
    /* restore the board at that level */
    put_board(board);
    /* done with this board */
    kill_board(board);
    parse_sgf_tree(node->next, mode);
#ifdef DEBUG_TRAVERSE_SGF
    next_level--;
#endif
  }


  return 1;
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
