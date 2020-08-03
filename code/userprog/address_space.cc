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

    // Table <OpenFile*> *filesTable;
    processOpenFiles = new Table<OpenFile*>;
    // Console Input y Output
    processOpenFiles -> Add(nullptr);
    processOpenFiles -> Add(nullptr);

    #ifdef USE_TLB
    tlbLocal = new TranslationEntry[TLB_SIZE];
    for (unsigned i = 0; i < TLB_SIZE; i++)
        tlbLocal[i].valid = false;
    // Plancha 4 - Ejercicio 4
    sprintf(swapName, "swap%d.asid", fileSystem->GetFileIndex());
    DEBUG('a', "Creating Swap File '%s'\n", swapName);
    ASSERT(fileSystem->Create(swapName, numPages * PAGE_SIZE));
    swapFile = fileSystem->Open(swapName);
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
        pageTable[i].inMemory     = false;
        pageTable[i].inTLB        = false;
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
        if(pageTable[i].valid && pageTable[i].inMemory)
            mapTable->Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
    // Plancha 4 - Ejercicio 3
    #ifdef USE_TLB
    delete [] tlbLocal;
    ASSERT(fileSystem->Remove(swapName));
    delete swapFile;
    #endif
    delete exe;
    delete processOpenFiles;
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
    // Plancha 4 - Ejercicio 4    
    // Liberamos la Memoria para el proximo proceso
    for (unsigned i = 0; i < numPages; i++)
    {
        if(pageTable[i].valid && pageTable[i].inMemory)
        {
            // Guardamos en Swap las páginas que están en memoria
            /// TODO: guardar solos las páginas dirty
            saveInSwap(pageTable[i].virtualPage);
            pageTable[i].inMemory = false;
            pageTable[i].inTLB = false;
            mapTable->Clear(pageTable[i].physicalPage);
        }
    }

    for (size_t i = 0; i < TLB_SIZE; i++)
    {
        machine->GetMMU()->tlb[i].valid = false;
    }
    
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
    #ifndef USE_TLB
        machine->GetMMU()->pageTable     = pageTable;
        machine->GetMMU()->pageTableSize = numPages;
    #endif
}

// Plancha 4 - Ejercicio 1
#ifdef USE_TLB
void
AddressSpace::LoadPage(unsigned vpn)
{
    DEBUG('e', "Loading page %d in memory\n",vpn);

    // unsigned victimPage;
    unsigned victimPageTLB = machine->GetMMU()->getTLBVictimPage();
    DEBUG('e', "Indice de la página víctima de la TLB: %d\n", victimPageTLB);
    
    // Buscamos un lugar para la página en Memoria
    int pageNumber = -1;
    if (mapTable->CountClear() > 0 && ! pageTable[vpn].inMemory){
        // Si hay lugar en Memoria y la página no está en ella
        pageNumber = mapTable->Find();
    }
    if (pageNumber == -1){
        // La Memoria está llena, saco una página de memoria y la guardo en Swap
        int pageTableIndex = -1;
        pageTableIndex = getPageTableVictim(victimPageTLB);
        ASSERT(pageTableIndex != -1);
        
        pageNumber = pageTable[pageTableIndex].physicalPage;
        saveInSwap(pageTableIndex);
        pageTable[pageTableIndex].inMemory = false;
    }
    ASSERT(pageNumber != -1);

    // Cargamos la página en Memoria
    if (! pageTable[vpn].valid){
        // La página no fue cargada todavía
        pageTable[vpn].physicalPage = pageNumber;
        loadPageFromExe(vpn);
        saveInSwap(vpn);
    }
    else if (! pageTable[vpn].inMemory){
        // La página ya fue cargada pero está en Swap y no en Memoria
        loadPageFromSwap(vpn, pageNumber);    
    }

    // load page in TLB
    pageTable[vpn].valid = true;
    pageTable[vpn].inTLB = true;

    machine->GetMMU()->tlb[victimPageTLB] = pageTable[vpn];
    DEBUG('e', "Virtual Page %d Loaded Successfully in TLB[%d] with PhysicalPage %d\n", vpn, victimPageTLB, machine->GetMMU()->tlb[victimPageTLB].physicalPage);
}

void
AddressSpace::loadPageFromExe(unsigned vpn){
    unsigned physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
    char *mainMemory = machine->GetMMU()->mainMemory;   
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t initDataSize = exe->GetInitDataSize();
    uint32_t initDataAddr = exe->GetInitDataAddr();

    if (vpn * PAGE_SIZE < codeSize){
        //we are in Code
        DEBUG('a', "Loading Code\n");
        exe->ReadCodeBlock(&mainMemory[physicalAddr], PAGE_SIZE, vpn*PAGE_SIZE);
    }
    else if (vpn * PAGE_SIZE < codeSize + initDataSize){
        //we are in Data
        DEBUG('a', "Loading Data\n");
        exe->ReadDataBlock(&mainMemory[physicalAddr], PAGE_SIZE, vpn*PAGE_SIZE - initDataAddr);
    }
    else {
        //we are in Stack
        DEBUG('a', "Loading Stack %d\n", pageTable[vpn].physicalPage);
        memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);
    }
    pageTable[vpn].inMemory = true;
}

void
AddressSpace::loadPageFromSwap(unsigned vpn, unsigned physicalPage){
    DEBUG('e', "Loading Virtual Page '%d' with Physical Page %d (from Swap to Main Memory)\n", vpn, physicalPage);
    unsigned physicalAddr = physicalPage * PAGE_SIZE;
    char *mainMemory = machine->GetMMU()->mainMemory;
    memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);
    swapFile->ReadAt(&mainMemory[physicalAddr],PAGE_SIZE, vpn*PAGE_SIZE);
    pageTable[vpn].inMemory = true;
    pageTable[vpn].physicalPage = physicalPage;
}

void
AddressSpace::saveInSwap(unsigned vpn){
    DEBUG('e', "Saving Virtual Page '%d' with Physical Page %d (from Main Memory to Swap)\n", vpn, pageTable[vpn].physicalPage);
    char *mainMemory = machine->GetMMU()->mainMemory;
    unsigned physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
    swapFile->WriteAt(&mainMemory[physicalAddr],PAGE_SIZE, vpn*PAGE_SIZE);
    pageTable[vpn].dirty = false;
}

// Plancha 4 - Ejercicio 5
unsigned
AddressSpace::replaceAlgorithm(){
    // Algoritmo de Segunda oportunidad mejorada
    DEBUG('e', "Replace Algorithm for PageTable starts\n");

    for (unsigned i = 0; i <= numPages; i++)
    {
        // DEBUG('e', "Buscando Página Victima, vpn %d valid %d inMemory %d inTLb %d\n", i, pageTable[i].valid, pageTable[i].inMemory, pageTable[i].inTLB);
        if(pageTable[i].valid && pageTable[i].inMemory)
        {
            return i;
        } 
    }
    // Si no encuentra una página para reemplazar, levanta un error
    ASSERT(false);
    return 0;
}

unsigned
AddressSpace::getPageTableVictim(unsigned victimIndexTLB){
    // Algoritmo de Segunda oportunidad mejorada
    DEBUG('e', "Looking for Page to replace and save in Swap\n");

    // Primero tratamos de usar la página que libera la TLB (si está en Memoria)
    unsigned victimPageTLB = machine->GetMMU()->tlb[victimIndexTLB].virtualPage;
    if (machine->GetMMU()->tlb[victimIndexTLB].valid){
        pageTable[victimPageTLB].inTLB = false;
        DEBUG('e', "Victim page for PageTable (same as for TLB): %d\n",victimPageTLB);
        return victimPageTLB;
    }

    // usamos un algoritmo de reemplazo de página para obtener un índice de página válido
    unsigned victimPage = replaceAlgorithm();
    // Si la página está en la TLB, invalidamos la entrada 
    // (CASO TLB con espacio pero Memoria llena)
    if(pageTable[victimPage].inTLB){
        for(unsigned j = 0; j <= TLB_SIZE; j++){
            if(machine->GetMMU()->tlb[j].virtualPage == victimPage)
                machine->GetMMU()->tlb[j].valid = false;
        }
    }
    DEBUG('e', "Victim page for PageTable: %d\n",victimPage);
    return victimPage;
}

#endif