/*
 * Copyright (c) 2013-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -----------------------------------------------------------------------------
 *
 * Project:     CMSIS-RTOS RTX
 * Title:       Cortex-A Core definitions
 *
 * -----------------------------------------------------------------------------
 */

#ifndef RTX_CORE_CA_H_
#define RTX_CORE_CA_H_

#ifndef RTX_CORE_C_H_
#ifndef RTE_COMPONENTS_H
#include "RTE_Components.h"
#endif
#include CMSIS_device_header
#endif
#include "irq_ctrl.h"
#include "os_tick.h"

#include <stdbool.h>
typedef bool bool_t;

#ifndef FALSE
#define FALSE                   ((bool_t)0)
#endif

#ifndef TRUE
#define TRUE                    ((bool_t)1)
#endif

#if defined(RTX_STACK_CHECK)
#warning "Stack overrun checking is not supported!"
#endif

#define EXCLUSIVE_ACCESS        1

#define OS_TICK_HANDLER         osRtxTick_Handler

/// xPSR_Initialization Value
/// \param[in]  privileged      true=privileged, false=unprivileged
/// \param[in]  thumb           true=Thumb, false=Arm
/// \return                     xPSR Init Value
__STATIC_INLINE uint32_t xPSR_InitVal (bool_t privileged, bool_t thumb) {
  uint32_t psr;

  if (privileged) {
    if (thumb) {
      psr = CPSR_M_SYS | CPSR_T_Msk;
    } else {
      psr = CPSR_M_SYS;
    }
  } else {
    if (thumb) {
      psr = CPSR_M_USR | CPSR_T_Msk;
    } else {
      psr = CPSR_M_USR;
    }
  }
  
  return psr;
}

// Stack Frame:
//  - VFP-D32: D16-31, D0-D15, FPSCR, Reserved, R4-R11, R0-R3, R12, LR, PC, CPSR
//  - VFP-D16:         D0-D15, FPSCR, Reserved, R4-R11, R0-R3, R12, LR, PC, CPSR
//  - Basic:                                    R4-R11, R0-R3, R12, LR, PC, CPSR

/// Stack Frame Initialization Value
#define STACK_FRAME_INIT_VAL    0x00U

/// Stack Offset of Register R0
/// \param[in]  stack_frame     Stack Frame
/// \return                     R0 Offset
__STATIC_INLINE uint32_t StackOffsetR0 (uint8_t stack_frame) {
  uint32_t offset;

  if        ((stack_frame & 0x04U) != 0U) {
    offset = (32U*8U) + (2U*4U) + (8U*4U);
  } else if ((stack_frame & 0x02U) != 0U) {
    offset = (16U*8U) + (2U*4U) + (8U*4U);
  } else {
    offset =                      (8U*4U);
  }
  return offset;
}


//  ==== Emulated Cortex-M functions ====

/// Get xPSR Register - emulate M profile: SP_usr - (8*4)
/// \return      xPSR Register value
#ifdef __ICCARM__
__arm
#else
__attribute__((target("arm")))
#endif
__STATIC_INLINE uint32_t __get_PSP (void) {
  register uint32_t ret;

  __ASM volatile (
    "sub  sp,sp,#4\n\t"
    "stm  sp,{sp}^\n\t"
    "pop  {%[ret]}\n\t"
    "sub  %[ret],%[ret],#32\n\t"
    : [ret] "=&l" (ret)
    :
    : "memory"
  );

  return ret;
}


extern uint8_t SVC_Active;      // SVC Handler Active
extern uint8_t IRQ_PendSV;      // Pending SVC flag


//  ==== Core functions ====

/// Check if running Privileged
/// \return     true=privileged, false=unprivileged
__STATIC_INLINE bool_t IsPrivileged (void) {
  return (__get_mode() != CPSR_M_USR);
}

/// Set thread Privileged mode (not needed on Cortex-A)
/// \param[in]  privileged      true=privileged, false=unprivileged
__STATIC_INLINE void SetPrivileged (bool_t privileged) {
  (void)privileged;
}

/// Check if in Exception
/// \return     true=exception, false=thread
__STATIC_INLINE bool_t IsException (void) {
  return ((__get_mode() != CPSR_M_USR) && (__get_mode() != CPSR_M_SYS));
}

/// Check if in Fault
/// \return     true, false
__STATIC_INLINE bool_t IsFault (void) {
  return ((__get_mode() == CPSR_M_ABT) || (__get_mode() == CPSR_M_UND));
}

/// Check if in SVCall IRQ
/// \return     true, false
__STATIC_INLINE bool_t IsSVCallIrq (void) {
  return (SVC_Active != 0U);
}

/// Check if in PendSV IRQ
/// \return     true, false
__STATIC_INLINE bool_t IsPendSvIrq (void) {
  return ((__get_mode() == CPSR_M_SVC) && (SVC_Active == 0U) && (IRQ_PendSV != 0U));
}

/// Check if in Tick Timer IRQ
/// \return     true, false
__STATIC_INLINE bool_t IsTickIrq (int32_t tick_irqn) {
  return ((__get_mode() == CPSR_M_SVC) && (IRQ_GetActiveIRQ() == OS_Tick_GetIRQn()));
}

/// Check if IRQ is Masked
/// \return     true=masked, false=not masked
__STATIC_INLINE bool_t IsIrqMasked (void) {
  return  FALSE;
}


//  ==== Core Peripherals functions ====

/// Setup SVC and PendSV System Service Calls (not needed on Cortex-A)
__STATIC_INLINE void SVC_Setup (void) {
}

/// Get Pending SV (Service Call) Flag
/// \return     Pending SV Flag
__STATIC_INLINE uint8_t GetPendSV (void) {
  return (IRQ_PendSV);
}

/// Clear Pending SV (Service Call) Flag
__STATIC_INLINE void ClrPendSV (void) {
  IRQ_PendSV = 0U;
}

/// Set Pending SV (Service Call) Flag
__STATIC_INLINE void SetPendSV (void) {
  IRQ_PendSV = 1U;
}


//  ==== Service Calls definitions ====

//lint -save -e9023 -e9024 -e9026 "Function-like macros using '#/##'" [MISRA Note 10]

#if defined(RTX_SVC_PTR_CHECK)
#warning "SVC Function Pointer checking is not supported!"
#endif

#if defined(__ICCARM__)

#define SVC_ArgF(f)                                                            \
  __asm(                                                                       \
    "mov r12,%0\n"                                                             \
    :: "r"(&f): "r12"                                                          \
  );

#define STRINGIFY(a) #a
#define SVC_INDIRECT(n) _Pragma(STRINGIFY(svc_number = n)) __svc

#define SVC0_0N(f,t)                                                           \
SVC_INDIRECT(0) t    svc##f ();                                                \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (void) {                                           \
  SVC_ArgF(svcRtx##f);                                                         \
  svc##f();                                                                    \
}

#define SVC0_0(f,t)                                                            \
SVC_INDIRECT(0) t    svc##f ();                                                \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (void) {                                           \
  SVC_ArgF(svcRtx##f);                                                         \
  return svc##f();                                                             \
}

#define SVC0_1N(f,t,t1)                                                        \
SVC_INDIRECT(0) t    svc##f (t1 a1);                                           \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (t1 a1) {                                          \
  SVC_ArgF(svcRtx##f);                                                         \
  svc##f(a1);                                                                  \
}

#define SVC0_1(f,t,t1)                                                         \
SVC_INDIRECT(0) t    svc##f (t1 a1);                                           \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (t1 a1) {                                          \
  SVC_ArgF(svcRtx##f);                                                         \
  return svc##f(a1);                                                           \
}

#define SVC0_2(f,t,t1,t2)                                                      \
SVC_INDIRECT(0) t    svc##f (t1 a1, t2 a2);                                    \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (t1 a1, t2 a2) {                                   \
  SVC_ArgF(svcRtx##f);                                                         \
  return svc##f(a1,a2);                                                        \
}

#define SVC0_3(f,t,t1,t2,t3)                                                   \
SVC_INDIRECT(0) t    svc##f (t1 a1, t2 a2, t3 a3);                             \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (t1 a1, t2 a2, t3 a3) {                            \
  SVC_ArgF(svcRtx##f);                                                         \
  return svc##f(a1,a2,a3);                                                     \
}

#define SVC0_4(f,t,t1,t2,t3,t4)                                                \
SVC_INDIRECT(0) t    svc##f (t1 a1, t2 a2, t3 a3, t4 a4);                      \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t  __svc##f (t1 a1, t2 a2, t3 a3, t4 a4) {                     \
  SVC_ArgF(svcRtx##f);                                                         \
  return svc##f(a1,a2,a3,a4);                                                  \
}

#else   // !defined(__ICCARM__)

#define SVC_RegF "r12"

#define SVC_ArgN(n) \
register uint32_t __r##n __ASM("r"#n)

#define SVC_ArgR(n,a) \
register uint32_t __r##n __ASM("r"#n) = (uint32_t)a

#if    (defined(RTX_SVC_PTR_CHECK) && !defined(_lint))
#define SVC_ArgF(f) \
register uint32_t __rf   __ASM(SVC_RegF) = (uint32_t)jmpRtx##f
#else
#define SVC_ArgF(f) \
register uint32_t __rf   __ASM(SVC_RegF) = (uint32_t)svcRtx##f
#endif

#define SVC_In0 "r"(__rf)
#define SVC_In1 "r"(__rf),"r"(__r0)
#define SVC_In2 "r"(__rf),"r"(__r0),"r"(__r1)
#define SVC_In3 "r"(__rf),"r"(__r0),"r"(__r1),"r"(__r2)
#define SVC_In4 "r"(__rf),"r"(__r0),"r"(__r1),"r"(__r2),"r"(__r3)

#define SVC_Out0
#define SVC_Out1 "=r"(__r0)

#define SVC_CL0
#define SVC_CL1 "r0"

#define SVC_Call0(in, out, cl)                                                 \
  __ASM volatile ("svc 0" : out : in : cl)

#if    (defined(RTX_SVC_PTR_CHECK) && !defined(_lint))
#define SVC_Jump(f)                                                            \
  __ASM volatile (                                                             \
    ".align 2\n\t"                                                             \
    "b.w %[adr]" : : [adr] "X" (f)                                             \
  )
#define SVC_Veneer_Prototye(f)                                                 \
__STATIC_INLINE void jmpRtx##f (void);
#define SVC_Veneer_Function(f)                                                 \
__attribute__((naked,section(".text.os.svc.veneer."#f)))                       \
__STATIC_INLINE void jmpRtx##f (void) {                                        \
  SVC_Jump(svcRtx##f);                                                         \
}
#else
#define SVC_Veneer_Prototye(f)
#define SVC_Veneer_Function(f)
#endif

#define SVC0_0N(f,t)                                                           \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (void) {                                            \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In0, SVC_Out0, SVC_CL1);                                       \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_0(f,t)                                                            \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (void) {                                            \
  SVC_ArgN(0);                                                                 \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In0, SVC_Out1, SVC_CL0);                                       \
  return (t) __r0;                                                             \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_1N(f,t,t1)                                                        \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (t1 a1) {                                           \
  SVC_ArgR(0,a1);                                                              \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In1, SVC_Out1, SVC_CL0);                                       \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_1(f,t,t1)                                                         \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (t1 a1) {                                           \
  SVC_ArgR(0,a1);                                                              \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In1, SVC_Out1, SVC_CL0);                                       \
  return (t) __r0;                                                             \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_2(f,t,t1,t2)                                                      \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (t1 a1, t2 a2) {                                    \
  SVC_ArgR(0,a1);                                                              \
  SVC_ArgR(1,a2);                                                              \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In2, SVC_Out1, SVC_CL0);                                       \
  return (t) __r0;                                                             \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_3(f,t,t1,t2,t3)                                                   \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (t1 a1, t2 a2, t3 a3) {                             \
  SVC_ArgR(0,a1);                                                              \
  SVC_ArgR(1,a2);                                                              \
  SVC_ArgR(2,a3);                                                              \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In3, SVC_Out1, SVC_CL0);                                       \
  return (t) __r0;                                                             \
}                                                                              \
SVC_Veneer_Function(f)

#define SVC0_4(f,t,t1,t2,t3,t4)                                                \
SVC_Veneer_Prototye(f)                                                         \
__attribute__((always_inline))                                                 \
__STATIC_INLINE t __svc##f (t1 a1, t2 a2, t3 a3, t4 a4) {                      \
  SVC_ArgR(0,a1);                                                              \
  SVC_ArgR(1,a2);                                                              \
  SVC_ArgR(2,a3);                                                              \
  SVC_ArgR(3,a4);                                                              \
  SVC_ArgF(f);                                                                 \
  SVC_Call0(SVC_In4, SVC_Out1, SVC_CL0);                                       \
  return (t) __r0;                                                             \
}                                                                              \
SVC_Veneer_Function(f)

#endif

//lint -restore [MISRA Note 10]


//  ==== Exclusive Access Operation ====

#if (EXCLUSIVE_ACCESS == 1)

//lint ++flb "Library Begin" [MISRA Note 12]

/// Atomic Access Operation: Write (8-bit)
/// \param[in]  mem             Memory address
/// \param[in]  val             Value to write
/// \return                     Previous value
__STATIC_INLINE uint8_t atomic_wr8 (uint8_t *mem, uint8_t val) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint8_t  ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrexb %[ret],[%[mem]]\n\t"
    "strexb %[res],%[val],[%[mem]]\n\t"
    "cmp    %[res],#0\n\t"
    "bne    1b\n\t"
  : [ret] "=&l" (ret),
    [res] "=&l" (res)
  : [mem] "l"   (mem),
    [val] "l"   (val)
  : "memory"
  );

  return ret;
}

/// Atomic Access Operation: Set bits (32-bit)
/// \param[in]  mem             Memory address
/// \param[in]  bits            Bit mask
/// \return                     New value
__STATIC_INLINE uint32_t atomic_set32 (uint32_t *mem, uint32_t bits) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[val],[%[mem]]\n\t"
    "orr   %[ret],%[val],%[bits]\n\t"
    "strex %[res],%[ret],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  : [ret]  "=&l" (ret),
    [val]  "=&l" (val),
    [res]  "=&l" (res)
  : [mem]  "l"   (mem),
    [bits] "l"   (bits)
  : "memory"
  );

  return ret;
}

/// Atomic Access Operation: Clear bits (32-bit)
/// \param[in]  mem             Memory address
/// \param[in]  bits            Bit mask
/// \return                     Previous value
__STATIC_INLINE uint32_t atomic_clr32 (uint32_t *mem, uint32_t bits) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "bic   %[val],%[ret],%[bits]\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  : [ret]  "=&l" (ret),
    [val]  "=&l" (val),
    [res]  "=&l" (res)
  : [mem]  "l"   (mem),
    [bits] "l"   (bits)
  : "memory"
  );

  return ret;
}

/// Atomic Access Operation: Check if all specified bits (32-bit) are active and clear them
/// \param[in]  mem             Memory address
/// \param[in]  bits            Bit mask
/// \return                     Active bits before clearing or 0 if not active
__STATIC_INLINE uint32_t atomic_chk32_all (uint32_t *mem, uint32_t bits) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "and   %[val],%[ret],%[bits]\n\t"
    "cmp   %[val],%[bits]\n\t"
    "beq   2f\n\t"
    "clrex\n\t"
    "movs  %[ret],#0\n\t"
    "b     3f\n"
  "2:\n\t"
    "bic   %[val],%[ret],%[bits]\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  "3:"
  : [ret]  "=&l" (ret),
    [val]  "=&l" (val),
    [res]  "=&l" (res)
  : [mem]  "l"   (mem),
    [bits] "l"   (bits)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Check if any specified bits (32-bit) are active and clear them
/// \param[in]  mem             Memory address
/// \param[in]  bits            Bit mask
/// \return                     Active bits before clearing or 0 if not active
__STATIC_INLINE uint32_t atomic_chk32_any (uint32_t *mem, uint32_t bits) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "tst   %[ret],%[bits]\n\t"
    "bne   2f\n\t"
    "clrex\n\t"
    "movs  %[ret],#0\n\t"
    "b     3f\n"
  "2:\n\t"
    "bic   %[val],%[ret],%[bits]\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  "3:"
  : [ret]  "=&l" (ret),
    [val]  "=&l" (val),
    [res]  "=&l" (res)
  : [mem]  "l"   (mem),
    [bits] "l"   (bits)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Increment (32-bit)
/// \param[in]  mem             Memory address
/// \return                     Previous value
__STATIC_INLINE uint32_t atomic_inc32 (uint32_t *mem) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "adds  %[val],%[ret],#1\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Increment (16-bit) if Less Than
/// \param[in]  mem             Memory address
/// \param[in]  max             Maximum value
/// \return                     Previous value
__STATIC_INLINE uint16_t atomic_inc16_lt (uint16_t *mem, uint16_t max) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint16_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrexh %[ret],[%[mem]]\n\t"
    "cmp    %[max],%[ret]\n\t"
    "bhi    2f\n\t"
    "clrex\n\t"
    "b      3f\n"
  "2:\n\t"
    "adds   %[val],%[ret],#1\n\t"
    "strexh %[res],%[val],[%[mem]]\n\t"
    "cmp    %[res],#0\n\t"
    "bne    1b\n"
  "3:"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem),
    [max] "l"   (max)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Increment (16-bit) and clear on Limit
/// \param[in]  mem             Memory address
/// \param[in]  max             Maximum value
/// \return                     Previous value
__STATIC_INLINE uint16_t atomic_inc16_lim (uint16_t *mem, uint16_t lim) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint16_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrexh %[ret],[%[mem]]\n\t"
    "adds   %[val],%[ret],#1\n\t"
    "cmp    %[lim],%[val]\n\t"
    "bhi    2f\n\t"
    "movs   %[val],#0\n"
  "2:\n\t"
    "strexh %[res],%[val],[%[mem]]\n\t"
    "cmp    %[res],#0\n\t"
    "bne    1b\n"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem),
    [lim] "l"   (lim)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Decrement (32-bit)
/// \param[in]  mem             Memory address
/// \return                     Previous value
__STATIC_INLINE uint32_t atomic_dec32 (uint32_t *mem) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "subs  %[val],%[ret],#1\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Decrement (32-bit) if Not Zero
/// \param[in]  mem             Memory address
/// \return                     Previous value
__STATIC_INLINE uint32_t atomic_dec32_nz (uint32_t *mem) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint32_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[mem]]\n\t"
    "cmp   %[ret],#0\n\t"
    "bne   2f\n"
    "clrex\n\t"
    "b     3f\n"
  "2:\n\t"
    "subs  %[val],%[ret],#1\n\t"
    "strex %[res],%[val],[%[mem]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  "3:"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Decrement (16-bit) if Not Zero
/// \param[in]  mem             Memory address
/// \return                     Previous value
__STATIC_INLINE uint16_t atomic_dec16_nz (uint16_t *mem) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register uint16_t ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrexh %[ret],[%[mem]]\n\t"
    "cmp    %[ret],#0\n\t"
    "bne    2f\n\t"
    "clrex\n\t"
    "b      3f\n"
  "2:\n\t"
    "subs   %[val],%[ret],#1\n\t"
    "strexh %[res],%[val],[%[mem]]\n\t"
    "cmp    %[res],#0\n\t"
    "bne    1b\n"
  "3:"
  : [ret] "=&l" (ret),
    [val] "=&l" (val),
    [res] "=&l" (res)
  : [mem] "l"   (mem)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Link Get
/// \param[in]  root            Root address
/// \return                     Link
__STATIC_INLINE void *atomic_link_get (void **root) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif
  register void    *ret;

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldrex %[ret],[%[root]]\n\t"
    "cmp   %[ret],#0\n\t"
    "bne   2f\n\t"
    "clrex\n\t"
    "b     3f\n"
  "2:\n\t"
    "ldr   %[val],[%[ret]]\n\t"
    "strex %[res],%[val],[%[root]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  "3:"
  : [ret]  "=&l" (ret),
    [val]  "=&l" (val),
    [res]  "=&l" (res)
  : [root] "l"   (root)
  : "cc", "memory"
  );

  return ret;
}

/// Atomic Access Operation: Link Put
/// \param[in]  root            Root address
/// \param[in]  lnk             Link
__STATIC_INLINE void atomic_link_put (void **root, void *link) {
#ifdef  __ICCARM__
#pragma diag_suppress=Pe550
#endif
  register uint32_t val1, val2, res;
#ifdef  __ICCARM__
#pragma diag_default=Pe550
#endif

  __ASM volatile (
#ifndef __ICCARM__
  ".syntax unified\n\t"
#endif
  "1:\n\t"
    "ldr   %[val1],[%[root]]\n\t"
    "str   %[val1],[%[link]]\n\t"
    "dmb\n\t"
    "ldrex %[val1],[%[root]]\n\t"
    "ldr   %[val2],[%[link]]\n\t"
    "cmp   %[val2],%[val1]\n\t"
    "bne   1b\n\t"
    "strex %[res],%[link],[%[root]]\n\t"
    "cmp   %[res],#0\n\t"
    "bne   1b\n"
  : [val1] "=&l" (val1),
    [val2] "=&l" (val2),
    [res]  "=&l" (res)
  : [root] "l"   (root),
    [link] "l"   (link)
  : "cc", "memory"
  );
}

//lint --flb "Library End" [MISRA Note 12]

#endif  // (EXCLUSIVE_ACCESS == 1)


#endif  // RTX_CORE_CA_H_
