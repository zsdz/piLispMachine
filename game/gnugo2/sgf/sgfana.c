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

#include <ctype.h>
#include <string.h>

#include <assert.h>
#include "ttsgf.h"
#include "sgf_properties.h"
#include "sgfana.h"
#define BUILDING_GNUGO_ENGINE
#include "../engine/liberty.h"
#include "../interface/interface.h"


#define USAGE "\n\
Usage : gnugo [-opts]\n\
\n\
Use --analyze with --analyzerfile and --testmode, --score or --benchmark.\n\
See the Texinfo documentation (User Guide/Invoking GNU Go) for more\n\
information.\n\n\
Analyzer Options:\n\
       --analyzerfile <name>    filename for analyzer output\n\
       --analyze <options>      where <options> can be:\n\
              areacolor         show color of influence \n\
              capture           show capturing moves\n\
              considered        show considered moves\n\
              defense           show defending moves\n\
              dragoninfo        print info about all dragons\n\
              dragonstatus      show dragonstatus\n\
              eyeinfo           print info about all eyes\n\
              eyes              show eyes and their vital points\n\
              moyocolor         show color of moyos\n\
              moyovalue         show moyo values \n\
              overwrite         overwrites game info from the inputfile\n\
              recommended       show moves recommend by modules\n\
              terricolor        show color of territories (moyo)\n\
              territory         show territory (worms)\n\
              terrivalue        show territory values (moyo)\n\
              worminfo          print info about all worms\n\
              wormliberties     show liberties of the worms\n\
You may use \"option1 option2 ...\" or option1,option2,... \n\
to specify more than one option for --analyze\n\
"

/* same order as ANALYZE_.. in sgfana.h */
const char *analyzer_options[]={
  "overwrite",
  "dragoninfo",
  "dragonstatus",
  "considered",
  "wormliberties",
  "eyes",
  "worminfo",
  "eyeinfo",
  "gameinfo",
  "moyocolor",
  "recommended",
  "capture",
  "moyovalue",
  "defense",
  "terricolor",
  "terrivalue",
  "territory",
  "areacolor",
  "areavalue",
};


extern SGFNodeP lastnode;
int analyzerflag=0;


void
sgfNextString(char *s)
{
  int i;

  i = strlen(s)-1;
  s[i]++;
  while(i >= 0  && s[i] > 'Z') {
    s[i] = 'A';
    i--;
    if(i>=0) s[i]++;
  }

  if (i == -1) {
    i = strlen(s);
    if (i<3) {
      s[i] = 'A';
      s[i+1] = '\0';
    }
  }
}


void
sgfPrevString(char *s)
{
  int i;

  i = strlen(s)-1;
  s[i]--;
  while (i >= 0  && s[i] < 'a') {
    s[i] = 'z';
    i--;
    if (i>=0) s[i]--;
  }

  if (i == -1) {
    i = strlen(s);
    if (i<3) {
      s[i] = 'z';
      s[i+1] = '\0';
    }
  }
}


/*
 * Perform the moves and place the stones from the SGF Tree on the 
 * board.  Return whose turn it is to move (WHITE or BLACK).
 */

int
sgfAddMoveFromTree(SGFNodeP node, int next_to_move)
{
  int i,j;
  int next=next_to_move;
  SGFPropertyP prop;

  for (prop=node->prop; prop; prop=prop->next) {
    switch(prop->name) {
    case SGFAB:
      /* A black stone. */
      i=prop->value[1]-'a';
      j=prop->value[0]-'a';
      p[i][j]=BLACK;
      break;

    case SGFAW:
      /* A white stone. */
      i=prop->value[1]-'a';
      j=prop->value[0]-'a';
      p[i][j]=WHITE;
      break;

    case SGFPL:
      /* Player property - who is next to move? */
      if (prop->value[0] == 'w' || prop->value[0] == 'W')
	next = WHITE;
      else
	next = BLACK;
      break;

    case SGFW:
    case SGFB:
      /* An ordinary move. */
      next = prop->name == SGFW ? WHITE : BLACK;
      if ((!prop->value)||(prop->value[0]<'a')) {
	i=board_size;
	j=board_size;
      }
      else {
	i=prop->value[1]-'a';
	j=prop->value[0]-'a';
      }

      inc_movenumber();
      updateboard(i,j,next);
      next=OTHER_COLOR(next);
      break;
    }
  }

  return next;
}


/*
 * Play the moves in ROOT UNTIL movenumber is reached.
 */

int
sgfPlayTree(SGFNodeP *root, int *until)
{
  SGFNodeP node;
  int next=BLACK;
  SGFPropertyP prop;
  int i,j;
  int movenumber=0;

  /* Start from the empty board. */
  init_board();

  for (node=*root; node; node=node->child) {
    for (prop=node->prop; prop; prop=prop->next) {
      switch(prop->name) {
      case SGFAB:
	i = prop->value[1] - 'a';
	j = prop->value[0] - 'a';
	updateboard(i,j,BLACK);
	break;

      case SGFAW:
	i = prop->value[1] - 'a';
	j = prop->value[0] - 'a';
	updateboard(i,j,WHITE);
	break;

      case SGFPL:
	if (prop->value[0] == 'w' || prop->value[0] == 'W')
	  next = WHITE;
	else
	  next = BLACK;
	break;

      case SGFW:
      case SGFB:
	next = prop->name == SGFW ? WHITE : BLACK;
	if ((!prop->value)||(prop->value[0]<'a')) {
	  i=board_size;
	  j=board_size;
	}
	else {
	  i=prop->value[1]-'a';
	  j=prop->value[0]-'a';
	}
	movenumber++;
	if (movenumber == *until) {
	  *root=node->parent;
	  return next;
	}
	updateboard(i,j,next);
	next=OTHER_COLOR(next);
	break;
      }
    }
  }
  *until=movenumber;

  return next;
}


/*
 * Write result of the game to the game tree, when RE is not there or
 * when "--analyze overwrite" is given at the cmdline.
 */

void
sgfWriteResult(float score)
{
  char text[8];
  char winner;
  float s;
  int dummy;

  /* If not writing to the SGF file, skip everything and return now. */
  if (!sgf_root)
    return;

  /* If not overwriting and there already is a result property, return. */
  if (!(analyzerflag & ANALYZE_OVERWRITE))
    if (sgfGetIntProperty(sgf_root,"RE",&dummy))
      return;

  if (score>0) {
    winner='B';
    s=score;
  }
  else if (score<0) {
    winner='W';
    s=-score;
  }
  else {
    winner='0';
    s=0;
  }

  if (score < 1000 && score > -1000)
    gg_snprintf(text, 8, "%c+%3.1f", winner, s);
  else
    gg_snprintf(text, 8, "%c+%c", winner, 'R');
  sgfOverwriteProperty(sgf_root,"RE",text);
}



/*
 * Mark all parts of a worm with mark.
 */

void
sgfMarkWorm(SGFNodeP pr, int x, int y, const char *mark)
{
  int i,j;
  char text[3];
  SGFNodeP node=sgfNodeCheck(pr);

  for(i=0;i<board_size;i++)
    for(j=0;j<board_size;j++) {
      if ((worm[i][j].origini==x) && (worm[i][j].originj==y)) {
	gg_snprintf(text, 3, "%c%c", j+'a', i+'a');
	sgfAddProperty(node, mark, text);
      }
    }
}


/*
 * Place a territory mark for color on the board at position (i,j).
 */

SGFNodeP
sgfTerritory(SGFNodeP pr,int i,int j,int color)
{
  char text[3];
  SGFNodeP ret=sgfNodeCheck(pr);

  gg_snprintf(text, 3, "%c%c", j+'a', i+'a');
  if (color==BLACK)
    sgfAddProperty(ret, "TB", text);
  else
    sgfAddProperty(ret, "TW", text);

  return ret;
}


/*
 * Parse the options string and set the corresponding flags.
 */

void
parse_analyzer_options(char *optarg)
{
  char *str;
  unsigned int i;
  unsigned int n_options=sizeof(analyzer_options)/sizeof(analyzer_options[0]);

  str=strtok(optarg, " ,");
  do {
    i=0;
    if (!strcmp(str,"all")) {
      analyzerflag=-1;
      break;
    }

    while(i< n_options) {
      if (!strcmp( str,analyzer_options[i]))
	break;
      else
	i++;
    }

    if (i>=n_options) {
      fprintf(stderr,"gnugo: unknown option --analyze %s\n",str);
      exit(EXIT_FAILURE);
    }
    else
      analyzerflag|=1<<i;
  } while ((str=strtok(NULL, " ,")));
}


/* ================================================================ */
/*                          OUTPUT FUNCTIONS                        */
/* ================================================================ */


/*
 * Calls the different output functions according to analyzerflag.
 */

void
sgfAnalyzer(int value)
{
  if (!analyzerflag)
    return;
    
  if (analyzerflag & ANALYZE_CONSIDERED)
    sgfShowConsideredMoves();
  if (analyzerflag & ANALYZE_DRAGONSTATUS)
    sgfShowDragons(SGF_SHOW_STATUS);
  if (analyzerflag & ANALYZE_WORMS)
    sgfShowWorms(SGF_SHOW_LIBERTIES);
  if (analyzerflag & ANALYZE_EYES)
    sgfShowEyes(0);
  if (analyzerflag & ANALYZE_EYEINFO)
    sgfShowEyeInfo(0);
  if (analyzerflag & ANALYZE_DRAGONINFO)
    sgfShowDragonInfo(0);
  if (analyzerflag & ANALYZE_WORMINFO)
    sgfShowWormInfo(0);
  if (analyzerflag & ANALYZE_GAMEINFO)
    sgfPrintGameInfo(value);
  if (analyzerflag & ANALYZE_MOYOCOLOR)
    sgfShowMoyo(SGF_SHOW_COLOR, 1);
  if (analyzerflag & ANALYZE_MOYOVALUE)
    sgfShowMoyo(SGF_SHOW_VALUE, 1);
  if (analyzerflag & ANALYZE_TERRICOLOR)
    sgfShowTerri(SGF_SHOW_COLOR, 1);
  if (analyzerflag & ANALYZE_TERRIVALUE)
    sgfShowTerri(SGF_SHOW_VALUE, 1);
  if (analyzerflag & ANALYZE_TERRITORY)
    sgfShowTerritory(1);
  if (analyzerflag & ANALYZE_AREACOLOR)
    sgfShowArea(SGF_SHOW_COLOR,1);
  if (analyzerflag & ANALYZE_AREAVALUE)
    sgfShowArea(SGF_SHOW_VALUE,1);
  if (analyzerflag & ANALYZE_CAPTURING)
    sgfShowCapturingMoves();
  if (analyzerflag & ANALYZE_DEFENSE)
    sgfShowDefendingMoves();
}


void
sgfPrintGameInfo(int value)
{
  char text[128];
  SGFNodeP pr;

  gg_snprintf(text, 128, "Move value: %i", value);
  pr=sgfAddComment(0, text);
  gg_snprintf(text, 128, "W captured %i", black_captured);
  sgfAddComment(pr, text);
  gg_snprintf(text, 128, "B captured %i", white_captured);
  sgfAddComment(pr, text);
  gg_snprintf(text, 128, "W territory %i", moyo_eval[WHITE]);
  sgfAddComment(pr, text);
  gg_snprintf(text, 128, "B territory %i", moyo_eval[BLACK]);
  sgfAddComment(pr,text);
}


/*
 * Prints all considered moves and their values into the analyzer file.
 */

void
sgfShowConsideredMoves(void)
{
  int i,j;
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;

  assert(sgf_root);

  pr=sgfAddComment(startnode,"Considered Moves");
  for(i=0;i<board_size;i++)
    for(j=0;j<board_size;j++) {
      if (potential_moves[i][j]>0)
	sgfBoardNumber(pr,i,j, potential_moves[i][j]);
    }
}


/*
 * Shows dragons
 */

void
sgfShowDragons(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct dragon_data *d;
  int i,j;

  assert(sgf_root);

  pr=sgfAddComment(startnode,"Dragon Status");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      d=&(dragon[i][j]);
      if ((d->color==WHITE) || (d->color==BLACK)) {
	switch(mode) {
	case SGF_SHOW_STATUS:
	  switch(d->status) {
	  case ALIVE:    sgfBoardText(pr,i,j,"L"); break;
	  case CRITICAL: sgfBoardText(pr,i,j,"C"); break;
	  case DEAD:     sgfBoardText(pr,i,j,"D"); break;
	  case UNKNOWN:  sgfBoardText(pr,i,j,"U"); break;
	  default:
	    fprintf(stderr, "%s: illegal status %i at %i %i\n",
		    __FUNCTION__, d->color, i, j);
	    exit(-1);
	  }
	  break;
	default:
	  fprintf(stderr, "%s: illegal mode\n", __FUNCTION__);
	  exit(-1);
	}
      }
      else {
	switch(d->color) {
	case BLACK_BORDER: sgfTerritory(pr, i, j, BLACK); break;
	case WHITE_BORDER: sgfTerritory(pr, i, j, WHITE); break;
	case GRAY_BORDER:  ; break;
	case EMPTY:        ; break;
	default:
	  fprintf(stderr, "%s: illegal color %i at %i %i\n",
		  __FUNCTION__, d->color, i, j);
	  exit(-1);
	}
      }
    }
  }
}


void
sgfShowWorms(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct worm_data *w;
  int i,j;

  assert(sgf_root);

  pr=sgfAddComment(startnode,"Worm Liberties");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      w=&(worm[i][j]);
      if ((w->color==WHITE) || (w->color==BLACK)) {
	switch(mode) {
	case SGF_SHOW_LIBERTIES:
	  sgfBoardNumber(pr,i,j,w->liberties);
	  break;

	default:
	  fprintf(stderr, "%s: illegal mode\n", __FUNCTION__);
	  exit(-1);
	}
      }
      else {
	switch(w->color) {
	case BLACK_BORDER: sgfTerritory(pr,i,j,BLACK); break;
	case WHITE_BORDER: sgfTerritory(pr,i,j,WHITE); break;
	case GRAY_BORDER:  ; break;
	case EMPTY:        ; break;
	default:
	  fprintf(stderr, "%s: illegal color %i at %i %i\n",
		  __FUNCTION__, w->color, i, j);
	  exit(-1);
	}
      }
    }
  }
}


void
sgfShowEyes(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct eye_data *e;
  int i,j;

  i=mode; /*avoid warning */
  assert(sgf_root);

  pr=sgfAddComment(startnode,"Vital Points of Eyes");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      if (p[i][j]==EMPTY) {
	e=&(white_eye[i][j]);
	switch(e->color) {
	case WHITE_BORDER: sgfTerritory(pr,i,j,WHITE); break;
	case BLACK_BORDER: break;
	case GRAY_BORDER:
	case EMPTY:        break;

	default:
	  fprintf(stderr, "%s: illegal color %i at %i %i\n",
		  __FUNCTION__, e->color, i, j);
	  exit(-1);
	}

	if ((i==e->attacki)&&(j==e->attackj))
	  sgfMark(pr,i,j);
            
	e=&(black_eye[i][j]);
	switch(e->color) {
	case BLACK_BORDER: sgfTerritory(pr,i,j,BLACK); break;
	case WHITE_BORDER: break;
	case GRAY_BORDER:  break;
	case EMPTY:        break;
	default:
	  fprintf(stderr, "%s: illegal color %i at %i %i\n",
		  __FUNCTION__, e->color, i, j);
	  exit(-1);
	}
	if ((i==e->attacki)&&(j==e->attackj))
	  sgfMark(pr,i,j);
      }
    }
  }
}


/*
 * Shows territory information from worms.
 */

void
sgfShowTerritory(int as_variant)
{
  SGFNodeP startnode=0;
  SGFNodeP pr=startnode;
  struct worm_data *w;
  int i,j;

  assert(sgf_root);

  if (as_variant) {
    startnode=sgfStartVariant(0);
    pr=sgfAddComment(startnode,"Territory");
  }

  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      w=&(worm[i][j]);
      if (!((w->color==WHITE) || (w->color==BLACK))) {
	switch(w->color) {
	case BLACK_BORDER: sgfTerritory(pr,i,j,BLACK); break;
	case WHITE_BORDER: sgfTerritory(pr,i,j,WHITE); break;
	case GRAY_BORDER:  ; break;
	default:
	  fprintf(stderr, "%s: illegal color %i at %i %i\n", 
		  __FUNCTION__, w->color, i, j);
	}
      }
    }
  }
}


/*
 * Prints the info of the eye at i,j with the name c.
 */

void
sgfPrintSingleEyeInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c)
{
  char text[128];
  struct eye_data *e;

  if (white_eye[i][j].color==WHITE_BORDER)
    e=&(white_eye[i][j]);
  else if (black_eye[i][j].color==BLACK_BORDER)
    e=&(black_eye[i][j]);
  else
    return;

  gg_snprintf(text, 128, "%s esize=%i msize=%i mineye=%i maxeye=%i", c,
	   e->esize, e->msize, e->mineye, e->maxeye);
  sgfAddComment(pr, text);
}


void
sgfShowEyeInfo(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct eye_data *e;
  int i,j;
  UCHAR_ALIAS next_black[4], next_white[4];
  UCHAR_ALIAS names[MAX_BOARD][MAX_BOARD][4];

  /* count down from 'z' to minimise confusion. 
     Avoid % operator on -ve quantities */
  strncpy(next_white, "z", 4);
  strncpy(next_black, "A", 4); /* avoid 0, which means unsigned */
  for(i=0;i<board_size;i++)
    for(j=0;j<board_size;j++)
      names[i][j][0]='\0';

  if (mode<0)
    return;

  assert(sgf_root);
  pr=sgfAddComment(startnode,"Eye Info\n");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      e=&(black_eye[i][j]);
      switch(e->color) {
      case BLACK_BORDER:
	if (!names[e->origini][e->originj][0]) {
	  strcpy(names[e->origini][e->originj],next_black);
	  sgfNextString(next_black);
	  sgfPrintSingleEyeInfo(pr, e->origini, e->originj,
				names[e->origini][e->originj]);
	}
	sgfBoardText(pr, i, j, names[e->origini][e->originj]);
	break;
      case WHITE_BORDER:
      case GRAY_BORDER:
      case BLACK:
      case WHITE:
      case EMPTY:
	break;

      default:
	fprintf(stderr, "%s: illegal color %i at %i %i\n", 
		__FUNCTION__, e->color, i, j);
	exit(-1);
      }

      e=&(white_eye[i][j]);
      switch(e->color) {
      case WHITE_BORDER:
	if (!names[e->origini][e->originj][0]) {
	  strcpy(names[e->origini][e->originj],next_white);
	  sgfPrevString(next_white);
	  sgfPrintSingleEyeInfo(pr, e->origini, e->originj,
				names[e->origini][e->originj]);
	}
              
	sgfBoardText(startnode, i, j, names[e->origini][e->originj]);
	break;
      case GRAY_BORDER:
      case BLACK:
      case WHITE:
      case EMPTY:
      case BLACK_BORDER:
	break;
      default:
	fprintf(stderr, "%s: illegal color %i at %i %i\n", 
		__FUNCTION__, e->color, i, j);
	exit(-1);
      }
    }
  }
}


/*
 * Prints the info of the dragon at (i,j) with the name c.
 */

void
sgfPrintSingleDragonInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c)
{
  char text[128];
  const char *status;

  switch (dragon[i][j].status) {
  case ALIVE:    status="alive"; break;
  case DEAD:     status="dead";  break;
  case CRITICAL: status="crititcal"; break;
  case UNKNOWN:  status="unknown";break;
  default:
    fprintf(stderr, "%s: illegal status %i at %i %i\n",
	    __FUNCTION__, dragon[i][j].status, i, j);
    exit(-1);
  }
  gg_snprintf(text, 128, "%s %s size=%i genus=%i", c, status,
	   dragon[i][j].size, dragon[i][j].genus);
  sgfAddComment(pr,text);
}


void
sgfShowDragonInfo(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct dragon_data *d=&dragon[0][0];
  int i,j=0;
  UCHAR_ALIAS next_black[4], next_white[4];
  UCHAR_ALIAS names[MAX_BOARD][MAX_BOARD][4];

 /* count down from 'z' to minimise confusion. 
    Avoid % operator on -ve quantities */
  strncpy(next_white,"z",4);
  strncpy(next_black,"A",4); /* avoid 0, which means unsigned */
  for(i=0;i<board_size;i++)
    for(j=0;j<board_size;j++)
      names[i][j][0]='\0';

  if (mode<0)
    return;

  assert(sgf_root);
  pr=sgfAddComment(startnode,"Dragon Info\n");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      d=&(dragon[i][j]);
      switch(d->color) {
      case BLACK:
	if (!names[d->origini][d->originj][0]) {
	  strcpy(names[d->origini][d->originj], next_black);
	  sgfNextString(next_black);
	  sgfPrintSingleDragonInfo(pr, d->origini, d->originj, 
				   names[d->origini][d->originj]);
	}
	sgfBoardText(pr, i, j, names[d->origini][d->originj]);
	break;
      case WHITE:
	if (!names[d->origini][d->originj][0]) {
	  strcpy(names[d->origini][d->originj], next_white);
	  sgfPrevString(next_white);
	  sgfPrintSingleDragonInfo(pr, d->origini, d->originj,
				   names[d->origini][d->originj]);
	}
	sgfBoardText(startnode,i,j,names[d->origini][d->originj]);
	break;

      case BLACK_BORDER: sgfTerritory(startnode,i,j,BLACK); break;
      case WHITE_BORDER: sgfTerritory(startnode,i,j,WHITE); break;
      case GRAY_BORDER:  ; break;
      case EMPTY:          break;
      default:
	fprintf(stderr, "%s: illegal color %i at %i %i\n",
		__FUNCTION__, d->color, i, j);
	exit(-1);
      }
    }
  }
}


/*
 * Prints the info of the worm at (i,j) with the name c.
 */

void
sgfPrintSingleWormInfo(SGFNodeP pr,int i, int j, UCHAR_ALIAS *c)
{
  char text[128];

  gg_snprintf(text, 128, "%s %s size=%i value=%i", c,
	   worm[i][j].inessential?"inessential":"essential",
	   worm[i][j].size, worm[i][j].value);
  sgfAddComment(pr, text);
}


void
sgfShowWormInfo(int mode)
{
  SGFNodeP startnode=sgfStartVariant(0);
  SGFNodeP pr=startnode;
  struct worm_data *w=&worm[0][0];
  int i,j=0;
  UCHAR_ALIAS next_black[4], next_white[4];
  UCHAR_ALIAS names[MAX_BOARD][MAX_BOARD][4];

  /* count down from 'z' to minimise confusion. 
    Avoid % operator on -ve quantities */
  strncpy(next_white, "z", 4);
  strncpy(next_black, "A", 4); /* avoid 0, which means unsigned */
  for(i=0;i<board_size;i++)
    for(j=0;j<board_size;j++)
      names[i][j][0]='\0';

  if (mode<0)
    return;

  assert(sgf_root);
  pr=sgfAddComment(startnode,"Worm Info\n");
  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      w=&(worm[i][j]);
      switch(w->color) {
      case BLACK:
	if (!names[w->origini][w->originj][0]) {
	  strcpy(names[w->origini][w->originj],next_black);
	  sgfNextString(next_black);
	  sgfPrintSingleWormInfo(pr, w->origini, w->originj, 
				 names[w->origini][w->originj]);
	}
	sgfBoardText(pr, i, j, names[w->origini][w->originj]);
	break;

      case WHITE:
	if (!names[w->origini][w->originj][0]) {
	  strcpy(names[w->origini][w->originj],next_white);
	  sgfPrevString(next_white);
	  sgfPrintSingleWormInfo(pr, w->origini, w->originj,
				 names[w->origini][w->originj]);
	}
              
	sgfBoardText(startnode, i, j, names[w->origini][w->originj]);
	break;

      case BLACK_BORDER: sgfTerritory(startnode,i,j,BLACK); break;
      case WHITE_BORDER: sgfTerritory(startnode,i,j,WHITE); break;
      case GRAY_BORDER:  ; break;
      case EMPTY:        break;
      default:
	fprintf(stderr, "%s: illegal color %i at %i %i\n",
		__FUNCTION__, w->color, i, j);
	exit(-1);
      }
    }
  }
}


/*
 * Shows the capturing move for all capturable worms.
 */

void
sgfShowCapturingMoves(void)
{
  int *worms=xalloc(MAX_BOARD*MAX_BOARD*sizeof(int));
  int wp=0;
  int i,j,k;
  int wormname;

  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      if ((p[i][j]!=EMPTY) && (worm[i][j].liberties>1)) {
	wormname= (worm[i][j].origini<<8) + worm[i][j].originj;
	k=0;
	while((k<wp)&&(wormname!=worms[k])) k++;
	if (k==wp) {
	  worms[wp]=wormname;
	  wp++;
	  /*     gprintf("%s %m %d\n",__FUNCTION__,i,j,wormname);*/
	  sgfCaptureString(i,j);
	}
      }
    }
  }
}


/*
 * Shows how to capture the string at (i,j).
*/

void
sgfCaptureString(int i, int j)
{
  SGFNodeP startnode;
  char move[3];
  int ki,kj;

  if (!attack(i, j, &ki, &kj)) 
    return;

  startnode=sgfStartVariant(lastnode->child);
  sprintf(move, "%c%c", kj+'a', ki+'a');
  sgfAddProperty(startnode, p[i][j]==WHITE ? "B":"W", move);
  sgfMarkWorm(startnode, i, j, "MA");
  sgfAddComment(startnode, "Capturing move");
}


/*
 * Shows the defending move for all capturable worms.
 */

void
sgfShowDefendingMoves(void)
{
  int *worms=xalloc(MAX_BOARD*MAX_BOARD*sizeof(int));
  int wp=0;
  int i,j,k;
  int wormname;

  for(i=0;i<board_size;i++) {
    for(j=0;j<board_size;j++) {
      if ((p[i][j]!=EMPTY) && (worm[i][j].liberties>1)) {
	wormname= (worm[i][j].origini<<8) + worm[i][j].originj;
	k=0;
	while ((k<wp) && (wormname!=worms[k]))
	  k++;
	if (k==wp) {
	  worms[wp]=wormname;
	  wp++;
	  /*     gprintf("%s %m %d\n",__FUNCTION__,i,j,wormname);*/
	  sgfDefendString(i, j);
	}
      }
    }
  }
}


/*
 * Shows how to capture the string at (i,j).
 */

void
sgfDefendString(int i, int j)
{
  SGFNodeP startnode;
  char move[3];
  int ki,kj;

  if (!attack(i, j, &ki, &kj))
    return;
  if (!find_defense(i, j, &ki, &kj))
    return;

  startnode=sgfStartVariant(lastnode->child);
  sprintf(move,"%c%c",kj+'a',ki+'a');
  sgfAddProperty(startnode, p[i][j]==WHITE ? "W":"B", move);
  sgfMarkWorm(startnode,i,j,"MA");
  sgfAddComment(startnode,"Defending move");
}


void
sgfHelp()
{
  fprintf(stderr, USAGE);
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */

