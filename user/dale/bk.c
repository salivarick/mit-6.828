// program to cause a breakpoint trap

#include <inc/lib.h>

# define    DALE    1

void
umain(int argc, char **argv)
{
# ifdef DALE
    int a = 1, b = 2;
    int c = a + b;
# endif /* Dale */

	asm volatile("int $3");

    cprintf("%d + %d = %d\n", a, b, c);
}

