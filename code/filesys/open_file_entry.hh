#include "directory_entry.hh"
#include "threads/synch.hh"

const unsigned MAX_OPEN_FILES = 64;  

/// The following class defines an entry in the system open files table
class OpenFileEntry {
public:
    OpenFileEntry();
    
    ~OpenFileEntry();

    bool IsEmpty();

    void StartReading();
    void StopReading();

    void StartWriting();
    void StopWriting();
    
    void Open();
    void Close();
    void Remove();

    const char *name;
    
    int closed;

private:

    // Quantity of process reading the file.
    unsigned readers;

    //  Quantity of processes who have the file open.
    unsigned users;

    Lock *lock;
    Condition *canWrite;

    Lock *removeLock;
    Condition *canRemove;
};

class OpenFileList {
public:
    OpenFileList();
    
    ~OpenFileList();

    int Add(const char *name);

    OpenFileEntry* Find(const char *name);

private:
    OpenFileEntry *openFiles[MAX_OPEN_FILES];
};