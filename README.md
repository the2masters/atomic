I adjusted atomic.h from avr-libc to use pthread_mutex.

I use it to test / debug code written for AVR on my x86 development machine with
multiple threads (main + interrupts)
