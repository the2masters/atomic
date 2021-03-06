/* Copyright (c) 2007 Dean Camera
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/* $Id: atomic.h 2158 2010-06-10 15:48:28Z joerg_wunsch $ */

#ifndef _UTIL_ATOMIC_H_
#define _UTIL_ATOMIC_H_ 1

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#  ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#    define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#  else
#    error "PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP undefined, make sure to define _GNU_SOURCE globally"
#  endif
#endif

#if !defined(__DOXYGEN__)
/* Internal helper functions. */

pthread_mutex_t __attribute__((weak)) _atomic_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
uint32_t __attribute__((weak)) _atomic_count = 0;

static __inline__ void __sei(uint8_t type)
{
	if(--_atomic_count && type)
		fprintf(stderr, "you nested non recursive locks\n");
	pthread_mutex_unlock(&_atomic_mutex);
}

static __inline__ void __cli(uint8_t type)
{
	pthread_mutex_lock(&_atomic_mutex);
	if(_atomic_count++ && type)
		fprintf(stderr, "you nested non recursive locks\n");
}

static __inline__ uint8_t __iSeiRetVal(uint8_t type)
{
	__sei(type);
	return 1;
}

static __inline__ uint8_t __iCliRetVal(uint8_t type)
{
	__cli(type);
	return 1;
}

static __inline__ void __iSeiParam(const uint8_t *type)
{
	__sei(*type);
	__asm__ volatile ("" ::: "memory");
}

static __inline__ void __iCliParam(const uint8_t *type)
{
	__cli(*type);
	__asm__ volatile ("" ::: "memory");
}
#endif	/* !__DOXYGEN__ */

/** \file */
/** \defgroup util_atomic <util/atomic.h> Atomically and Non-Atomically Executed Code Blocks

    \code
    #include <util/atomic.h>
    \endcode

    \note The macros in this header file require the ISO/IEC 9899:1999
    ("ISO C99") feature of for loop variables that are declared inside
    the for loop itself.  For that reason, this header file can only
    be used if the standard level of the compiler (option --std=) is
    set to either \c c99 or \c gnu99.

    The macros in this header file deal with code blocks that are
    guaranteed to be excuted Atomically or Non-Atmomically.  The term
    "Atomic" in this context refers to the unability of the respective
    code to be interrupted.

    These macros operate via automatic manipulation of the Global
    Interrupt Status (I) bit of the SREG register. Exit paths from
    both block types are all managed automatically without the need
    for special considerations, i. e. the interrupt status will be
    restored to the same value it has been when entering the
    respective block.

    A typical example that requires atomic access is a 16 (or more)
    bit variable that is shared between the main execution path and an
    ISR.  While declaring such a variable as volatile ensures that the
    compiler will not optimize accesses to it away, it does not
    guarantee atomic access to it.  Assuming the following example:

    \code
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>

volatile uint16_t ctr;

ISR(TIMER1_OVF_vect)
{
  ctr--;
}

...
int
main(void)
{
   ...
   ctr = 0x200;
   start_timer();
   while (ctr != 0)
     // wait
       ;
   ...
}
    \endcode

    There is a chance where the main context will exit its wait loop
    when the variable \c ctr just reached the value 0xFF.  This happens
    because the compiler cannot natively access a 16-bit variable
    atomically in an 8-bit CPU.  So the variable is for example at
    0x100, the compiler then tests the low byte for 0, which succeeds.
    It then proceeds to test the high byte, but that moment the ISR
    triggers, and the main context is interrupted.  The ISR will
    decrement the variable from 0x100 to 0xFF, and the main context
    proceeds.  It now tests the high byte of the variable which is
    (now) also 0, so it concludes the variable has reached 0, and
    terminates the loop.

    Using the macros from this header file, the above code can be
    rewritten like:

    \code
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

volatile uint16_t ctr;

ISR(TIMER1_OVF_vect)
{
  ctr--;
}

...
int
main(void)
{
   ...
   ctr = 0x200;
   start_timer();
   sei();
   uint16_t ctr_copy;
   do
   {
     ATOMIC_BLOCK(ATOMIC_FORCEON)
     {
       ctr_copy = ctr;
     }
   }
   while (ctr_copy != 0);
   ...
}
    \endcode

    This will install the appropriate interrupt protection before
    accessing variable \c ctr, so it is guaranteed to be consistently
    tested.  If the global interrupt state were uncertain before
    entering the ATOMIC_BLOCK, it should be executed with the
    parameter ATOMIC_RESTORESTATE rather than ATOMIC_FORCEON.

    See \ref optim_code_reorder for things to be taken into account
    with respect to compiler optimizations.
*/

/** \def ATOMIC_BLOCK(type)
    \ingroup util_atomic

    Creates a block of code that is guaranteed to be executed
    atomically. Upon entering the block the Global Interrupt Status
    flag in SREG is disabled, and re-enabled upon exiting the block
    from any exit path.

    Two possible macro parameters are permitted, ATOMIC_RESTORESTATE
    and ATOMIC_FORCEON.
*/
#if defined(__DOXYGEN__)
#define ATOMIC_BLOCK(type)
#else
#define ATOMIC_BLOCK(type) for ( type, __ToDo = __iCliRetVal(sreg_save); \
	                       __ToDo ; __ToDo = 0 )
#endif	/* __DOXYGEN__ */

/** \def NONATOMIC_BLOCK(type)
    \ingroup util_atomic

    Creates a block of code that is executed non-atomically. Upon
    entering the block the Global Interrupt Status flag in SREG is
    enabled, and disabled upon exiting the block from any exit
    path. This is useful when nested inside ATOMIC_BLOCK sections,
    allowing for non-atomic execution of small blocks of code while
    maintaining the atomic access of the other sections of the parent
    ATOMIC_BLOCK.

    Two possible macro parameters are permitted,
    NONATOMIC_RESTORESTATE and NONATOMIC_FORCEOFF.
*/
#if defined(__DOXYGEN__)
#define NONATOMIC_BLOCK(type)
#else
#define NONATOMIC_BLOCK(type) for ( type, __ToDo = __iSeiRetVal(sreg_save); \
	                          __ToDo ;  __ToDo = 0 )
#endif	/* __DOXYGEN__ */

/** \def ATOMIC_RESTORESTATE
    \ingroup util_atomic

    This is a possible parameter for ATOMIC_BLOCK. When used, it will
    cause the ATOMIC_BLOCK to restore the previous state of the SREG
    register, saved before the Global Interrupt Status flag bit was
    disabled. The net effect of this is to make the ATOMIC_BLOCK's
    contents guaranteed atomic, without changing the state of the
    Global Interrupt Status flag when execution of the block
    completes.
*/
#if defined(__DOXYGEN__)
#define ATOMIC_RESTORESTATE
#else
#define ATOMIC_RESTORESTATE uint8_t sreg_save \
	__attribute__((__cleanup__(__iSeiParam))) = 0
#endif	/* __DOXYGEN__ */

/** \def ATOMIC_FORCEON
    \ingroup util_atomic

    This is a possible parameter for ATOMIC_BLOCK. When used, it will
    cause the ATOMIC_BLOCK to force the state of the SREG register on
    exit, enabling the Global Interrupt Status flag bit. This saves on
    flash space as the previous value of the SREG register does not
    need to be saved at the start of the block.

    Care should be taken that ATOMIC_FORCEON is only used when it is
    known that interrupts are enabled before the block's execution or
    when the side effects of enabling global interrupts at the block's
    completion are known and understood.
*/
#if defined(__DOXYGEN__)
#define ATOMIC_FORCEON
#else
#define ATOMIC_FORCEON uint8_t sreg_save \
	__attribute__((__cleanup__(__iSeiParam))) = 1
#endif	/* __DOXYGEN__ */

/** \def NONATOMIC_RESTORESTATE
    \ingroup util_atomic

    This is a possible parameter for NONATOMIC_BLOCK. When used, it
    will cause the NONATOMIC_BLOCK to restore the previous state of
    the SREG register, saved before the Global Interrupt Status flag
    bit was enabled. The net effect of this is to make the
    NONATOMIC_BLOCK's contents guaranteed non-atomic, without changing
    the state of the Global Interrupt Status flag when execution of
    the block completes.
*/
#if defined(__DOXYGEN__)
#define NONATOMIC_RESTORESTATE
#else
#define NONATOMIC_RESTORESTATE uint8_t sreg_save \
	__attribute__((__cleanup__(__iCliParam))) = 0
#endif	/* __DOXYGEN__ */

/** \def NONATOMIC_FORCEOFF
    \ingroup util_atomic

    This is a possible parameter for NONATOMIC_BLOCK. When used, it
    will cause the NONATOMIC_BLOCK to force the state of the SREG
    register on exit, disabling the Global Interrupt Status flag
    bit. This saves on flash space as the previous value of the SREG
    register does not need to be saved at the start of the block.

    Care should be taken that NONATOMIC_FORCEOFF is only used when it
    is known that interrupts are disabled before the block's execution
    or when the side effects of disabling global interrupts at the
    block's completion are known and understood.
*/
#if defined(__DOXYGEN__)
#define NONATOMIC_FORCEOFF
#else
#define NONATOMIC_FORCEOFF uint8_t sreg_save \
	__attribute__((__cleanup__(__iCliParam))) = 1
#endif	/* __DOXYGEN__ */

#endif
