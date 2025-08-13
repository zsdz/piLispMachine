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

/*  Parts of this code were given to us by Tommy Thorn */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "ttsgf.h"
#include "sgf_properties.h"
#include "../engine/liberty.h"

PUBLIC_VARIABLE int board_size;


SGFNodeP lastnode=0; /*last node where move was placed*/

void gg_snprintf(char *dest, unsigned long len, const char *fmt, ...)
{
	va_list args;
	va_start(args,fmt);
#ifdef HAVE_SNPRINTF
	vsnprintf(dest,len,fmt,args);
#elif defined HAVE_LIBGLIB
	g_vsnprintf(dest,len,fmt,args);
#else
	len = len; /* Eliminate warning about unused parameter. */
	vsprintf(dest,fmt,args);
#endif
	va_end(args);
}

/*
 * Utility: a checking, initializing malloc
 */

void *
xalloc(unsigned int size)
{
  void *pt = malloc(size);

  if (!pt) {
    fprintf(stderr, "%s: Out of memory!\n",__FUNCTION__);
    exit(2);
  }

  memset(pt, 0, (unsigned long) size);
  return pt;
}


/*
 * Allocate memory for a new node.
 */

SGFNodeP
sgfNewNode()
{
  SGFNodeP res = xalloc(sizeof(*res));
  return res;
}


void
sgfSetLastNode(SGFNodeP last_node)
{
    lastnode=last_node;
}


/*
 * Add a text property to the gametree.
 */

void sgfAddProperty(SGFNodeP n, const char *name, const char *value)
{
  SGFPropertyP pr = n->prop;

  if (pr)
    while (pr->next)
      pr=pr->next;

  sgfMkProperty(name, value, n, pr);
}


/*
 * Add a integer property to the gametree.
 */

void
sgfAddPropertyInt(SGFNodeP n, const char *name, long v)
{
  char buffer[10];

  gg_snprintf(buffer, 10, "%ld", v);
  sgfAddProperty(n, name, buffer);
}


/*
 * Add a float property to the gametree.
 */

void
sgfAddPropertyFloat(SGFNodeP n, const char *name, float v)
{
  char buffer[10];

  gg_snprintf(buffer, 10, "%3.1f", v);
  sgfAddProperty(n, name, buffer);
}


/*
 * Read a property as int from the gametree.
 */

int
sgfGetIntProperty(SGFNodeP n, const UCHAR_ALIAS *name, int *value)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      *value = atoi(pr->value);
      return 1;
    }

  return 0;
}


/*
 * Read a property as float from the gametree.
 */

int
sgfGetFloatProperty(SGFNodeP n, const UCHAR_ALIAS *name, float *value)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      *value = (float) atof(pr->value);
         /* MS-C warns of loss of data (double to float) */
      return 1;
    }

  return 0;
}


/*
 * Read a property as text from the gametree.
 */

int
sgfGetCharProperty(SGFNodeP n, const UCHAR_ALIAS *name, char **value)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      *value=pr->value;
      return 1;
    }

  return 0;
}


/*
 * Overwrite a property in the gametree with text or create a new
 * one if it does not exist.
 */

void
sgfOverwriteProperty(SGFNodeP n, const UCHAR_ALIAS *name, const char *text)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      /* FIXME: Realloc pr with strlen text. */
      strncpy(pr->value, text, strlen(pr->value));
      return;
    }

  sgfAddProperty(n, name, text);
}


/* 
 * Overwrite a float property in the gametree with v or create
 * a new one if it does not exist.
 */

void
sgfOverwritePropertyFloat(SGFNodeP n, const UCHAR_ALIAS *name, float v)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      /* FIXME: Realloc pr. */
      gg_snprintf(pr->value,strlen(pr->value)+1, "%3.1f", v);
      return;
    }

  sgfAddPropertyFloat(n, name, (float)v);
}


/*
 * Overwrite a int property in the gametree with v or create a new 
 * one if it does not exist.
 */

void
sgfOverwritePropertyInt(SGFNodeP n, const UCHAR_ALIAS *name, int v)
{
  SGFPropertyP pr;
  short nam = name[0] | name[1] << 8;

  for (pr = n->prop; pr; pr=pr->next)
    if (pr->name == nam) {
      /* FIXME: Realloc pr. */
      gg_snprintf(pr->value,strlen(pr->value)+1,"%d",v);
      return;
   }

  sgfAddPropertyFloat(n,name,(float)v);
}


/*
 * Make a property.
 *
 * FIXME: To speed up, we should have MkMoveProperty.
 */

SGFPropertyP
sgfMkProperty(const UCHAR_ALIAS *name,const  UCHAR_ALIAS *value,
	      SGFNodeP n, SGFPropertyP last)
{
  if (strlen(name) == 1) {
    SGFPropertyP pr = xalloc(sizeof(*pr)+strlen(value));

    pr->name = name[0] | (short) (' ' << 8);
    strcpy(pr->value, value);

    if (last == NULL)
      n->prop = pr;
    else
      last->next = pr;

    return pr;
  }
  else {
    SGFPropertyP pr = xalloc(sizeof(*pr)+strlen(value));

    pr->name = name[0] | name[1] << 8;
    strcpy(pr->value, value);

    if (last == NULL)
      n->prop = pr;
    else
      last->next = pr;

    return pr;
  }
}


/*
 * Goto previous node.
 */

SGFNodeP
sgfPrev(SGFNodeP pr)
{
  if (pr->parent) {
    SGFNodeP q = pr->parent->child, prev = NULL;

    while (q && q != pr) {
      prev = q;
      q = q->next;
    }
    return prev;
  } else
    return NULL;
}


/*
 * Goto root node.
 */

SGFNodeP
sgfRoot(SGFNodeP pr)
{
  while (pr->parent)
    pr = pr->parent;

  return pr;
}


/*
 * Add a stone to the current or the given node.
 * Return the node where the stone was added.
 */

SGFNodeP
sgfAddStone(SGFNodeP pr, int color, int movex,int movey)
{
  SGFNodeP node=sgfNodeCheck(pr);
  char move[3];

  sprintf(move,"%c%c",movey+'a',movex+'a');
  sgfAddProperty(node, (color==BLACK) ? "AB":"AW", move);

  return node;
}


/*
 * Add a move to the gametree.
 */

SGFNodeP
sgfAddPlay(SGFNodeP pr, int who, int movex,int movey)
{
  char move[3];
  SGFNodeP new;
  SGFNodeP last= sgfNodeCheck(pr);

  /* a pass move? */
  if ((movex==movey) && (movey==board_size))
     move[0]=0;
  else
    sprintf(move,"%c%c",movey+'a',movex+'a');

  if (last->child) {
    lastnode=sgfStartVariantFirst(last->child);
    sgfAddProperty(lastnode,(who==BLACK) ? "B":"W",move);

    return lastnode;
  }
  else {
    new = sgfNewNode();
    last->child=new;
    new->parent=last;
    lastnode=new;
    sgfAddProperty(new, (who==BLACK) ? "B":"W", move);

    return new;
  }
}


SGFNodeP
sgfCreateHeaderNode(float komi)
{
    SGFNodeP root=sgfNewNode();

    sgfAddPropertyInt(root, "SZ", board_size);
    sgfAddPropertyFloat(root, "KM", komi);
    sgf_root=root;

    return root;
}


/*
 * Add a comment to the gametree.
 */

SGFNodeP
sgfAddComment(SGFNodeP pr, const char *comment)
{
  SGFNodeP ret;

  assert(sgf_root);
  ret=sgfNodeCheck(pr);
  sgfAddProperty(ret,"C ",comment);

  return ret;
}


/*
 * Returns the node to modify. When p==0 then lastnode is used,
 * except if lastnode is NULL, then the current end of game is used.
 */

SGFNodeP
sgfNodeCheck(SGFNodeP pr)
{
   SGFNodeP last=pr;

   assert(sgf_root);
   if (!last) {
     if (lastnode)
       last=lastnode;
     else {
       last=sgf_root;
       if (last->child) {
	 for (; last->child; last = last->child)
	   ;
       }
     }
   }

   return last;
}


/*
 * Place text on the board at position (i,j).
 */

SGFNodeP
sgfBoardText(SGFNodeP pr,int i,int j, const char *text)
{
  void *str = malloc(strlen(text)+3);
  SGFNodeP ret;

  assert(sgf_root);
  ret=sgfNodeCheck(pr);
  if (!str)
    perror("sgfBoardText: Out of memory!\n");
  else {
    sprintf(str,"%c%c:%s",j+'a',i+'a',text);
    sgfAddProperty(ret,"LB",str);
    free(str);
  }

  return ret;
}


/*
 * Place a character on the board at position (i,j).
 */

SGFNodeP
sgfBoardChar(SGFNodeP pr,int i,int j, UCHAR_ALIAS c)
{
  char text[2]="";

  text[0]=c;
  text[1]=0;

  return sgfBoardText(pr,i,j,text);
}


/*
 * Place a number on the board at position (i,j).
 */

SGFNodeP 
sgfBoardNumber(SGFNodeP pr,int i,int j, int number)
{
  char text[10];
  SGFNodeP ret=sgfNodeCheck(pr);

  gg_snprintf(text,10,"%c%c:%i",j+'a',i+'a',number);
  sgfAddProperty(ret, "LB", text);

  return ret;
}


/*
 * Place a circle mark on the board at position (i,j).
 */

SGFNodeP 
sgfTriangle(SGFNodeP pr,int i,int j)
{
  SGFNodeP ret=sgfNodeCheck(pr);
  char text[3];

  gg_snprintf(text,3,"%c%c",j+'a',i+'a');
  sgfAddProperty(ret,"TR",text);

  return ret;
}


/*
 * Place a circle mark on the board at position (i,j).
 */

SGFNodeP 
sgfCircle(SGFNodeP pr,int i,int j)
{
  SGFNodeP ret=sgfNodeCheck(pr);
  char text[3];

  gg_snprintf(text,3,"%c%c",j+'a',i+'a');
  sgfAddProperty(ret, "CR", text);

  return ret;
}


/*
 * Place a square mark on the board at position (i,j).
 */

SGFNodeP
sgfSquare(SGFNodeP pr,int i,int j)
{
  return sgfMark(pr,i,j);   /*cgoban 1.9.5 does not understand SQ*/
}


/*
 * Place a (square) mark on the board at position (i,j).
 */

SGFNodeP
sgfMark(SGFNodeP pr,int i,int j)
{
  char text[3];
  SGFNodeP ret=sgfNodeCheck(pr);

  gg_snprintf(text,3,"%c%c",j+'a',i+'a');
  sgfAddProperty(ret,"MA",text);

  return ret;
}


/*
 * Start a new variant. Returns a pointer to the new node.
 */

SGFNodeP
sgfStartVariant(SGFNodeP pr)
{
  SGFNodeP last=sgfNodeCheck(pr);

  for (; last->next; last = last->next)
    ;
  last->next=sgfNewNode();
  last->next->parent=last;

  return last->next;
}


/*
 * Start a new variant as first child. Returns a pointer to the new node.
 */

SGFNodeP
sgfStartVariantFirst(SGFNodeP pr)
{
  SGFNodeP old_first_child=sgfNodeCheck(pr);
  SGFNodeP new_first_child=sgfNewNode();

  new_first_child->next=old_first_child;
  if (old_first_child->parent)
    new_first_child->parent=old_first_child->parent;
  old_first_child->parent=new_first_child;
  if (new_first_child->parent)
    new_first_child->parent->child=new_first_child;

  return new_first_child;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

