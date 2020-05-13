#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

// ---------------------------
//   PLANCHA 3 - EJERCICIO 1  
// ---------------------------

void 
ReadBufferFromUser (int userAddress, char *outBuffer, unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);
    //DEBUG("Trying to read buffer from user");

    int temp;
    for(unsigned i = 0; i < byteCount; i++){
        ASSERT (machine -> ReadMem(userAddress++, 1, &temp));
        outBuffer[i] = (unsigned char) temp;
    }

}

void
WriteBufferToUser (const char *buffer, int userAddress, unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);
    //DEBUG("Trying to write buffer from user");

    for(unsigned i = 0; i < byteCount; i++)
        ASSERT (machine -> WriteMem(userAddress++, 1, buffer[i]));
}

void
WriteStringToUser (const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    for (int i = 0; string[i] != '\0'; i++)
        ASSERT(machine -> WriteMem(userAddress++, 1, string[i]));

}