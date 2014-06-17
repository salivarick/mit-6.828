#ifndef JOS_KERN_SYSCALL_H
#define JOS_KERN_SYSCALL_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/syscall.h>

#define IA32_SYSENTER_CS_ADDR   0x174
#define IA32_SYSENTER_ESP_ADDR  0x175
#define IA32_SYSENTER_EIP_ADDR  0x176

int32_t syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

#endif /* !JOS_KERN_SYSCALL_H */
