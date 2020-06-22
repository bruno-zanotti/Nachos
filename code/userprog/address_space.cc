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
#include <stdio.h>

// Plancha 3 - Ejercicio 3
/// return the phiysical address related to a virtual address by a TranslationEntry
unsigned AddressTranslation(uint32_t virtualAddr,TranslationEntry* pageTable){
   DEBUG('a',"virtual address %u\n",virtualAddr);
   unsigned offset = virtualAddr % PAGE_SIZE;

   ///TODO: podría llegar a pasar que se rompa la division, habría que agregar DivRoundDown
   unsigned virtualPage =  virtualAddr / PAGE_SIZE;
   DEBUG('a',"virtual page %u and offset %u\n",virtualPage,offset);
   return (pageTable[virtualPage].physicalPage*PAGE_SIZE) + offset;
}

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    // Plancha 4 - Ejercicio 3
    exe = new Executable (executable_file);
    ASSERT(exe->CheckMagic());
    // How big is address space?

    unsigned size = exe->GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    #ifdef USE_TLB
    physicalPagesAssigned = 0;
    tlbLocal = new TranslationEntry[TLB_SIZE];
    for (unsigned i = 0; i < TLB_SIZE; i++)
        tlbLocal[i].valid = false;
    char str[60];
    // char str[] = "swap";
    // std::string index = std::to_string(fileSystem->GetFileIndex()); 
    // strcat(str, index);
    // strcat(str,".asid");
    sprintf(str, "swap%d.asid", fileSystem->GetFileIndex());
    DEBUG('a', "Creating Swap File '%s'\n", str);
    ASSERT(fileSystem->Create(str, numPages * PAGE_SIZE));
    // swapFile = new OpenFile("swap.asid");
    swapFile = fileSystem->Open(str);
    #endif

    // Plancha 3 - Ejercicio 3
    DEBUG('a', "Cantidad de páginas que ocupa el programa %u\n", numPages);
    #ifndef USE_TLB
        ASSERT(numPages <= mapTable->CountClear());
    #endif
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        // For nowe, virtual page number = physical page number.
        // Plancha 4 - Ejercicio 3
        #ifdef USE_TLB
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        #else
        // Plancha 3 - Ejercicio 3
        int pageNumber = mapTable->Find();
        ASSERT(pageNumber != -1);
        pageTable[i].physicalPage = pageNumber;
        pageTable[i].valid        = true;
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        pageTable[i].inSwap       = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    // Plancha 4 - Ejercicio 3
    #ifndef USE_TLB
    char *mainMemory = machine->GetMMU()->mainMemory;

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.

    // Plancha 3 - Ejercicio 3
    for (unsigned i = 0; i < numPages; i++) {
        unsigned physicalAddr = pageTable[i].physicalPage * PAGE_SIZE;
        memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);
    }

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t initDataSize = exe->GetInitDataSize();

    // Plancha 3 - Ejercicio 3
    if (codeSize > 0) {
        uint32_t physicalAddr, virtualAddr = exe->GetCodeAddr();
        DEBUG('a', "Initializing code segment, at virtualAddr 0x%X, en dec %d, size %u\n", virtualAddr, virtualAddr, codeSize);

        // writtenSize es la cantidad de código escrito en esta iteracion y totalWrittenCode es la cantidad de código escrito hasta ahora
        uint32_t writtenSize, totalWrittenCode = 0;
        for (unsigned i = 0; totalWrittenCode < codeSize; i++)
        {
            physicalAddr = AddressTranslation(virtualAddr + i * PAGE_SIZE, pageTable);
            DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
            writtenSize = min(PAGE_SIZE, codeSize - totalWrittenCode);
            DEBUG('a', "Loading 2 ARGS '%d' '%d' '%d'\n",physicalAddr, writtenSize, totalWrittenCode);
            exe->ReadCodeBlock(&mainMemory[physicalAddr], writtenSize, totalWrittenCode);
            totalWrittenCode += writtenSize;
        }
        DEBUG('a', "Code segment copied\n");
    }

    // Plancha 3 - Ejercicio 3
    if (initDataSize > 0) {
        uint32_t physicalAddr, virtualAddr = exe->GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, en dec %d, size %u\n", virtualAddr, virtualAddr, initDataSize);

        // writtenData es la cantidad de código escrito en esta iteracion y totalWrittenData es la cantidad de código escrito hasta ahora
        uint32_t writtenSize, totalWrittenData = 0;
        for (unsigned i = 0; totalWrittenData < initDataSize; i++)
        {
            physicalAddr = AddressTranslation(virtualAddr + i * PAGE_SIZE, pageTable);
            DEBUG('a', "direccion fisica, at 0x%X\n",physicalAddr);
            writtenSize = min(PAGE_SIZE, initDataSize - totalWrittenData);
            exe->ReadDataBlock(&mainMemory[physicalAddr], writtenSize, totalWrittenData);
            totalWrittenData += writtenSize;
        }
        DEBUG('a', "Data segment copied\n");
    }
    #endif
}

/// Deallocate an address space.
///
// Plancha 3 - Ejercicio 3
AddressSpace::~AddressSpace()
{

    for (unsigned i = 0; i < numPages; i++)
    {
        // Plancha 4 - Ejercicio 4    
        // Liberamos las paginas usadas que no estén en Swap
        if(pageTable[i].valid && ! pageTable[i].inSwap)
            mapTable->Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
    // Plancha 4 - Ejercicio 3
    #ifdef USE_TLB
    delete [] tlbLocal;
    delete swapFile;
    /// TODO: hacer remove del swapfile? ASSERT(Remove());
    #endif
    delete exe;
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
{
    // Plancha 4 - Ejercicio 3
    DEBUG('a', "Saving state..\n");
    #ifdef USE_TLB
        tlbLocal = machine->GetMMU()->tlb;
    #endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
// Plancha 4 - Ejercicio 1
void
AddressSpace::RestoreState()
{
    // Plancha 4 - Ejercicio 3
    DEBUG('a', "Restoring state..\n");
    #ifdef USE_TLB
        machine->GetMMU()->tlb = tlbLocal;
    #else
        machine->GetMMU()->pageTable     = pageTable;
        machine->GetMMU()->pageTableSize = numPages;
    #endif
}

// Plancha 4 - Ejercicio 1
#ifdef USE_TLB
void
AddressSpace::LoadPage(unsigned vpn)
{
    // ASSERT(vpn <= NUM_PHYS_PAGES);

    DEBUG('e', "Loading page %d in memory\n",vpn);
    // load page in physical memory
    int pageNumber;
    if (! pageTable[vpn].valid){
        // La página no está en la pageTable
        // Evita que un solo programa consuma toda la Memoria Física
        if (physicalPagesAssigned >= NUM_PHYS_PAGES * 2/3)
            pageNumber = -1;
        else 
            pageNumber = mapTable->Find();
        if (pageNumber == -1){
            // No hay lugar en la memoria física
            // Se envía una página de la memoria física al swap file
            // Save the page in the swapFile
            DEBUG('e', "Entranding al while\n");
            // do {
            //     pageTableIndex = (pageTableIndex+1) % numPages;
            // } while (! pageTable[pageTableIndex].valid || pageTable[pageTableIndex].inSwap);
            int pageTableIndex = -1;
            for (unsigned i = 0; i <= numPages; i++)
            {
                if(pageTable[i].valid && ! pageTable[i].inSwap)
                {
                    pageTableIndex = i;
                    break;
                } 
            }
            ASSERT(pageTableIndex != -1);
            
            pageNumber = pageTable[pageTableIndex].physicalPage;
            saveInSwap(pageTableIndex);
        }
        else 
            physicalPagesAssigned++;

        pageTable[vpn].physicalPage = pageNumber;
        loadPageFromExe(vpn);
    }
    else if (pageTable[vpn].inSwap){
        // Se envía una página de la memoria física al swap file
        // Usaremos la dirección Física liberada para la página que estaba en Swap
        // Save the page in the swapFile
        DEBUG('e', "Entranding al while\n");
        int pageTableIndex = -1;
        for (unsigned i = 0; i <= numPages; i++)
        {
            if(pageTable[i].valid && !pageTable[i].inSwap)
            {
                pageTableIndex = i;
                break;
            } 
        }
        ASSERT(pageTableIndex != -1);

        pageNumber = pageTable[pageTableIndex].physicalPage;
        pageTable[vpn].physicalPage = pageNumber;
               
        saveInSwap(pageTableIndex);
        // pageTableIndex = (pageTableIndex+1) % numPages;
        loadPageFromSwap(vpn);
    }

    // load page in TLB
    pageTable[vpn].valid = true;
    unsigned i = machine->GetMMU()->tlbIndex;
    machine->GetMMU()->tlb[i] = pageTable[vpn];
    machine->GetMMU()->tlbIndex = (i+1) % TLB_SIZE;
    DEBUG('e', "Virtual Page %d Loaded Successfully in TLB[%d] with PhysicalPage %d\n", vpn, i, machine->GetMMU()->tlb[i].physicalPage);
}

void
AddressSpace::loadPageFromExe(unsigned vpn){
    unsigned physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
    char *mainMemory = machine->GetMMU()->mainMemory;   
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t initDataSize = exe->GetInitDataSize();

    if (vpn * PAGE_SIZE < codeSize){
        //we are in Code
        DEBUG('a', "Loading Code\n");
        exe->ReadCodeBlock(&mainMemory[physicalAddr], PAGE_SIZE, vpn*PAGE_SIZE);
    }
    else if (vpn * PAGE_SIZE < codeSize + initDataSize){
        //we are in Data
        DEBUG('a', "Loading data\n");
        exe->ReadDataBlock(&mainMemory[physicalAddr], PAGE_SIZE, vpn*PAGE_SIZE);
    }
    else {
        //we are in Stack
        DEBUG('a', "Loading Stack %d\n", pageTable[vpn].physicalPage);
        memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);
    }
}

void
AddressSpace::loadPageFromSwap(unsigned vpn){
    DEBUG('e', "Loading Virtual Page '%d' with Physical Page %d (from Swap to Main Memory)\n", vpn, pageTable[vpn].physicalPage);
    unsigned physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
    char *mainMemory = machine->GetMMU()->mainMemory;
    swapFile->ReadAt(&mainMemory[physicalAddr],PAGE_SIZE, vpn*PAGE_SIZE);
    pageTable[vpn].inSwap = false;
}

void
AddressSpace::saveInSwap(unsigned vpn){
    char *mainMemory = machine->GetMMU()->mainMemory;
    /// TODO: revisar physicalPage unsigned
    DEBUG('e', "Saving Virtual Page '%d' with Physical Page %d (from Main Memory to Swap)\n", vpn, pageTable[vpn].physicalPage);
    unsigned physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
    swapFile->WriteAt(&mainMemory[physicalAddr],PAGE_SIZE, vpn*PAGE_SIZE);
    pageTable[vpn].physicalPage = -2;
    pageTable[vpn].inSwap = true;

    // actualizamos TLB
    for (unsigned i = 0; i < TLB_SIZE; i++){
        if (machine->GetMMU()->tlb[i].valid && machine->GetMMU()->tlb[i].virtualPage == vpn)
            machine->GetMMU()->tlb[i].valid = false;
    }    
}
#endif
