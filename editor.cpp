#include "kernel.h"
#include "util.h"
#include <unistd.h>
#include <fcntl.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_TAB_STOP 8

typedef struct erow
{
    int size;
    char *chars;

    /*
    Rendering tabs

    If you try opening the Makefile using ./kilo Makefile, you’ll notice that the tab character on the second line of the Makefile takes up a width of 8 columns or so. The length of a tab is up to the terminal being used and its settings. We want to know the length of each tab, and we also want control over how to render tabs, so we’re going to add a second string to the erow struct called render, which will contain the actual characters to draw on the screen for that row of text. We’ll only use render for tabs for now, but in the future it could be used to render nonprintable control characters as a ^ character followed by another character, such as ^A for the Ctrl-A character (this is a common way to display control characters in the terminal).
    */
    char *render;
    int rsize;
} erow;

struct abuf
{
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

struct editorConfig
{
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows = 25;
    int screencols = 80;
    int numrows;
    erow *row;
    char *filename;
    int dirty;
};

struct editorConfig E;

enum editorKey
{
    /*
    If you look at the ASCII table, you’ll see that ASCII code 8 is named BS for “backspace”, and ASCII code 127 is named DEL for “delete”. But for whatever reason, in modern computers the Backspace key is mapped to 127 and the Delete key is mapped to the escape sequence <esc>[3~
    */
    BACKSPACE = 127,
    ARROW_LEFT = 1,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

void initEditor()
{
    CKernel::Get()->setRaw(); // if enable,can't display right

    CKernel::Get()->clear();

    E.cx = 0;
    E.cy = 0;
    E.rx = 0; // let’s introduce a new horizontal coordinate variable, E.rx
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.filename = NULL;
    E.dirty = 0;
    E.screenrows -= 2;
}

ssize_t my_getline(char **lineptr, size_t *n, FILE *stream)
{
    if (lineptr == NULL || n == NULL || stream == NULL)
    {
        return -1; 
    }

    size_t capacity = *n; 
    size_t length = 0;   
    int c;

    if (capacity == 0)
    {
        capacity = 128; 
        *lineptr = (char *)malloc(capacity);
        if (*lineptr == NULL)
        {
            return -1; 
        }
    }

    while ((c = fgetc(stream)) != EOF)
    {
        if (c == '\n')
        {
            break;
        }

        if (length + 1 >= capacity)
        {
            capacity *= 2; 
            *lineptr = (char *)realloc(*lineptr, capacity);
            if (*lineptr == NULL)
            {
                return -1; 
            }
        }

        
        (*lineptr)[length++] = (char)c;
    }

    if (length == 0 && c == EOF)
    {
        return -1;
    }

    (*lineptr)[length] = '\0';

    *n = capacity;
    return length;
}

void abAppend(struct abuf *ab, const char *s, int len)
{
    char *neww = (char *)realloc(ab->b, ab->len + len);

    if (neww == NULL)
        return;
    memcpy(&neww[ab->len], s, len);
    ab->b = neww;
    ab->len += len;
}

void abFree(struct abuf *ab)
{
    free(ab->b);
}

void editorDrawRows(struct abuf *ab)
{
    int y;
    for (y = 0; y < E.screenrows; y++)
    {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows)
        {
            abAppend(ab, "~", 1);
        }
        else
        {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0)
                len = 0;
            if (len > E.screencols)
                len = E.screencols;
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }

        /*
        Clear lines one at a time

        Instead of clearing the entire screen before each refresh, it seems more optimal to clear each line as we redraw them. Let’s remove the <esc>[2J (clear entire screen) escape sequence, and instead put a <esc>[K sequence at the end of each line we draw.

        The K command (Erase In Line) erases part of the current line. Its argument is analogous to the J command’s argument: 2 erases the whole line, 1 erases the part of the line to the left of the cursor, and 0 erases the part of the line to the right of the cursor. 0 is the default argument, and that’s what we want, so we leave out the argument and just use <esc>[K.
        */
        abAppend(ab, "\x1b[K", 3); // Clear to end of line
        abAppend(ab, "\r\n", 2);
    }
}

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorRefreshScreen()
{
    editorScroll();

    struct abuf ab = ABUF_INIT;

    /*
    Hide the cursor when repainting

    There is another possible source of the annoying flicker effect we will take care of now. It’s possible that the cursor might be displayed in the middle of the screen somewhere for a split second while the terminal is drawing to the screen. To make sure that doesn’t happen, let’s hide the cursor before refreshing the screen, and show it again immediately after the refresh finishes.

    We use escape sequences to tell the terminal to hide and show the cursor. The h and l commands (Set Mode, Reset Mode) are used to turn on and turn off various terminal features or “modes”. The VT100 User Guide just linked to doesn’t document argument ?25 which we use above. It appears the cursor hiding/showing feature appeared in later VT models. So some terminals might not support hiding/showing the cursor, but if they don’t, then they will just ignore those escape sequences, which isn’t a big deal in this case.
    */
    abAppend(&ab, "\x1b[?25l", 6); // Cursor invisible

    /*
    Reposition the cursor

    You may notice that the <esc>[2J command left the cursor at the bottom of the screen. Let’s reposition it at the top-left corner so that we’re ready to draw the editor interface from top to bottom.

    This escape sequence is only 3 bytes long, and uses the H command (Cursor Position) to position the cursor. The H command actually takes two arguments: the row number and the column number at which to position the cursor. So if you have an 80×24 size terminal and you want the cursor in the center of the screen, you could use the command <esc>[12;40H. (Multiple arguments are separated by a ; character.) The default arguments for H both happen to be 1, so we can leave both arguments out and it will position the cursor at the first row and first column, as if we had sent the <esc>[1;1H command. (Rows and columns are numbered starting at 1, not 0.)
    */
    abAppend(&ab, "\x1b[H", 3); // Cursor home

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
             (E.rx - E.coloff) + 1);

    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6); // Normal cursor visible

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorUpdateRow(erow *row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t')
            tabs++;

    free(row->render);
    row->render = (char *)malloc(row->size + tabs * (KILO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++)
    {
        if (row->chars[j] == '\t')
        {
            row->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0)
                row->render[idx++] = ' ';
        }
        else
        {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
    // editorRefreshScreen();
}

void editorMoveCursor(int key)
{
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key)
    {
    case ARROW_LEFT:
        if (E.cx != 0)
        {
            E.cx--;
        }
        else if (E.cy > 0)
        {
            E.cy--;
            E.cx = E.row[E.cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (row && E.cx < row->size)
        {
            E.cx++;
        }
        else if (row && E.cx == row->size)
        {
            E.cy++;
            E.cx = 0;
        }
        break;
    case ARROW_UP:
        if (E.cy != 0)
        {
            E.cy--;
        }
        break;
    case ARROW_DOWN:
        if (E.cy < E.numrows)
        {
            E.cy++;
        }
        break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen)
    {
        E.cx = rowlen;
    }
}

void editorRowInsertChar(erow *row, int at, int c)
{
    // printf("row->size when enter editorRowInsertChar: %d\n",row->size);

    if (at < 0 || at > row->size)
        at = row->size;

    // Then we allocate one more byte for the chars of the erow (we add 2 because we also have to make room for the null byte)
    row->chars = (char *)realloc(row->chars, row->size + 2);

    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
    // editorRefreshScreen();
}

char *editorRowsToString(int *buflen)
{
    int totlen = 0;
    int j;

    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;

    *buflen = totlen;

    char *buf = (char *)malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++)
    {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void editorSave(char *fileName)
{
    int len;
    char *buf = editorRowsToString(&len);

    // FILE* fp = fopen(fileName, "w+");
    /*
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
      if (ftruncate(fd, len) != -1) {
        if (write(fd, buf, len) == len) {
          close(fd);
          free(buf);
          E.dirty = 0;

          return;
        }
      }
    */
    // fputs(buf, fp);
    int fd = open(fileName, O_RDWR | O_CREAT);//fcntl.h
    // ftruncate(fd, len);

    write(fd, buf, len);
    E.dirty = 0;
    close(fd);

    free(buf);
}

int editorReadKey()
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

int file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fclose(file);
        return 1;
    }
    return 0;
}

void editorRowDelChar(erow *row, int at)
{
    if (at < 0 || at >= row->size)
        return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len)
{
    row->chars = (char *)realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorFreeRow(erow *row)
{
    free(row->render);
    free(row->chars);
}

void editorDelRow(int at)
{
    if (at < 0 || at >= E.numrows)
        return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}

void editorDelChar()
{
    if (E.cy == E.numrows)
        return;
    if (E.cx == 0 && E.cy == 0)
        return;

    erow *row = &E.row[E.cy];
    if (E.cx > 0)
    {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    }
    else
    {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;

  E.row = (erow*)realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

  E.row[at].size = len;
  E.row[at].chars = (char*)malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  editorUpdateRow(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void editorInsertChar(int c)
{
    if (E.cy == E.numrows)
    {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorOpen(char *filename)
{

    free(E.filename);
    E.filename = my_strdup(filename);

    FILE *fp = fopen(filename, "r"); // read&write,if file not exist,will not create

    // if (!fp)
    // die("fopen");
    /*

    */
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = my_getline(&line, &linecap, fp)) != -1)
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

// This func should be the end of almost all editor's function,before edit()
char editorProcessKeypress()
{
    int c = editorReadKey();

    switch (c)
    {

    case '\n':
      editorInsertNewline();
      return 'x';

    case CTRL_KEY('q'):
        return 'q';

    case CTRL_KEY('s'):
        // editorSave();
        return 's';

    /*
    Backspace doesn’t have a human-readable backslash-escape representation in C (like \n, \r, and so on), so we make it part of the editorKey enum and assign it its ASCII value of 127.
    */
    case BACKSPACE:
    /*
    The Delete key

    Lastly, let’s detect when the Delete key is pressed. It simply sends the escape sequence <esc>[3~
    */
    //case DEL_KEY:
        //if (c == DEL_KEY)
            //editorMoveCursor(ARROW_RIGHT);
        editorDelChar();
        return 'x';

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        return 'x';
    default:
        editorInsertChar(c);
        return 'x';
    }
}