#include "kernel.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

struct abuf {
  char *b;
  int len;
};

typedef struct erow {
  int size;
  char *chars;
} erow;

struct editorConfig
{
    int cx, cy;
    int numrows;
    erow row;
    const int screenrows = 25;
    const int screencols = 80;
};

struct editorConfig E;

enum editorKey
{
    ARROW_LEFT = 1,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

void abAppend(struct abuf *ab, const char *s, int len) {
  char *neww = (char*)realloc(ab->b, ab->len + len);
  if (neww == NULL) return;
  memcpy(&neww[ab->len], s, len);
  ab->b = neww;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

void initEditor()
{
    CKernel::Get()->setRaw(); // if enable,can't display right

    CKernel::Get()->clear();

    E.cx = 0;
    E.cy = 0;
    E.numrows = 0;
}

void editorDrawRows(struct abuf *ab) {
    abAppend(ab, E.row.chars,E.row.size);
}

void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;
    editorDrawRows(&ab);
    write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

ssize_t my_getline(char **lineptr, size_t *n, FILE *stream) {
    if (lineptr == NULL || n == NULL || stream == NULL) {
        return -1;  
    }
 
    size_t capacity = *n;  
    size_t length = 0;     
    int c;
 
    if (capacity == 0) {
        capacity = 128;  
        *lineptr = (char *)malloc(capacity);
        if (*lineptr == NULL) {
            return -1;  
        }
    }
 
    while ((c = fgetc(stream)) != EOF) {
        if (c == '\n') {
            break;
        }
 
        if (length + 1 >= capacity) {
            capacity *= 2;  
            *lineptr = (char *)realloc(*lineptr, capacity);
            if (*lineptr == NULL) {
                return -1;  
            }
        }
 
        (*lineptr)[length++] = (char)c;
    }
 
    if (length == 0 && c == EOF) {
        return -1;
    }
 
    (*lineptr)[length] = '\0';
 
    *n = capacity;
    return length;
}

void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  //if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  linelen = my_getline(&line, &linecap, fp);
  if (linelen != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    E.row.size = linelen;
    E.row.chars = (char*)malloc(linelen + 1);
    memcpy(E.row.chars, line, linelen);
    E.row.chars[linelen] = '\0';
    E.numrows = 1;
  }
  free(line);
  fclose(fp);
}

char editorReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        // if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b')
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            switch (seq[1])
            {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
            }
        }

        return '\x1b';
    }
    else
    {
        return c;
    }
}

void editorMoveCursor(char key)
{
    switch (key)
    {
    case ARROW_LEFT:
        E.cx--;
        break;
    case ARROW_RIGHT:
        E.cx++;
        break;
    case ARROW_UP:
        E.cy--;
        break;
    case ARROW_DOWN:
        E.cy++;
        break;
    }
}

char editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {

    case CTRL_KEY('q'):
        //CKernel::Get()->clear();
        return 'q';

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        CKernel::Get()->printPosition(E.cx);
        CKernel::Get()->printPosition(E.cy);
        return 'a';
        // break;
    // case 'q':
    // return 'q';
    //  break;
    default:
        return 'x';
    }
}

void editorProcessKeypressVoid()
{
    char c = editorReadKey();

    switch (c)
    {

        /*
        case CTRL_KEY('q'):
            CKernel::Get()->clear();
            return 'q';
        */

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        CKernel::Get()->printPosition(E.cx);
        CKernel::Get()->printPosition(E.cy);
        // return 'a';
        break;
    case 'q':
        // return 'q';
        break;
    }
}

struct object *edit(struct object *args)
{
    initEditor();
    editorOpen((car(args))->string);
    editorRefreshScreen();

    while (1)
    {
        // editorRefreshScreen();

        if (editorProcessKeypress() == 'q')
        {
            break;
        }
    }

    CKernel::Get()->clear();//if not clear,the history command's result will not appear too

    CKernel::Get()->restoreMode();

    return NULL;
}

