#ifndef KERNEL_H
#define KERNEL_H

/* Display message in white-on-red, disable interrupts, halt forever */
void kernel_panic(const char* message);

#endif /* KERNEL_H */
