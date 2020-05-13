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
    console   = new Console(readFile, writeFile, ReadAvail_, WriteDone_, this);
    readAvailSem = new Semaphore("read avail", 0);
    writeDoneSem = new Semaphore("write done", 0);
}

SynchConsole::~SynchConsole()
{
	delete console;
	delete readAvailSem;
	delete writeDoneSem;
}


char
SynchConsole::GetChar()
{
	readAvailSem -> P();
    char c = console -> GetChar();
    return c;
}

void
SynchConsole::PutChar(char c)
{
    console -> PutChar(c);
    writeDoneSem -> P();
}

void 
SynchConsole::ReadAvail(){
    readAvailSem -> V();
}

void 
SynchConsole::WriteDone(){
    writeDoneSem -> V();
}