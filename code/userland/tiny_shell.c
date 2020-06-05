#include "syscall.h"


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

        if (i > 0) {
            newProc = Exec(buffer);
            Join(newProc);
        }
    }

    return -1;
}
