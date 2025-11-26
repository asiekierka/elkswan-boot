#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <wonderful.h>

void console_init(void);
void cputs(char __far* buf);
void cprintf(const char __far* format, ...);
__attribute__((noreturn))
void panic(const char __far* format, ...);

#endif /* CONSOLE_H_ */