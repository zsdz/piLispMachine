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
#include <time.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "ttsgf_write.h"
#include "ttsgf.h"
#include "../interface/interface.h"
#include "sgfana.h"


#define OPTION_STRICT_FF4 0

#define PACKAGE "gnugo"
/*
 * p->child is the next move.
 * p->next  is the next variation
 */
#define VERSION "2.6"

extern int analyzerflag;
FILE *ttsgfout=NULL;


void
sgf_putch(char c)
{
  fputc(c,ttsgfout);
}


void
sgf_puts(const char *s)
{
  fputs(s,ttsgfout);
}


void
sgf_putint(int i)
{
  char buf[10], *p=buf;
  
  if (i < 0)
    sgf_putch('-'), i = -i;
  do {
    *p++ = i % 10;
    i /= 10;
  } while (i > 9);
  while (p > buf)
    sgf_putch( (char) ( *--p + '0') );
       /* MS-C complains even of this conversion, if not cast... */
}


#if 0
static void
unparse_prop(SGFPropertyP p)
{
  char name[3], *cp;

  name[2] = 0;
  while (p != NULL) {
    name[0] = p->name & 255;
    name[1] = p->name >> 8;
    if(name[1] == ' ') name[1] = 0;
    sgf_puts(name);
    sgf_putch('[');
    for (cp = p->value; *cp; cp++) {
      if (*cp == '[' || *cp == ']' || *cp == '\\')
	sgf_putch('\\');
      sgf_putch(*cp);
    }
    sgf_putch(']');
    p = p->next;
  }
}
#endif


/*
 * Print all unprinted property values at node N to file.
 */

void
sgfPrintAllProperties(SGFNodeP n)
{
  SGFPropertyP p;
  SGFPropertyP l;
  
  p = n->prop;
  if (!p)
    return;

  do {
    if (!(p->name&0x20)) {
      l=p;
          
      fprintf(ttsgfout, "%c%c[%s]", p->name&0xff, (p->name>>8) & 0xff, 	      p->value);
      p=p->next;
      while(p) {
	if (l->name==p->name) {
	  fprintf(ttsgfout,"[%s]",p->value);
	  p->name|=0x20;  /*indicate already printed*/
	}
	p=p->next;
      };
      p=l;
      p->name|=0x20;  /*indicate already printed*/
    }
    p=p->next;
  } while(p);
}


/*
 * Print the property values of NAME at node N and mark it as printed. 
 */

int
sgfPrintCharProperty(SGFNodeP n, const UCHAR_ALIAS *name)
{
  int first=1;
  SGFPropertyP p;
  short nam = name[0] | name[1] << 8;

  for (p = n->prop; p; p=p->next) {
    if (p->name == nam) {
      p->name|=0x20;  /*indicate already printed*/
      if (first) {
	/*    if(name[1]==' ')
	      name[1]='';
	      */    fprintf(ttsgfout,"%s",name);
	      first=0;
      }
      fprintf(ttsgfout,"[%s]",p->value);
    }
  }

  return !first;
}


/*
 * Print comments form Node N.
 *
 * NOTE: cgoban does not print "C[comment1][comment2]" and I don't know
 *       what the sgfspec says.
 */

int
sgfPrintCommentProperty(SGFNodeP n, const UCHAR_ALIAS *name)
{
  int first=1;
  SGFPropertyP p;
  short nam = name[0] | name[1] << 8;

  for (p = n->prop; p; p=p->next) {
    if (p->name == nam) {
      p->name|=0x20;  /*indicate already printed*/
      if (first) {
	/*   if(name[1]==' ')
	     name[1]='';
	     */
	fprintf(ttsgfout,"%s[%s",name,p->value);
	first=0;
      } else
	  fprintf(ttsgfout,"\n%s",p->value);
    }
  }

  if (!first) {
    fprintf(ttsgfout,"]");
    return 1;
  } else
    return 0;
}


void
unparse_node(SGFNodeP n)
{
  fputs("\n;",ttsgfout);
  sgfPrintCharProperty(n,"B ");
  sgfPrintCharProperty(n,"W ");
  sgfPrintCommentProperty(n,"N ");
  sgfPrintCommentProperty(n,"C ");
  if(sgfPrintCommentProperty(n,"PB")|sgfPrintCommentProperty(n,"BR"))
    fputs("\n",ttsgfout);
  if(sgfPrintCommentProperty(n,"PW")|sgfPrintCommentProperty(n,"WR"))
    fputs("\n",ttsgfout);
  sgfPrintAllProperties(n);
}


void
unparse_root(SGFNodeP n)
{
  fputs("\n;",ttsgfout);
  sgfPrintCharProperty(n,"FF");
  fputs("\n",ttsgfout);
  if(sgfPrintCharProperty(n,"GM"))
      fputs("\n",ttsgfout);
  else
      fputs("GM[1]\n",ttsgfout);

  if(sgfPrintCharProperty(n,"SZ"))
      fputs("\n",ttsgfout);
  if(sgfPrintCharProperty(n,"GN"))
      fputs("\n",ttsgfout);
  if(sgfPrintCharProperty(n,"DT"))
      fputs("\n",ttsgfout);
  if(sgfPrintCommentProperty(n,"PB")|sgfPrintCommentProperty(n,"BR"))
      fputs("\n",ttsgfout);
  if(sgfPrintCommentProperty(n,"PW")|sgfPrintCommentProperty(n,"WR"))
      fputs("\n",ttsgfout);
  sgfPrintCharProperty(n,"B ");
  sgfPrintCharProperty(n,"W ");
  sgfPrintCommentProperty(n,"N ");
  sgfPrintCommentProperty(n,"C ");
  sgfPrintAllProperties(n);
}


void
unparse_game(SGFNodeP n)
{
  sgf_putch('(');
  unparse_root(n);

  n = n->child;
  while (n != NULL && n->next == NULL) {
    unparse_node(n);
    n = n->child;
  } 

  while (n != NULL) {
    unparse_game(n);
    n = n->next;
  }
  sgf_puts("\n)");
}


/*
 * Opens filename and writes the game stored in the sgf structure.
 */

int
writesgf(SGFNodeP root, const char *filename,int seed)
{
  time_t curtime=time(NULL);
  struct tm *loctime=localtime(&curtime);
  char str[128];
  int dummy;

  if (strcmp(filename, "-")==0) 
    ttsgfout=stdout;
  else
    ttsgfout = fopen(filename, "w");

  if (!ttsgfout) {
    fprintf(stderr,"Can not open %s\n",filename);
    return 0;
  }

  gg_snprintf(str, 128, "GNU Go %s Random Seed %d", VERSION, seed);
  if ((analyzerflag&ANALYZE_OVERWRITE)
      || (!sgfGetIntProperty(sgf_root,"GN",&dummy)))
    sgfOverwriteProperty(root,"GN",str);
  gg_snprintf(str, 128, "%4.4i-%2.2i-%2.2i",
	   loctime->tm_year+1900, loctime->tm_mon+1, loctime->tm_mday);
  if ((analyzerflag&ANALYZE_OVERWRITE)
      || (!sgfGetIntProperty(sgf_root,"DT",&dummy)))
    sgfOverwriteProperty(root,"DT",str);
  if ((analyzerflag&ANALYZE_OVERWRITE)
      || (!sgfGetIntProperty(sgf_root,"AP",&dummy)))
    sgfOverwriteProperty(root,"AP",PACKAGE":"VERSION);
  if ((analyzerflag&ANALYZE_OVERWRITE)
      || (!sgfGetIntProperty(sgf_root,"RU",&dummy)))
    sgfOverwriteProperty(root,"RU","Japanese");
  sgfOverwriteProperty(root,"FF","4");
  sgfOverwritePropertyFloat(root,"KM",get_komi());

  unparse_game(root);
  fclose(ttsgfout);
  return 1;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
