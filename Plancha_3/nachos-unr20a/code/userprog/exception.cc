/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2020 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
// Plancha 3 - Ejercicio 2
#include "filesys/file_system.hh"

#include <stdio.h>

// Plancha 3 - Ejercicio 2
#define INIT_FILE_SIZE 10

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2), i;
    // Plancha 3 - Ejercicio 2
    char buffer[512];
    for(int lu=0;lu<512;lu++)
        buffer[lu]=0;

    switch (scid) {

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        // Plancha 3 - Ejercicio 2
        case SC_EXIT: {
            // Read Exit Status
            int s = machine->ReadRegister(4);
            DEBUG('e', "Program exited with '%u' status.\n",s);
            currentThread->Finish();
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_EXEC: {
            
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_JOIN: {
            
            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0)
                DEBUG('e', "Error: address to filename string is null.\n");

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);

            // Plancha 3 - Ejercicio 2
            DEBUG('e', "Open requested for file `%s`.\n", filename); 
            fileSystem -> Create(filename,INIT_FILE_SIZE);

            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            DEBUG('f',"%s created\n",filename);
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0)
                DEBUG('a', "Error: address to filename string is null.\n");

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('a', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);

            fileSystem -> Remove(filename);
            DEBUG('f',"%s removed\n",filename);

            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0)
                DEBUG('a', "Error: address to filename string is null.\n");

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('a', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            DEBUG('a', "Open requested for file `%s`.\n", filename);

            OpenFile *file = fileSystem -> Open(filename);
            int fid = filesTable -> Add(file);

            DEBUG('f',"%s opened for id %u.\n",filename,fid);

            machine -> WriteRegister(2, fid); 
            break;
        }

        case SC_CLOSE: {
            
            // Plancha 3 - Ejercicio 2
            OpenFileId fid = machine -> ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);
            filesTable -> Remove(fid);
            DEBUG('f', "%u closed.\n", fid);
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_READ: {
            int addr = machine -> ReadRegister(4);
            int size = machine -> ReadRegister(5);
            OpenFileId fid = machine -> ReadRegister(6);
            
            //ASSERT(addr != NULL);
            ASSERT(size > 0);

            if (fid == CONSOLE_INPUT)
            {
                for (i = 0; i < size ; i++)
                    buffer[i] = synchConsole -> GetChar();       
                WriteBufferToUser(buffer, addr, size);
                DEBUG('f', "Read %s from Console.\n", buffer);

            }
            else {
                OpenFile *file = filesTable -> Get(fid);
                i = file -> ReadAt(buffer,size,0);
                WriteBufferToUser(buffer, addr, size);
                DEBUG('f', "Read '%u' bytes from %u: '%s'.\n",i,fid, buffer);

            }

            machine -> WriteRegister(2, i); 
            break;
        }        

        // Plancha 3 - Ejercicio 2
        case SC_WRITE: {
            int addr = machine -> ReadRegister(4);
            int size = machine -> ReadRegister(5);
            OpenFileId fid = machine -> ReadRegister(6);

            //ASSERT(addr != NULL);
            ASSERT(size > 0);

            ReadBufferFromUser(addr, buffer, size);

            if (fid == CONSOLE_OUTPUT)
            {
                DEBUG('f', "Write '%s' in shell from %u.\n", buffer,fid); 
                for (i = 0; i < size && buffer[i] != '\0'; i++)
                    synchConsole -> PutChar(buffer[i]);       
            }
            else {
                OpenFile *file = filesTable -> Get(fid);
                i = file -> Write(buffer,size);
                DEBUG('f', "Write '%s' from %u.\n", buffer,fid);            }
            
            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
