#include "open_file_entry.hh"
#include <string.h>


/// The following class defines an entry in the system open files table

OpenFileEntry::OpenFileEntry(){
    name = nullptr;
    readers = 0;
    closed = false;
    lock = new Lock("Open Files Lock");
    removeLock = new Lock("Open Files Remove Lock");
    canWrite = new Condition("Open Files write condition", lock);
    canRemove = new Condition("Open Files remove condition", removeLock);
}

OpenFileEntry::~OpenFileEntry(){
    delete lock;
    delete canWrite;
    delete removeLock;
    delete canRemove;
}

bool
OpenFileEntry::IsEmpty(){
    return name == nullptr;
}

void
OpenFileEntry::StartReading(){
    if(! lock->IsHeldByCurrentThread())
        lock->Acquire();
    readers++;
    lock->Release();
}

void
OpenFileEntry::StopReading(){
    if(! lock->IsHeldByCurrentThread())
        lock->Acquire();
    readers--;
    if(readers == 0)
        canWrite-> Broadcast();
    lock->Release();
}

void
OpenFileEntry::StartWriting(){
    if(! lock->IsHeldByCurrentThread())
        lock->Acquire();
    while(readers != 0)
        canWrite->Wait();
}

void     
OpenFileEntry::StopWriting(){
    if(lock->IsHeldByCurrentThread()){
        canWrite -> Broadcast();
        lock->Release();
    }
}

void     
OpenFileEntry::Open(){
    if(! removeLock->IsHeldByCurrentThread())
        removeLock->Acquire();
    users++;
    removeLock->Release();
}

void     
OpenFileEntry::Close(){
    if(! removeLock->IsHeldByCurrentThread())
        removeLock->Acquire();
    users--;
    if(users == 0)
        canRemove -> Broadcast();
    removeLock -> Release();
}

void
OpenFileEntry::Remove(){
    if(! removeLock->IsHeldByCurrentThread())
        removeLock->Acquire();
    while(users != 0)
        canRemove->Wait();
}

//  -----------------------------------------------------------


OpenFileList::OpenFileList(){
    for (size_t i = 0; i < MAX_OPEN_FILES; i++)
        openFiles[i] = new OpenFileEntry();
}
    
OpenFileList::~OpenFileList(){
    for (size_t i = 0; i < MAX_OPEN_FILES; i++)
        delete openFiles[i];
}

int
OpenFileList::Add(const char *name){
    for (size_t i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (openFiles[i]->IsEmpty()){
            openFiles[i]->name = name;
            return i;
        }
    }
    // No more space
    return -1;
}

OpenFileEntry*
OpenFileList::Find(const char *name){
    for (size_t i = 0; i < MAX_OPEN_FILES; i++)
    {
        if(strcmp(openFiles[i]->name,name))
            return openFiles[i];
    }
    // not found
    return nullptr;
}