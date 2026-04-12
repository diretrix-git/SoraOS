#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256

void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_handler(void);

#endif
