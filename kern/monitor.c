// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Back trace the kernel stack", mon_backtrace }, 
    { "showmappings", "Show virtual address mappings", mon_show_mappings }, 
    { "memchmod", "Change memory mapping's permission", mon_mem_chmod }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace (int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    
    struct Eipdebuginfo info;
    uint32_t eip, *ebp = (uint32_t *)read_ebp();
    int i;

    cprintf("Stack backtrace:\n");

    while (ebp) {
        eip = *(ebp + 1);
        cprintf("  ebp %08x eip %08x", ebp, eip);
        cprintf("  args ");

        if (debuginfo_eip(eip, &info) == 0) {
            for (i = 0; i < info.eip_fn_narg; ++ i) {
                // Attention: you should not use the *(esp+2+i)
                cprintf("%08x ", *((uint32_t *)(*ebp) + 2 + i));
            }
            cprintf("\n");

            // Debug info
            cprintf("\t%s:%u:  %.*s+%u\n", info.eip_file,
                                        info.eip_line,
                                        info.eip_fn_namelen, info.eip_fn_name, 
                                        eip - info.eip_fn_addr);
        }
        ebp = (uint32_t *) (*ebp);
    }

	return 0;
}

char
tolower (char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 'a' - 'A';
    else
        return c;
}

int
isxdigit (char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (tolower(c) >= 'a' && tolower(c) <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

int
axtoi (char *s)
{
    int i, t;
    int result = 0;
    
    for (i = 0; s[i] && s[i] != '0'; ++ i);

    if (!s[i])
        return -1;

    if (tolower(s[i+1]) == 'x') {
        i += 2;
        while (s[i]) {
            if ((t = isxdigit(s[i])) != -1) {
                result <<= 4;
                result += t;
            }
            else
                break;
            ++ i;
        }
        return result;
    }
    else
        return -1;
}

void 
print_mappings (uintptr_t vaddr, physaddr_t paddr, int perm)
{
    
    if (perm & PTE_P) {
        cprintf("0x%08x         0x%08x          ", vaddr, paddr);
        
        if (perm & PTE_W)
            cprintf("RW");
        else
            cprintf("R-");
        
        cprintf("/");
        if (perm & PTE_U) {
            if (perm & PTE_W)
                cprintf("RW");
            else
                cprintf("R-");
        }
        else
            cprintf("--");
    }
    else
        cprintf("0x%08x                             --/--", vaddr);

    cprintf("\n");
}

int
mon_show_mappings (int argc, char **argv, struct Trapframe *tf)
{
    uint32_t i, low_addr, high_addr;
    pde_t *pgdir = (pde_t *) KADDR(rcr3());
    pte_t *pte;
    
    if (argc > 3 || argc <= 1)
        return 0;

    if (argc == 3) {
        low_addr = ROUNDUP(axtoi(argv[1]), PGSIZE);
        high_addr = ROUNDDOWN(axtoi(argv[2]), PGSIZE);
    }
    else {
        low_addr = ROUNDUP(axtoi(argv[1]), PGSIZE);
        high_addr = ROUNDDOWN(axtoi(argv[1]), PGSIZE);
    }

    if (low_addr > high_addr) {
        i = low_addr;
        low_addr = high_addr;
        high_addr = i;
    }

    cprintf("virtual address    physical address    permissions\n");
    for (i = low_addr; i <= high_addr; i += PGSIZE) {
        if ((pte = pgdir_walk(pgdir, (void *) i, 0)) != NULL)
            print_mappings(i, PTE_ADDR(*pte), *pte);
        else
            print_mappings(i, 0, 0); 
    }

    return 0;
}

int
atoperm (char *s)
{
    if (s[2] != '/')
        return 0;

    if (tolower(s[1]) == 'w')
        return PTE_W | PTE_U;
    else {
        if (tolower(s[4]) == 'w')
            return PTE_W;
    }

    return 0;
}

int
mon_mem_chmod (int argc, char **argv, struct Trapframe *tf)
{
    uintptr_t vaddr;
    int perm;
    pde_t *pgdir = (pde_t *) KADDR(rcr3());
    pte_t *pte;
    
    if (argc != 3)
        return 0;

    vaddr = ROUNDDOWN(axtoi(argv[1]), PGSIZE);
    perm = atoperm(argv[2]);

    if ((pte = pgdir_walk(pgdir, (void *) vaddr, 0)) != NULL) {
        if (perm)
            *pte = PTE_ADDR(*pte) | perm | PTE_P;
        else
            page_remove(pgdir, (void *) vaddr);
    }

    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
