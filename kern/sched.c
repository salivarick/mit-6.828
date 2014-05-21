#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;
    uint32_t i;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
    cprintf("cpu id is %d \n", cpunum());
    if (!curenv || ((curenv + 1) >= envs+NENV))
        idle = envs;
    else
        idle = curenv + 1;

    for (i = 0; i <= NENV; 
            idle = envs + (idle-envs)%NENV, ++ i) {
        if (idle->env_status == ENV_RUNNABLE)
            break;
    }

    if (idle->env_status != ENV_FREE &&
            ((idle != curenv && idle->env_status == ENV_RUNNABLE) 
             || (idle == curenv && idle->env_status == ENV_RUNNING))) {
        if (idle == curenv)
            cprintf("original env\n");
        else {
            cprintf("not original env, env id is %d\n", idle->env_id);
        }
        cprintf("scheduler env run\n");
        env_run(idle);
    }
    // sched_halt never returns
    sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
    void
sched_halt(void)
{
    int i;

    cprintf("cpu id %d sched halt\n", cpunum());
    // For debugging and testing purposes, if there are no runnable
    // environments in the system, then drop into the kernel monitor.
    for (i = 0; i < NENV; i++) {
        if ((envs[i].env_status == ENV_RUNNABLE ||
                    envs[i].env_status == ENV_RUNNING))
            break;
    }
    cprintf("checkpoin1\n");
    if (i == NENV) {
        cprintf("No runnable environments in the system!\n");
        while (1)
            monitor(NULL);
    }
    cprintf("checkpoint2\n");

    // Mark that no environment is running on this CPU
    curenv = NULL;
    lcr3(PADDR(kern_pgdir));
    cprintf("checkpoint3\n");
    // Mark that this CPU is in the HALT state, so that when
    // timer interupts come in, we know we should re-acquire the
    // big kernel lock
    xchg(&thiscpu->cpu_status, CPU_HALTED);

    // Release the big kernel lock as if we were "leaving" the kernel
    unlock_kernel();
    cprintf("checkpoint5");
    // Reset stack pointer, enable interrupts and then halt.
    asm volatile (
            "movl $0, %%ebp\n"
            "movl %0, %%esp\n"
            "pushl $0\n"
            "pushl $0\n"
            "sti\n"
            "hlt\n"
            : : "a" (thiscpu->cpu_ts.ts_esp0));
}

