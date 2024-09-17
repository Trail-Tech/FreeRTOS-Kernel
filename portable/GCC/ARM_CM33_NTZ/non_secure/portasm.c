#include <stdint.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE ensures that PRIVILEGED_FUNCTION
 * is defined correctly and privileged functions are placed in correct sections. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Portasm includes. */
#include "portasm.h"
#include "task.h"

/* System call numbers includes. */
#include "mpu_syscall_numbers.h"

/* MPU_WRAPPERS_INCLUDED_FROM_API_FILE is needed to be defined only for the
 * header files. */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

void vRestoreContextOfFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION */
{
    extern TaskHandle_t* pxCurrentTCB;

    if (pxCurrentTCB == NULL)
    {
        while (1){}
    }

    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr  r2, =pxCurrentTCB                          \n" /* Read the location of pxCurrentTCB i.e. &( pxCurrentTCB ). */
        "   ldr  r1, [r2]                                   \n" /* Read pxCurrentTCB. */
        "   ldr  r0, [r1]                                   \n" /* Read top of stack from TCB - The first item in pxCurrentTCB is the task top of stack. */
        "                                                   \n"
        "   ldm  r0!, {r1-r2}                               \n" /* Read from stack - r1 = PSPLIM and r2 = EXC_RETURN. */
        "   msr  psplim, r1                                 \n" /* Set this task's PSPLIM value. */
        "   movs r1, #2                                     \n" /* r1 = 2. */
        "   msr  CONTROL, r1                                \n" /* Switch to use PSP in the thread mode. */
        "   adds r0, #32                                    \n" /* Discard everything up to r0. */
        "   msr  psp, r0                                    \n" /* This is now the new top of stack to use in the task. */
        "   isb                                             \n"
        "   mov  r0, #0                                     \n"
        "   msr  basepri, r0                                \n" /* Ensure that interrupts are enabled when the first task starts. */
        "   bx   r2                                         \n" /* Finally, branch to EXC_RETURN. */
    );
}

BaseType_t xIsPrivileged( void ) /* __attribute__ (( naked )) */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 = CONTROL. */
        "   tst r0, #1                                      \n" /* Perform r0 & 1 (bitwise AND) and update the conditions flag. */
        "   ite ne                                          \n"
        "   movne r0, #0                                    \n" /* CONTROL[0]!=0. Return false to indicate that the processor is not privileged. */
        "   moveq r0, #1                                    \n" /* CONTROL[0]==0. Return true to indicate that the processor is privileged. */
        "   bx lr                                           \n" /* Return. */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vRaisePrivilege( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs  r0, control                                \n" /* Read the CONTROL register. */
        "   bic r0, #1                                      \n" /* Clear the bit 0. */
        "   msr  control, r0                                \n" /* Write back the new CONTROL value. */
        "   bx lr                                           \n" /* Return to the caller. */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vResetPrivilege( void ) /* __attribute__ (( naked )) */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, control                                 \n" /* r0 = CONTROL. */
        "   orr r0, #1                                      \n" /* r0 = r0 | 1. */
        "   msr control, r0                                 \n" /* CONTROL = r0. */
        "   bx lr                                           \n" /* Return to the caller. */
        ::: "r0", "memory"
    );
}
/*-----------------------------------------------------------*/

void vStartFirstTask( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   ldr r0, =0xe000ed08                             \n" /* Use the NVIC offset register to locate the stack. */
        "   ldr r0, [r0]                                    \n" /* Read the VTOR register which gives the address of vector table. */
        "   ldr r0, [r0]                                    \n" /* The first entry in vector table is stack pointer. */
        "   msr msp, r0                                     \n" /* Set the MSP back to the start of the stack. */
        "   cpsie i                                         \n" /* Globally enable interrupts. */
        "   cpsie f                                         \n"
        "   dsb                                             \n"
        "   isb                                             \n"
        "   svc %0                                          \n" /* System call to start the first task. */
        "   nop                                             \n"
        ::"i" ( portSVC_START_SCHEDULER ) : "memory"
    );
}
/*-----------------------------------------------------------*/

uint32_t ulSetInterruptMask( void ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, basepri                                 \n" /* r0 = basepri. Return original basepri value. */
        "   mov r1, %0                                      \n" /* r1 = configMAX_SYSCALL_INTERRUPT_PRIORITY. */
        "   msr basepri, r1                                 \n" /* Disable interrupts upto configMAX_SYSCALL_INTERRUPT_PRIORITY. */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   bx lr                                           \n" /* Return. */
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
    );
}
/*-----------------------------------------------------------*/

void vClearInterruptMask( __attribute__( ( unused ) ) uint32_t ulMask ) /* __attribute__(( naked )) PRIVILEGED_FUNCTION */
{
    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   msr basepri, r0                                 \n" /* basepri = ulMask. */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   bx lr                                           \n" /* Return. */
        ::: "memory"
    );
}

void PendSV_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION */
{
    extern TaskHandle_t* pxCurrentTCB;

    if (pxCurrentTCB == NULL)
    {
        while (1){}
    }

    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   mrs r0, psp                                     \n" /* Read PSP in r0. */
        "                                                   \n"
        "                                                   \n"
        "   mrs r2, psplim                                  \n" /* r2 = PSPLIM. */
        "   mov r3, lr                                      \n" /* r3 = LR/EXC_RETURN. */
        "   stmdb r0!, {r2-r11}                             \n" /* Store on the stack - PSPLIM, LR and registers that are not automatically saved. */
        "                                                   \n"
        "   ldr r2, =pxCurrentTCB                           \n" /* Read the location of pxCurrentTCB i.e. &( pxCurrentTCB ). */
        "   ldr r1, [r2]                                    \n" /* Read pxCurrentTCB. */
        "   str r0, [r1]                                    \n" /* Save the new top of stack in TCB. */
        "                                                   \n"
        "   mov r0, %0                                      \n" /* r0 = configMAX_SYSCALL_INTERRUPT_PRIORITY */
        "   msr basepri, r0                                 \n" /* Disable interrupts upto configMAX_SYSCALL_INTERRUPT_PRIORITY. */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   bl vTaskSwitchContext                           \n"
        "   mov r0, #0                                      \n" /* r0 = 0. */
        "   msr basepri, r0                                 \n" /* Enable interrupts. */
        "                                                   \n"
        "   ldr r2, =pxCurrentTCB                           \n" /* Read the location of pxCurrentTCB i.e. &( pxCurrentTCB ). */
        "   ldr r1, [r2]                                    \n" /* Read pxCurrentTCB. */
        "   ldr r0, [r1]                                    \n" /* The first item in pxCurrentTCB is the task top of stack. r0 now points to the top of stack. */
        "                                                   \n"
        "   ldmia r0!, {r2-r11}                             \n" /* Read from stack - r2 = PSPLIM, r3 = LR and r4-r11 restored. */
        "                                                   \n"
        "                                                   \n"
        "   msr psplim, r2                                  \n" /* Restore the PSPLIM register value for the task. */
        "   msr psp, r0                                     \n" /* Remember the new top of stack for the task. */
        "   bx r3                                           \n"
        ::"i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
    );
}

void SVC_Handler( void ) /* __attribute__ (( naked )) PRIVILEGED_FUNCTION */
{
    extern TaskHandle_t* pxCurrentTCB;

    if (pxCurrentTCB == NULL)
    {
        while (1){}
    }

    __asm volatile
    (
        "   .syntax unified                                 \n"
        "                                                   \n"
        "   tst lr, #4                                      \n"
        "   ite eq                                          \n"
        "   mrseq r0, msp                                   \n"
        "   mrsne r0, psp                                   \n"
        "   ldr r1, =vPortSVCHandler_C                      \n"
        "   bx r1                                           \n"
    );
}

