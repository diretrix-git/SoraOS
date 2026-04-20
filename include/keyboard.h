#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Register IRQ1 handler and initialise the keyboard buffer */
void keyboard_init(void);

/* Return next character from the buffer, or 0 if empty */
char keyboard_getchar(void);

#endif /* KEYBOARD_H */
