// Plancha 3 - Ejercicio 5
/// Print the content of the specified file in the terminal

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define OPEN_ERROR  "Error: could not found the file.\n"
#define NAME_ERROR  "Error: the file already exists.\n"
#define CREATE_ERROR  "Error: could not create file.\n"

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(-1);
    }

    // Check if the file exists
    OpenFileId file = Open(argv[1]);
    if (file <= 0) {
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        return 0;
    }

    // Print
    char buffer;
    int i = 0;
    do {
        Read(&buffer, 1, file, i);
        Write(&buffer, 1, CONSOLE_OUTPUT);
        i++;
    } 
    while (buffer != '\0');
    
    Exit(0);
}