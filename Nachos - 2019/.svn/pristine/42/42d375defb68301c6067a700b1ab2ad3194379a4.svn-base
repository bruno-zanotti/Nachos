/// Simple test case for the threads assignment.
///
/// Create several threads, and have them context switch back and forth
/// between themselves by calling `Thread::Yield`, to illustrate the inner
/// workings of the thread system.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "system.hh"
#include "synch.hh"



#ifdef SEMAPHORE_TEST
    Semaphore *sem = new Semaphore("semaphore test", 3);
#endif


/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    
    #ifdef SEMAPHORE_TEST
        DEBUG('s', "Thread: %s tries to make P()\n", name);
        sem->P();
    #endif
        
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
    
    
    #ifdef SEMAPHORE_TEST
        DEBUG('s', "Thread: %s tries to make V()\n", name);
        sem->V();
    #endif
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching ten threads which call `SimpleThread`, and finally
/// calling `SimpleThread` ourselves.
void
ThreadTest()
{
    DEBUG('t', "Entering thread test\n");
    DEBUG('s', "Entering semaphore test\n");

    for (char num = '2'; num < '6'; num++) {
        char *name = new char [1];
        strncpy(name, &num, 1);
        Thread *newThread = new Thread(name);
        newThread->Fork(SimpleThread, (void *) name);
    }

    SimpleThread((void *) "1");
}
