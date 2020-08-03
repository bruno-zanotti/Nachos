/// Simple test routines for the file system.
///
/// We implement:
///
/// Copy
///     Copy a file from UNIX to Nachos.
/// Print
///     Cat the contents of a Nachos file.
/// Perftest
///     A stress test for the Nachos file system read and write a really
///     really large file in tiny chunks (will not work on baseline system!)
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2020 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>



static const unsigned TRANSFER_SIZE = 10;  // Make it small, just to be
                                           // difficult.

/// Copy the contents of the UNIX file `from` to the Nachos file `to`.
void
Copy(const char *from, const char *to)
{
    ASSERT(from != nullptr);
    ASSERT(to != nullptr);

    // Open UNIX file.
    FILE *fp = fopen(from, "r");
    if (fp == nullptr) {
        printf("Copy: could not open input file %s\n", from);
        return;
    }

    // Figure out length of UNIX file.
    fseek(fp, 0, 2);
    int fileLength = ftell(fp);
    fseek(fp, 0, 0);

    DEBUG('f', "Copying file %s, size %u, to file %s\n",
          from, fileLength, to);

    // Create a Nachos file of the same length.
    if (!fileSystem->Create(to, fileLength)) {  // Create Nachos file.
        printf("Copy: could not create output file %s\n", to);
        fclose(fp);
        return;
    }

    OpenFile *openFile = fileSystem->Open(to);
    ASSERT(openFile != nullptr);

    // Copy the data in `TRANSFER_SIZE` chunks.
    char *buffer = new char [TRANSFER_SIZE];
    int amountRead;
    while ((amountRead = fread(buffer, sizeof(char),
                               TRANSFER_SIZE, fp)) > 0)
        openFile->Write(buffer, amountRead);
    delete [] buffer;

    // Close the UNIX and the Nachos files.
    delete openFile;
    fclose(fp);
}

/// Print the contents of the Nachos file `name`.
void
Print(const char *name)
{
    ASSERT(name != nullptr);

    OpenFile *openFile = fileSystem->Open(name);
    if (openFile == nullptr) {
        fprintf(stderr, "Print: unable to open file %s\n", name);
        return;
    }

    char *buffer = new char [TRANSFER_SIZE];
    int amountRead;
    while ((amountRead = openFile->Read(buffer, TRANSFER_SIZE)) > 0)
        for (unsigned i = 0; i < (unsigned) amountRead; i++)
            printf("%c", buffer[i]);

    delete [] buffer;
    delete openFile;  // close the Nachos file
}


/// Performance test
///
/// Stress the Nachos file system by creating a large file, writing it out a
/// bit at a time, reading it back a bit at a time, and then deleting the
/// file.
///
/// Implemented as three separate routines:
/// * `FileWrite` -- write the file.
/// * `FileRead` -- read the file.
/// * `PerformanceTest` -- overall control, and print out performance #'s.

static const char FILE_NAME[] = "TestFile";
static const char CONTENTS[] = "1234567890";
static const unsigned CONTENT_SIZE = sizeof CONTENTS - 1;
static const unsigned FILE_SIZE = CONTENT_SIZE * 5000;

static void
FileWrite()
{
    printf("Sequential write of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);

    if (!fileSystem->Create(FILE_NAME, 0)) {
        fprintf(stderr, "Perf test: cannot create %s\n", FILE_NAME);
        return;
    }

    OpenFile *openFile = fileSystem->Open(FILE_NAME);
    if (openFile == nullptr) {
        fprintf(stderr, "Perf test: unable to open %s\n", FILE_NAME);
        return;
    }

    for (unsigned i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        int numBytes = openFile->Write(CONTENTS, CONTENT_SIZE);
        if (numBytes < 10) {
            fprintf(stderr, "Perf test: unable to write %s\n", FILE_NAME);
            break;
        }
    }

    delete openFile;
}

static void
FileRead()
{
    printf("Sequential read of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);

    OpenFile *openFile = fileSystem->Open(FILE_NAME);
    if (openFile == nullptr) {
        fprintf(stderr, "Perf test: unable to open file %s\n", FILE_NAME);
        return;
    }

    char *buffer = new char [CONTENT_SIZE];
    for (unsigned i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        int numBytes = openFile->Read(buffer, CONTENT_SIZE);
        if (numBytes < 10 || strncmp(buffer, CONTENTS, CONTENT_SIZE)) {
            printf("Perf test: unable to read %s\n", FILE_NAME);
            break;
        }
    }

    delete [] buffer;
    delete openFile;
}

void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FILE_NAME)) {
        printf("Perf test: unable to remove %s\n", FILE_NAME);
        return;
    }
    stats->Print();
}

void
function(void *name_)
{
    char *name = (char *) name_;

    char *buffer = new char [64];

    char text[100];
    for(int j=0;j<100;j++)
        text[j] = name[0];

    printf("Thread: %s opens the file\n", name);
<<<<<<< HEAD
    OpenFile *openFile = fileSystem->Open("test.txt");
=======
    OpenFile *openFile = fileSystem->Open("small");
>>>>>>> 0fc1d10a1dc5c9f9e8c00bb757c430e4f37c5617
    // This is the spring of our discontent.
    for (size_t i = 0; i < 1; i++)
    {
        openFile->WriteAt(text, 5, 0);
        // printf("Thread: %s writes '%s'\n", name, text);
        openFile->ReadAt(buffer, 10, 0);
<<<<<<< HEAD
        openFile->(buffer, 10, 0);
        printf("Thread: %s reads '%s'\n", name, buffer);
        currentThread->Yield();
    }
    fileSystem->Close("test.txt");
=======
        printf("Thread: %s reads '%s'\n", name, buffer);
        currentThread->Yield();
    }
>>>>>>> 0fc1d10a1dc5c9f9e8c00bb757c430e4f37c5617

    // // DEBUG('t', "Thread: %s is reading\n", name);
    // printf("Thread: %s is reading\n", name);
    // for (unsigned num = 0; num < 20; num++) {
    //     int numBytes = openFile->Read(buffer, 10);
    //     // DEBUG('t', "Thread: %s reads caracter %s\n", name, buffer);
    //     printf("Thread: %s reads caracter '%s'\n", name, buffer);
    //     currentThread->Yield();
    // }

    // for (unsigned num = 0; num < 10; num++) {
    //     printf("*** Thread `%s` is running: iteration %u\n", name, num);
    //     currentThread->Yield();
    // }

    printf("!!! Thread `%s` has finished\n", name);

<<<<<<< HEAD
=======
    // fileSystem->Remove("small");
>>>>>>> 0fc1d10a1dc5c9f9e8c00bb757c430e4f37c5617
    // for (size_t i = 0; i < 10; i++)
    // {
    //     fileSystem->close();
    // }
    
    
}

void
SynchRead()
{
    printf("Starting Synch Read test:\n");

    for (char num = '2'; num < '6'; num++) {
        char *name = new char [1];
        strncpy(name, &num, 1);
        Thread *newThread = new Thread(name);
        newThread->Fork(function, (void *) name);
    }

    function((void *) "1");  
<<<<<<< HEAD
    fileSystem->Remove("test.txt");
=======
>>>>>>> 0fc1d10a1dc5c9f9e8c00bb757c430e4f37c5617
}

