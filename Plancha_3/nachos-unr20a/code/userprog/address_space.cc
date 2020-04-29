/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2020 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"

#include <string.h>

unsigned AddressTranslation(uint32_t virtualAddr,TranslationEntry* pageTable){
   DEBUG('z',"direc virtual %u\n",virtualAddr);
   unsigned offset = virtualAddr % PAGE_SIZE;
   unsigned virtualPage =  virtualAddr / PAGE_SIZE;
   DEBUG('z',"pagina %u y offset %u\n",virtualPage,offset);
   return (pageTable[virtualPage].physicalPage*PAGE_SIZE) + offset;
}


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    Executable exe (executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;


    DEBUG('a', "Cantidad de páginas que ocupa el programa %u\n", numPages);
    // Plancha 3 - Ejercicio 3
    ASSERT(numPages <= mapTable->CountClear());
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        // For now, virtual page number = physical page number.
        int pageNumber = mapTable->Find();
        ASSERT(pageNumber != -1);
        pageTable[i].physicalPage = pageNumber;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    char *mainMemory = machine->GetMMU()->mainMemory;

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.

    /// TODO: borrar línea
    /* memset(mainMemory, 0, size); */

    // Plancha 3 - Ejercicio 3
    for (unsigned i = 0; i < numPages; i++) {
        unsigned addr = pageTable[i].physicalPage * PAGE_SIZE;
        memset(&mainMemory[addr], 0, PAGE_SIZE);
    }

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    if (codeSize > 0) {
        uint32_t physicalAddr, virtualAddr = exe.GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, en dec %d, size %u\n", virtualAddr, virtualAddr, codeSize);
        
        // Cantidad de páginas que ocupa el code en la memoria virtual
        unsigned int numVirtualPages = DivRoundUp(codeSize, PAGE_SIZE);
        DEBUG('a', "Cantidad de páginas que ocupa el code en la memoria virtual %u\n", numVirtualPages);

        // Página de la memoria virtual donde está la dirección virtualAddr 
        /// TODO: setear valor
        unsigned int initVirtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);

        unsigned i = 0;
        for (; i < numVirtualPages-1; i++)
        {
            physicalAddr = pageTable[initVirtualPage++].physicalPage * PAGE_SIZE;
            DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
            exe.ReadCodeBlock(&mainMemory[physicalAddr], PAGE_SIZE, 0);
        }
        
        uint32_t codeRestante = codeSize - PAGE_SIZE * i;
        physicalAddr = pageTable[initVirtualPage++].physicalPage * PAGE_SIZE;        
        DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
        DEBUG('a', "Escribe %d\n",codeRestante);
        exe.ReadCodeBlock(&mainMemory[physicalAddr], codeRestante, 0);

    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, en dec %d, size %u\n", virtualAddr, virtualAddr, initDataSize);

        // Cantidad de páginas que ocupa el data en la memoria virtual
        unsigned int numVirtualPages = DivRoundUp(initDataSize, PAGE_SIZE);
        DEBUG('a', "Cantidad de páginas que ocupa el data en la memoria virtual %u\n", numVirtualPages);

        // Página de la memoria virtual donde está la dirección virtualAddr 
        /// TODO: setear valor
        unsigned int initVirtualPage = DivRoundDown(virtualAddr,PAGE_SIZE);
        int offset = virtualAddr % PAGE_SIZE;
       
        uint32_t physicalAddr = pageTable[initVirtualPage++].physicalPage * PAGE_SIZE;
        DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
        exe.ReadDataBlock(&mainMemory[physicalAddr + offset], PAGE_SIZE - offset, 0);

        for (unsigned i = 1; i < numVirtualPages; i++)
        {
            /* uint32_t  */physicalAddr = pageTable[initVirtualPage++].physicalPage * PAGE_SIZE;
            DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
            exe.ReadDataBlock(&mainMemory[physicalAddr], PAGE_SIZE, 0);
        }

    }

}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{

    for (unsigned i = 0; i < numPages; i++)
    {
        mapTable->Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
}
