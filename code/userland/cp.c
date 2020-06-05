// Plancha 3 - Ejercicio 5
/// Copies files specified on the command line into a specific directory

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define OPEN_ERROR  "Error: could not found the file.\n"
#define NAME_ERROR  "Error: the file already exists.\n"
#define CREATE_ERROR  "Error: could not create file.\n"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(-1);
    }

    // Check if the file exists
    if (Open(argv[2]) >= 0) {
        Write(NAME_ERROR, sizeof(NAME_ERROR) - 1, CONSOLE_OUTPUT);
        return 0;
    }

    // Create new file 
    if (Create(argv[2]) < 0) {
        Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
        return 0;
    }
    OpenFileId new = Open(argv[2]);

    // Check if the old file exists
    OpenFileId old = Open(argv[1]);
    if (old <= 0) {
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        return 0;
    }

    char buffer;
    int i = 0;
    do {
        Read(&buffer, 1, old, i);
        Write(&buffer, 1, new);
        i++;
    } 
    while (buffer != '\0');
    
    return 1;
}