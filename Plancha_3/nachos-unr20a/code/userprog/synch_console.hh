// Plancha 3 - Ejercicio 2

#ifndef NACHOS_SYNCHCONSOLE
#define NACHOS_SYNCHCONSOLE


#include "machine/console.hh"
#include "threads/synch.hh"

class SynchConsole {
public:

    SynchConsole(const char *readFile, const char *writeFile);

    ~SynchConsole();

    char GetChar();
    void PutChar(char c);
    
    void ReadAvail();
    void WriteDone();

private:
    Console *console;
    Semaphore *readAvailSem;  
    Semaphore *writeDoneSem;
};

#endif