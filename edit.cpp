#include "kernel.h"

#include <unistd.h>

struct object *edit(struct object *args) {

    CKernel::Get()->setRaw();//if enable,can't display right

    CKernel::Get()->clear();

    char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');

    return NULL;
}