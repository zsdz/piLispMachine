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



#ifndef _TTSGF_H
#define _TTSGF_H 1

#ifdef HAVE_LIBGLIB
#include <glib.h>
#endif
void gg_snprintf(char *dest, unsigned long len, const char *fmt, ...);

#ifndef HAVE___FUNCTION__
#define __FUNCTION__ ""
#endif

/* from the old utils.h */
#ifndef NULL
#define NULL 0
#endif

void *xalloc(unsigned int);

#define UCHAR_ALIAS char

/* from the old goban.h */
#define X(p) ((p) & 31)
#define Y(p) ((p) >> 5)
#define XY(x,y) (((y)<<5)|(x))


/* Two special moves: PASS and NOMOVE */
/*#define PASS   ((1<<10)-1)*/
#define NOMOVE ((1<<10)-2)

/* typedef unsigned short int SGFNodeP; */
typedef struct SGFNode     *SGFNodeP;
typedef struct SGFProperty *SGFPropertyP;

struct SGFProperty {
  SGFPropertyP next;
  short        name;        /*FIXME should be char with enumed flags*/
  char         value[1];    /*FIXME variable length, different types*/
};

    
struct SGFNode {
  SGFPropertyP prop;
  SGFNodeP     parent, child, next;
};

#define sgfUp(p)   (p)->parent
#define sgfNext(p) (p)->brother
#define sgfDown(p) (p)->son

/* low level functions */
SGFNodeP sgfPrev(SGFNodeP);
SGFNodeP sgfRoot(SGFNodeP);
SGFNodeP sgfNewNode(void);
int sgfGetIntProperty(SGFNodeP n, const UCHAR_ALIAS *, int *);
int sgfGetFloatProperty(SGFNodeP, const UCHAR_ALIAS *, float *);
int sgfGetCharProperty(SGFNodeP, const UCHAR_ALIAS *, char **);
void sgfAddProperty(   SGFNodeP, const char *, const char *);
void sgfAddPropertyInt(SGFNodeP, const char *, long);
void sgfAddPropertyFloat(SGFNodeP n, const char *name, float v);
void sgfOverwriteProperty(SGFNodeP n, const UCHAR_ALIAS *name, const char *text);
void sgfOverwritePropertyFloat(SGFNodeP n, const UCHAR_ALIAS *name, float v);
void sgfOverwritePropertyInt(SGFNodeP n, const UCHAR_ALIAS *name, int v);
SGFPropertyP sgfMkProperty( const UCHAR_ALIAS *, const UCHAR_ALIAS *,
			   SGFNodeP, SGFPropertyP);
SGFNodeP sgfAddPlay(SGFNodeP n, int isblack, int movex,int movey);
SGFNodeP sgfAddStone(SGFNodeP pr, int color, int movex,int movey);
int sgfPrintCharProperty(SGFNodeP n, const UCHAR_ALIAS *name);
int sgfPrintCommentProperty(SGFNodeP n, const UCHAR_ALIAS *name);
void sgfWriteResult(float score);
SGFNodeP sgfNodeCheck(SGFNodeP);
SGFNodeP sgfCircle(SGFNodeP,int i,int j);
SGFNodeP sgfSquare(SGFNodeP,int i,int j);
SGFNodeP sgfTriangle(SGFNodeP pr,int i,int j);
SGFNodeP sgfMark(SGFNodeP,int i,int j);
SGFNodeP sgfAddComment(SGFNodeP, const char *comment);
SGFNodeP sgfBoardText(SGFNodeP,int i,int j,const char *text);
SGFNodeP sgfBoardChar(SGFNodeP pr,int i,int j, UCHAR_ALIAS c);
SGFNodeP sgfBoardNumber(SGFNodeP,int i,int j, int number);
SGFNodeP sgfStartVariant(SGFNodeP);
SGFNodeP sgfStartVariantFirst(SGFNodeP pr);
int sgfAddMoveFromTree(SGFNodeP node, int next_to_move);
SGFNodeP sgfCreateHeaderNode(float komi);
void sgfSetLastNode(SGFNodeP lastnode);

extern SGFNodeP sgf_root;

#endif
