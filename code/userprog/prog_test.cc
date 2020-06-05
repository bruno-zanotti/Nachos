/// Test routines for demonstrating that Nachos can load a user program and
/// execute it.
///
/// Also, routines for testing the Console hardware device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2020 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
//#include "machine/console.hh"
// Plancha 3 - Ejercicio 2
#include "userprog/synch_console.hh"
#include "threads/synch.hh"
#include "threads/system.hh"

#include <stdio.h>


/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void
StartProcess(const char *filename)
{
    ASSERT(filename != nullptr);

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) {
        printf("Unable to open file %s\n", filename);
        return;
    }

    AddressSpace *space = new AddressSpace(executable);
    currentThread->space = space;

    delete executable;

    space->InitRegisters();  // Set the initial register values.
    space->RestoreState();   // Load page table register.

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}



// Plancha 3 - Ejercicio 2
void
ConsoleTest(const char *in, const char *out)
{
    SynchConsole *console = new SynchConsole(in,out);
    for (;;) {

        char ch = console -> GetChar();        // Wait for character to arrive.
        console -> PutChar(ch);                // Echo it!

        if (ch == 'q')
            return;  // If `q`, then quit.
    }
}
