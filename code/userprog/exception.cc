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
#include "address_space.hh"
// Plancha 3 - Ejercicio 4
#include "args.hh"


#include <stdio.h>

// Plancha 3 - Ejercicio 2
#define INIT_FILE_SIZE 64


/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void
StartProcess(void *argv_)
{
    // Reinterpret args as a string.
    char **argv = (char **) argv_;

    // Set the initial register
    currentThread -> space -> InitRegisters();
    // Load page table register.
    currentThread -> space -> RestoreState();   

    // write args in the stack
    unsigned argc = WriteArgs(argv);

    // Decrease by 16 because it was increased during WriteArgs
    // to make room for "register saves".
    int argvAddr = machine->ReadRegister(STACK_REG);
    machine->WriteRegister(STACK_REG, argvAddr - 16);

    machine -> WriteRegister(4, argc); 
    machine -> WriteRegister(5, argvAddr);

    machine -> Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}

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
    int scid = machine -> ReadRegister(2), i;
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
            int status = machine -> ReadRegister(4);
            DEBUG('e', "Program exited with '%u' status.\n",status);
            // Plancha 4 - Ejercicio 2
            stats->Print();
            currentThread->Finish(status);
            break;
        }

        // Plancha 3 - Ejercicio 2 - Ejercicio 4 
        case SC_EXEC: {
            int filenameAddr = machine -> ReadRegister(4);
            int argsAddr     = machine -> ReadRegister(5);
            int joinable     = machine -> ReadRegister(6);

            // read file name
            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof(filename)))
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",FILE_NAME_MAX_LEN);

            // read arguments
            char** argv = nullptr;
            argv = SaveArgs(argsAddr);

        	for(int j=0; argv[j] != nullptr; j++){
                DEBUG('e', "args %s.\n", argv[j]);    
            }

            DEBUG('e', "Filename %s.\n", filename);    
            // open filename
            OpenFile *executable = fileSystem->Open(filename);
            if (executable == nullptr) {
                DEBUG('e', "Entra al if.\n");    
                DEBUG('e',"Unable to open file %s\n", filename);
                machine -> WriteRegister(2, -1);
                break;
            }

            // create address space
            AddressSpace *space = new AddressSpace(executable);
            // Plancha 4 - Ejercicio 3
            #ifndef USE_TLB
                delete executable;
            #endif
            
            // create child thread
            Thread *childThread = new Thread(filename,joinable);
            childThread->space = space;

            DEBUG('e', "SC_EXEC Program: '%s' starts.\n",filename);
            childThread -> Fork(StartProcess, (void *) argv);
            SpaceId id = userProgTable -> Add(childThread);

            DEBUG('e', "SC_EXEC has finished.\n");
    
            //return exit status
            machine -> WriteRegister(2, id); 
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_JOIN: {
            SpaceId id = machine -> ReadRegister(4);
            DEBUG('e', "SC_JOIN starts with ID %d.\n", id);
            Thread *thread = userProgTable -> Get(id);
            
            ASSERT(thread != nullptr);

            int status = thread -> Join();
            currentThread -> Yield();
            DEBUG('e', "SC_JOIN has finished with status: %d.\n",status);
            machine -> WriteRegister(2, status);
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
            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            fileSystem -> Create(filename,INIT_FILE_SIZE);

            DEBUG('e',"%s created\n",filename);
            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0)
                DEBUG('e', "Error: address to filename string is null.\n");

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);

            fileSystem -> Remove(filename);
            DEBUG('e',"%s removed\n",filename);

            break;
        }

        // Plancha 3 - Ejercicio 2
        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0)
                DEBUG('e', "Error: address to filename string is null.\n");

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            DEBUG('e', "Open requested for file `%s`.\n", filename);

            OpenFile *file = fileSystem -> Open(filename);
            if (file == nullptr){
                DEBUG('e', "OPEN: file `%s` not found.\n", filename);
                machine -> WriteRegister(2, -1); 
                break;
            }
            int fid = filesTable -> Add(file);

            DEBUG('e',"File '%s' with id '%u' opened.\n",filename,fid);

            machine -> WriteRegister(2, fid); 
            break;
        }

        case SC_CLOSE: {
            
            // Plancha 3 - Ejercicio 2
            OpenFileId fid = machine -> ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);
            filesTable -> Remove(fid);
            DEBUG('e', "%u closed.\n", fid);
            break;
        }

        // Plancha 3 - Ejercicio 2
        // Plancha 3 - Ejercicio 4
        // Added offset to be able to read some parts of a file.
        case SC_READ: {
            int addr = machine -> ReadRegister(4);
            int size = machine -> ReadRegister(5);
            OpenFileId fid = machine -> ReadRegister(6);
            int offset = machine -> ReadRegister(7);

            //ASSERT(addr != NULL);
            ASSERT(size > 0);

            if (fid == CONSOLE_INPUT)
            {
                for (i = 0; i < size ; i++)
                    buffer[i] = synchConsole -> GetChar();       
                WriteBufferToUser(buffer, addr, size);
                DEBUG('e', "Read %s from Console.\n", buffer);

            }
            else {
                OpenFile *file = filesTable -> Get(fid);
                if (file == nullptr){
                    DEBUG('e', "READ: file `%s` not found.\n", file);
                    machine -> WriteRegister(2, -1); 
                    break;
                }
                i = file -> ReadAt(buffer,size,offset);
                WriteBufferToUser(buffer, addr, size);
                DEBUG('e', "Read '%u' bytes from %d: '%s'.\n",i,fid,buffer);
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
                DEBUG('e', "Write '%s' in shell from %u.\n", buffer,fid); 
                for (i = 0; i < size && buffer[i] != '\0'; i++)
                    synchConsole -> PutChar(buffer[i]);
                machine -> WriteRegister(2, i); 
                  
            }
            else {
                OpenFile *file = filesTable -> Get(fid);
                if (!file) {
                    DEBUG('e', "Write: file with id '%d' not found.\n", fid);
                    machine->WriteRegister(2, -1);
                    break;
                }

                i = file -> Write(buffer,size);
                DEBUG('e', "Write '%s' from %u.\n", buffer,fid);            }
                machine -> WriteRegister(2, i); 
            
            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}

// Plancha 4 - Ejercicio 1
static void
PageFaultHandler(ExceptionType _et)
{
    unsigned Addr = machine->ReadRegister(BAD_VADDR_REG);
    unsigned page = DivRoundDown(Addr, PAGE_SIZE);
    
    DEBUG('e', "Page Fault Exception with page '%d'.\n", page);
    #ifdef USE_TLB
        currentThread -> space -> LoadPage(page);
        DEBUG('e', "Page '%d' loaded in TLB.\n", page);
    #endif
}
// Plancha 4 - Ejercicio 1
static void
ReadOnlyHandler(ExceptionType _et)
{
    unsigned Addr = machine->ReadRegister(BAD_VADDR_REG);
    unsigned page = DivRoundDown(Addr, PAGE_SIZE);

    DEBUG('e', "Read Only Exception for page '%d'.\n", page);
    ASSERT(false);
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    // Plancha 4 - Ejercicio 1
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &ReadOnlyHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
