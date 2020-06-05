// Plancha 3 - Ejercicio 2

#include "synch_console.hh"

static void 
ReadAvail_(void* data){
    ASSERT(data != nullptr);
    ((SynchConsole *) data) -> ReadAvail();
}

static void
WriteDone_(void* data){
    ASSERT(data != nullptr);
    ((SynchConsole *) data) -> WriteDone();
}


SynchConsole::SynchConsole(const char *readFile, const char *writeFile)
{
    console   = new Console(nullptr, nullptr, ReadAvail_, WriteDone_, this);
    readAvailSem = new Semaphore("read avail", 0);
    writeDoneSem = new Semaphore("write done", 0);
    readLock = new Lock("Read synch console lock");    
    writeLock = new Lock("Write synch console lock");    
}

SynchConsole::~SynchConsole()
{
	delete console;
	delete readAvailSem;
	delete writeDoneSem;
    delete readLock;
    delete writeLock;
}


char
SynchConsole::GetChar()
{
	readLock->Acquire();
    readAvailSem -> P();
    char c = console -> GetChar();
    readLock->Release();
    return c;
}

void
SynchConsole::PutChar(char c)
{
    writeLock->Acquire();
    console -> PutChar(c);
    writeDoneSem -> P();
    writeLock->Release();
}

void 
SynchConsole::ReadAvail(){
    readAvailSem -> V();
}

void 
SynchConsole::WriteDone(){
    writeDoneSem -> V();
}