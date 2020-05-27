/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"


int
main(int argc, char **argv)
{
    Create("test.txt");
    OpenFileId o = Open("test.txt");
    char buffer[60];
    int i = 0;
    do
        Read(&buffer[i], 1, 0);
    while (buffer[i++] != '\n');
    
    Write(buffer,i,o);

    Close(o);
    return 0;
}
