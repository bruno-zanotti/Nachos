#include "syscall.h"

#define NULL  ((void *) 0)

int
main(void)
{
    SpaceId    newProc;
    OpenFileId input  = CONSOLE_INPUT;
    OpenFileId output = CONSOLE_OUTPUT;
    char       prompt[2] = { '-', '-' };
    char       ch, buffer[60];
    int        i;

    for (;;) {
        Write(prompt, 2, output);
        i = 0;
        do
            /// Plancha 3 - Ejercicio 5
            Read(&buffer[i], 1, input, 0);
        while (buffer[i++] != '\n');

        buffer[--i] = '\0';
        char *argv[2];
        argv[0] = buffer;
        argv[1] = NULL;
        if (i > 0) {
            newProc = Exec(buffer,argv,1);
            Join(newProc);
        }
    }

    return -1;
}