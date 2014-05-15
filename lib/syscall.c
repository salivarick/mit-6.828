// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

#if FAST_SYS_CALL
static int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
#else
static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
#endif
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.
#if FAST_SYS_CALL
    // i copy this code from google code
    // save the current context
    // ax bx cx dx : ax used for return value, save bx cx dx
    // si di       : si used for passing return eip, save si di
    // bp          : bp used for passing return esp, save bp
    // ip cs sp ss : restore by sysexit
    // ds es       : save and restore in trapentry.S

    asm volatile(
            "pushl %%ebx\n\t"
            "pushl %%ecx\n\t"
            "pushl %%edx\n\t"
            "pushl %%edi\n\t"
            "pushfl \n\t"
            "pushl %%esi\n\t"
            "pushl %%ebp\n\t"
            "movl %%esp, %%ebp\n\t"
            "leal after_sysenter_label, %%esi\n\t"
            "sysenter\n\t"
            "after_sysenter_label:\n\t"
            "popl %%ebp\n\t"
            "popl %%esi\n\t"
            "popfl \n\t"
            "popl %%edi\n\t"
            "popl %%edx\n\t"
            "popl %%ecx\n\t"
            "popl %%ebx"
            : "=a" (ret)
            : "a" (num), "d" (a1), "c" (a2), "b" (a3), "D" (a4)
            : "cc", "esi", "esp", "memory");
#else
    asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");
#endif // !FAST_SYS_CALL
	
    if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

