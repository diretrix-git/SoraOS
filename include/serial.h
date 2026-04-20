#ifndef SERIAL_H
#define SERIAL_H

/* Initialise COM1 at 9600 baud, 8N1 */
void serial_init(void);

/* Write a single character to COM1 */
void serial_putchar(char c);

/* Write a null-terminated string to COM1 */
void serial_print(const char* s);

#endif /* SERIAL_H */
