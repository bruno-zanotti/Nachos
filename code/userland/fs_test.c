#include "syscall.h"
#include <string.h>

// #define NULL  ((void *) 0)

int
main()
{
    SpaceId    newProc;
    OpenFileId input  = CONSOLE_INPUT;
    OpenFileId output = CONSOLE_OUTPUT;
    char       prompt[2] = { '-', '-' };
    char       ch = '0';
    char       buffer[60];
    int        i;

    Write("Hello world",12,CONSOLE_OUTPUT);
    Create("test.txt");
    for (i=0;i<5;i++) {
        char *argv[2];
        argv[0] = "filetest";
        argv[1] = &ch;
        // strncpy(argv[1], &ch, 1);
        argv[2] = NULL;
        Write("Exec test",9,CONSOLE_OUTPUT);
        newProc = Exec(argv[0],argv,0);
        ch++;
    }

    return -1;
}
