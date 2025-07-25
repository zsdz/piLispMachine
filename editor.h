void initEditor();

int file_exists(const char *filename);

void editorOpen(char *filename);

void editorRefreshScreen();

char editorProcessKeypress();

void editorSave(char *fileName);