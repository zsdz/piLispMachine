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
#include "config.h"
#endif




#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "ttsgf_read.h"

#define MAX_FILE_BUFFER 200000 /* buffer for reading SGF file (tt code) */

/*
 * SGF grammar:
 *
 * Collection = GameTree { GameTree }
 * GameTree   = "(" Sequence { GameTree } ")"
 * Sequence   = Node { Node }
 * Node       = ";" { Property }
 * Property   = PropIdent PropValue { PropValue }
 * PropIdent  = UcLetter { UcLetter }
 * PropValue  = "[" CValueType "]"
 * CValueType = (ValueType | Compose)
 * ValueType  = (None | Number | Real | Double | Color | SimpleText |
 *               Text | Point  | Move | Stone)
 *
 * The above grammar has a number of simple properties which enables us
 * to write a simpler parser:
 *   1) There is never a need for backtracking
 *   2) The only recursion is on gametree.
 *   3) Tokens are only one character
 * 
 * We will use a global state to keep track of the remaining input
 * and a global char variable, `lookahead' to hold the next token.  
 * The function `nexttoken' skips whitespace and fills lookahead with 
 * the new token.
 */


#define STRICT 's'
#define LAX    'l'


static void parse_error(const char *msg, int arg);
static void nexttoken(void);
static void match(int expected);


static FILE *sgffile;


# define sgf_getch() (getc(sgffile))


static char *sgferr;
#ifdef TEST_SGFPARSER
static int sgferrarg;
#endif
static int sgferrpos;

static int lookahead;


/* ================================================================ */
/*                       Parsing primitives                         */
/* ================================================================ */


static void
parse_error(const char *msg, int arg)
{
  fprintf(stderr, msg, arg);
  exit(EXIT_FAILURE);
}


static void
nexttoken()
{
  do
    lookahead = sgf_getch();
  while (isspace(lookahead));
}


static void
match(int expected)
{
  if (lookahead != expected)
    parse_error("expected: %c", expected);
  else
    nexttoken();
}

/* ================================================================ */
/*                        The parser proper                         */
/* ================================================================ */


static void
propident(char *buffer, int size)
{
  if (lookahead == -1 || !isupper(lookahead)) 
    parse_error("Expected an upper case letter", 0);
  while (lookahead != -1 && isalpha(lookahead)) {
    if (isupper(lookahead) && size > 1) {
      *buffer++ = lookahead;
      size--;
    }
    nexttoken();
  }
  *buffer = '\0';
}


static void
propvalue(char *buffer, int size)
{
  char *p = buffer;

  match('[');
  while (lookahead != ']' && lookahead != '\0') {
    if (lookahead == '\\') {
      lookahead = sgf_getch();
      /* Follow the FF4 definition of backslash */
      if (lookahead == '\r') {
	lookahead = sgf_getch();
	if (lookahead == '\n') 
	  lookahead = sgf_getch();
      } else if (lookahead == '\n') {
	lookahead = sgf_getch();
	if (lookahead == '\r') 
	  lookahead = sgf_getch();
      }
    }
    if (size > 1) {
      *p++ = lookahead;
      size--;
    }
    lookahead = sgf_getch();
  }
  match(']');
  /* Remove trailing whitespace */
  --p;
  while (p > buffer && isspace((int) *p))
    --p;
  *++p = '\0';
}


static SGFPropertyP
property(SGFNodeP n, SGFPropertyP last)
{
  char name[3];
  char buffer[4000];

  propident(name, sizeof(name));
  do {
    propvalue(buffer, sizeof(buffer));
    last = sgfMkProperty(name, buffer, n, last);
  } while (lookahead == '[');
  return last;
}


static void
node(SGFNodeP n)
{
  SGFPropertyP last = NULL;
  match(';');
  while (lookahead != -1 && isupper(lookahead))
    last = property(n, last);
}


static SGFNodeP
sequence(SGFNodeP n)
{
  node(n);
  while (lookahead == ';') {
    SGFNodeP new = sgfNewNode();
    new->parent = n;
    n->child = new; n = new;
    node(n);
  }
  return n;
}


static void
gametree(SGFNodeP *p, SGFNodeP parent, int mode) 
{
  if (mode == STRICT)
    match('(');
  else
    for (;;) {
      if (lookahead == -1) {
	parse_error("Empty file?", 0);
	break;
      }
      if (lookahead == '(') {
	while (lookahead == '(')
	  nexttoken();
	if (lookahead == ';')
	  break;
      }
      nexttoken();
    }

  /* The head is parsed */
  {
    SGFNodeP head = sgfNewNode();
    SGFNodeP last;

    head->parent = parent;
    *p = head;

    last = sequence(head);
    p = &last->child;
    while (lookahead == '(') {
      gametree(p, last->parent, STRICT);
      p = &((*p)->next);
    }
    if (mode == STRICT)
      match(')');
  }
}


/*
 * Wrapper around readsgf which reads from a file rather than a string.
 * Returns NULL if file will not open, or some other parsing error.
 */

SGFNodeP
readsgffile(const char *filename)
{
  SGFNodeP root;
  int tmpi = 0;

  if (strcmp(filename, "-") == 0)
    sgffile = stdin;
  else
    sgffile = fopen(filename, "r");

  if (!sgffile)
    return NULL;


  nexttoken();
  gametree(&root, NULL, LAX);

  fclose(sgffile);

  if (sgferr) {
    fprintf(stderr, "Parse error: %s at position %d\n", sgferr, sgferrpos);
    return NULL;
  }

  /* perform some simple checks on the file */
  if (!sgfGetIntProperty(root, "GM", &tmpi)) {
    fprintf(stderr, "Couldn't find the game type (GM) attribute!\n");
  }
  if (tmpi != 1) {
    fprintf(stderr, "SGF file might be for game other than go: %d\n", tmpi);
    fprintf(stderr, "Trying to load anyway.\n");
  }

  if (!sgfGetIntProperty(root, "FF", &tmpi)) {
    fprintf(stderr, "Can not determine SGF spec version (FF)!\n");
  }
  if (tmpi<3 || tmpi>4) {
    fprintf(stderr, "Unsupported SGF spec version: %d\n", tmpi);
  }

  return root;
}





#ifdef TEST_SGFPARSER
int
main()
{
  static char buffer[25000];
  static char output[25000];
  SGFNodeP game;

  sgffile = stdin;

  nexttoken();
  gametree(&game, LAX);
  if (sgferr) {
    fprintf(stderr, "Parse error:");
    fprintf(stderr, sgferr, sgferrarg);
    fprintf(stderr, " at position %d\n", sgferrpos);
  } else {
    unparse_game(game);
    write(1,output,outputp-output);
  }
}
#endif



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
