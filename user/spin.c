// Test preemption by forking off a child process that just spins forever.
// Let it run for a couple time slices, then kill it.

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t env;

	cprintf("I am the parent.  Forking the child...\n");
	if ((env = fork()) == 0) {
		cprintf("I am the child.  Spinning...\n");
		while (1)
			/* do nothing */;
	}

	cprintf("I am the parent.  Running the child...\n");
    cprintf("yield 0\n");
	sys_yield();
    cprintf("yield 1\n");
	sys_yield();
    cprintf("yield 2\n");
	sys_yield();
    cprintf("yield 3\n");
	sys_yield();
    cprintf("yield 4\n");
	sys_yield();
    cprintf("yield 5\n");
	sys_yield();
    cprintf("yield 6\n");
	sys_yield();
    cprintf("yield 7\n");
	sys_yield();

	cprintf("I am the parent.  Killing the child...\n");
	sys_env_destroy(env);
}

