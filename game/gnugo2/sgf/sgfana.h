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





#ifndef _SGFANA_H
#define _SGFANA_H 1

/* from the old utils.h */
#ifndef NULL
#define NULL 0
#endif


/* low level functions */
void sgfWriteResult(float score);
int sgfAddMoveFromTree(SGFNodeP node, int next_to_move);
int sgfPlayTree(SGFNodeP *root,int *until);
SGFNodeP sgfTerritory(SGFNodeP pr,int i,int j,int color);
void sgfHelp(void);
void sgfSetLastNode(SGFNodeP lastnode);
void sgfNextString(char *s);
void sgfPrevString(char *s);

/* high level functions - should be moved to a separate file someday*/
void sgfMarkWorm(SGFNodeP pr, int x, int y, const char *mark);
void ttsgf_move_made(int i, int j, int who, int value);
void sgfShowDragons(int mode);
void sgfShowConsideredMoves(void);
void sgfShowWorms(int mode);
void sgfShowTerritory(int as_variant);
void sgfShowTerri(int mode, int as_variant);
void sgfShowMoyo(int mode, int as_variant);
void sgfShowArea(int mode, int as_variant);
void sgfShowEyes(int mode);
void sgfShowEyeInfo(int mode);
void sgfPrintSingleEyeInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c);
void parse_analyzer_options(char *optionstring);
void sgfAnalyzer(int value);
void sgfShowWormInfo(int mode);
void sgfShowDragonInfo(int mode);
void sgfPrintSingleDragonInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c);
void sgfPrintSingleWormInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c);
void sgfShowEyeInfo(int mode);
void sgfPrintGameInfo(int value);
void sgfCaptureString(int i, int j);
void sgfShowCapturingMoves(void);
void sgfDefendString(int i, int j);
void sgfShowDefendingMoves(void);



enum{SGF_SHOW_STATUS,  /*show status of worms or dragons*/
     SGF_SHOW_LIBERTIES,
     SGF_SHOW_COLOR,
     SGF_SHOW_VALUE,
};

extern SGFNodeP sgf_root;
extern int analyzerflag;

#define    ANALYZE_OVERWRITE    1  /* overwrite already existing sgf info*/
#define    ANALYZE_DRAGONINFO   2
#define    ANALYZE_DRAGONSTATUS 4
#define    ANALYZE_CONSIDERED   8   /* show considered moves */
#define    ANALYZE_WORMS        16
#define    ANALYZE_EYES         32
#define    ANALYZE_WORMINFO     64
#define    ANALYZE_EYEINFO      128
#define    ANALYZE_GAMEINFO     0x100
#define    ANALYZE_MOYOCOLOR    0x200
#define    ANALYZE_RECOMMENDED  0x400
#define    ANALYZE_CAPTURING    0x800
#define    ANALYZE_MOYOVALUE    0x1000
#define    ANALYZE_DEFENSE      0x2000
#define    ANALYZE_TERRICOLOR   0x4000
#define    ANALYZE_TERRIVALUE   0x8000
#define    ANALYZE_TERRITORY    0x10000
#define    ANALYZE_AREACOLOR    0x20000
#define    ANALYZE_AREAVALUE    0x40000

#endif
