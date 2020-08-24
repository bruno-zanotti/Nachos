/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2020 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "threads/system.hh"

#include <ctype.h>
#include <stdio.h>

// Initialize a fresh file header for a newly created file.
FileHeader::FileHeader()
{
    DEBUG('f', "Creating File Header\n");
    raw.numBytes = 0;
    raw.numSectors = 0;
    raw.nextHeader = -1;
    nextHeader = nullptr;
}

/// Allocate data blocks for the file out of the map of free disk blocks.
/// Return false if there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(unsigned size, Bitmap *freeMap)
{
    ASSERT(freeMap != nullptr);

    DEBUG('f', "Allocating Size '%u'\n", size);
    // If there is defined a Next Header
    if(raw.nextHeader != -1)
        return nextHeader->Allocate(size, freeMap);

    if (size > MAX_FILE_SIZE)
        return false;

    unsigned newSectors = DivRoundUp(size, SECTOR_SIZE);

    if (freeMap->CountClear() < newSectors)
        return false;  // Not enough space.

    unsigned oldSectors = raw.numSectors;
    unsigned i = oldSectors;

    for (; i < newSectors + oldSectors && i < NUM_DIRECT; i++)
        raw.dataSectors[i] = freeMap->Find();

    unsigned allocatedSectors = (i - oldSectors);
    raw.numSectors = allocatedSectors + oldSectors;
    raw.numBytes = raw.numSectors * SECTOR_SIZE;

    // Check if not enough Sectors in this Header for the required size
    int remainingSize = size - (allocatedSectors * SECTOR_SIZE);
    // int remainingSize = size - ((newSectors + oldSectors - raw.numSectors) * SECTOR_SIZE);

    if (remainingSize > 0){
        // We need a next header for the remaining size
        // Look for a sector in the freeMap
        raw.nextHeader = freeMap->Find();
        if(raw.nextHeader == -1)
            return false;

        // Create next header
        nextHeader = new FileHeader();
        bool success = nextHeader->Allocate(remainingSize, freeMap);

        if(!success)
            return false;

        // Write the changes
        nextHeader->WriteBack(raw.nextHeader);
    }

    DEBUG('f', "File Header successfully Allocated with Size '%u' and Next Header '%d'\n", raw.numBytes, raw.nextHeader);

    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap *freeMap)
{
    ASSERT(freeMap != nullptr);
    DEBUG('f', "Deallocating File Header\n");

    for (unsigned i = 0; i < raw.numSectors; i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!
        freeMap->Clear(raw.dataSectors[i]);
    }
    if(raw.nextHeader != -1)
        nextHeader->Deallocate(freeMap);

    DEBUG('f', "File Header deallocated\n");
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    DEBUG('f', "Fetch from sector %u\n", sector);
    synchDisk->ReadSector(sector, (char *) &raw);
    if(raw.nextHeader != -1){
        nextHeader = new FileHeader();
        nextHeader->FetchFrom(raw.nextHeader);
    }
    DEBUG('f', "Fetch from sector %u complete!\n", sector);
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    DEBUG('f', "File Header Writing Back in sector %u with NextHeader %i and Size %u\n", sector, raw.nextHeader, raw.numBytes);
    synchDisk->WriteSector(sector, (char *) &raw);
    if(raw.nextHeader != -1)
        nextHeader->WriteBack(raw.nextHeader);
    DEBUG('f', "File Header Writing Back in sector %u with NextHeader %i and Size %u complete!\n", sector, raw.nextHeader, raw.numBytes);
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    DEBUG('f', "File Header getting Byte Sector with Offset: '%d' dataSectors: '%d' \n", offset, offset / SECTOR_SIZE);
    // Si el offset es mayor que el tamaño del header se busca en el next header
    if(offset / SECTOR_SIZE >= NUM_DIRECT){
        ASSERT(nextHeader != nullptr);
        return nextHeader->ByteToSector(offset - SECTOR_SIZE * NUM_DIRECT);
    }
    DEBUG('f', "File Header getting Byte Sector with Offset: '%d' dataSectors: '%d' complete!\n", offset, offset / SECTOR_SIZE);
    return raw.dataSectors[offset / SECTOR_SIZE];

}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    DEBUG('f', "File Header getting File Length\n");
    
    if(raw.nextHeader != -1)
        return raw.numBytes + nextHeader->FileLength();

    DEBUG('f', "File Header final Lenght %u\n", raw.numBytes);
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print(const char *title)
{
    char *data = new char [SECTOR_SIZE];

    if (title == nullptr)
        printf("File header:\n");
    else
        printf("%s file header:\n", title);

    printf("    size: %u bytes\n"
           "    block indexes: ",
           raw.numBytes);

    for (unsigned i = 0; i < raw.numSectors; i++)
        printf("%u ", raw.dataSectors[i]);
    printf("\n");

    for (unsigned i = 0, k = 0; i < raw.numSectors; i++) {
        printf("    contents of block %u:\n", raw.dataSectors[i]);
        synchDisk->ReadSector(raw.dataSectors[i], data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j]))
                printf("%c", data[j]);
            else
                printf("\\%X", (unsigned char) data[j]);
        }
        printf("\n");
    }
    delete [] data;

    if(raw.nextHeader != -1)
        nextHeader->Print("Next Header");
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}
