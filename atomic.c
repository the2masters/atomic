#ifndef __AVR_ARCH__

// This file needs to be compiled into the project

#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>

pthread_mutex_t _atomic_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
uint32_t _atomic_count = 0;

#endif
