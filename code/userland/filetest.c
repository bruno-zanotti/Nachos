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
// #include "time.h"

int
main(int argc, char *argv[])
{
    Write("\nStart: ", 8, CONSOLE_OUTPUT);
    Write(argv[1], sizeof(argv[1]) - 1, CONSOLE_OUTPUT);

    OpenFileId o = Open("test.txt");
    Write("Hello world",12,o);
    Write(argv[1], sizeof(argv[1]) - 1, o);

    Close(o);
    
    // sleep(2);
    Write("\nFinish: ", 8, CONSOLE_OUTPUT);
    Write(argv[1], sizeof(argv[1]) - 1, CONSOLE_OUTPUT);
    return 0;
}

// int
// main(void)
// {
//     Create("test.txt");
//     OpenFileId o = Open("test.txt");
//     Write("Hello world\n",12,o);
//     Close(o);
//     return 0;
// }
