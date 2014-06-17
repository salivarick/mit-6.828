// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
    int i;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    if (!((err & FEC_WR) &&  
                 (uvpt[((uint32_t) addr)>>PGSHIFT] & PTE_COW)))
        panic("user level pagefault error");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
    if ((r = sys_page_alloc(0, UTEMP, PTE_P | PTE_U | PTE_W) < 0))
        panic("sys_page_alloc: %e", r);
    
    for (i = 0; i < PGSIZE; ++ i)
        ((char *) UTEMP)[i] = ((char *) ROUNDDOWN(addr, PGSIZE))[i];
    
    if ((r = sys_page_map(0, UTEMP, 
                    0, ROUNDDOWN(addr, PGSIZE), 
                    PTE_P | PTE_U | PTE_W)) < 0)
        panic("sys_page_map: %e", r);
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
    int perm = PTE_P | PTE_U;
	// LAB 4: Your code here.
	// panic("duppage not implemented");
    
    if (uvpd[pn >> 10] & PTE_P) {
        if (((uvpt[pn] & (perm | PTE_COW)) == (perm | PTE_COW)) ||
            ((uvpt[pn] & (perm | PTE_W)) == (perm | PTE_W))) {
            // cprintf("1pn = %x\n", pn);
            if ((r = sys_page_map(0, (void *) (pn*PGSIZE), 
                                  envid, (void *) (pn*PGSIZE),
                                  perm | PTE_COW)) < 0)
                return r;
            if ((r = sys_page_map(0, (void *) (pn*PGSIZE),
                                  0, (void *) (pn*PGSIZE),
                                  perm | PTE_COW)) < 0)
                return r;
        }
        else if ((uvpt[pn] & perm) == perm) {
            // cprintf("2pn = %x\n", pn); 
            if ((r = sys_page_map(0, (void *) (pn*PGSIZE), 
                                  envid, (void *) (pn*PGSIZE),
                                  perm)) < 0)
                return r;
        }
    }

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
    uint32_t pn;
    int r;
    envid_t envid;
    int perm;

    set_pgfault_handler(pgfault);
    if ((envid = sys_exofork()) < 0)
        return envid;
    if (envid == 0) {
        // child
        thisenv = &envs[ENVX(sys_getenvid())];
        return envid;    
    }

    // set pgfault handler for child
    if ((r = sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE),
                    PTE_P | PTE_U | PTE_W)) < 0)
        panic("parent alloc exception stack for child error: %e", r);
    sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);

    for (pn = 0; pn < USTACKTOP>>PGSHIFT; ++ pn)
        duppage(envid, pn);

    if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
        panic("sys_env_set_status: %e", r);

    return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
