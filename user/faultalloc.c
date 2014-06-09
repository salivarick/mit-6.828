// test user-level fault handler -- alloc pages to fix faults

#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
	int r;
	void *addr = (void*)utf->utf_fault_va;

	cprintf("fault %x\n", addr);
	if ((r = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE),
				PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating at %x in page fault handler: %e", addr, r);
    cprintf("check\n");
    // function snprintf() does not prints to screen, instead, it prints
    // to a buffer. Here, snprintf prints string to a buffer which addr
    // point, then the cprintf can print to the screen.
	snprintf((char*) addr, 100, "this string was faulted in at %x", addr);
    cprintf("check done\n");
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
    // print length is 100(0x64) in handler function 
    // snprintf, so, for addr 0xDeadBeef, 0xDeadBeef + 0x64 in one page
    // but, addr 0xCafeBffe + 0x64 in two pages.
	cprintf("%s\n", (char*)0xDeadBeef);
	cprintf("%s\n", (char*)0xCafeBffe);
}
