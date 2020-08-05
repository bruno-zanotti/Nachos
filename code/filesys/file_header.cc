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


/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize, const char *name, Directory *dir)
{
    ASSERT(freeMap != nullptr);
    DEBUG('f', "Allocating File Header with Size %u\n", fileSize);

    if (fileSize > MAX_FILE_SIZE)
        return false;

    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);
    raw.nextHeader = -1;

    if (freeMap->CountClear() < raw.numSectors + 1)
        return false;  // Not enough space.

    unsigned i = 0;
    for (; i < raw.numSectors; i++){
        if (i>=NUM_DIRECT)
            break;
        raw.dataSectors[i] = freeMap->Find();
    }
    if (i >= NUM_DIRECT){
        // We need a next header
        // Look for a sector in the freeMap
        raw.nextHeader = freeMap->Find();

        // Update directory
        char nextHeaderName[60];
        sprintf(nextHeaderName, "%s nextH", name);
        if (!dir->Add(nextHeaderName, raw.nextHeader))
            return false;  // No space in directory.

        // Create next header
        FileHeader *nextH = new FileHeader;
        bool success = nextH->Allocate(freeMap, fileSize - NUM_DIRECT * SECTOR_SIZE, nextHeaderName, dir);
        raw.numBytes = fileSize - (fileSize - NUM_DIRECT * SECTOR_SIZE);
        raw.numSectors = NUM_DIRECT;

        if(!success)
            return false;

        // Write the changes
        nextH->WriteBack(raw.nextHeader);
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

    unsigned i = 0;
    for (; i < raw.numSectors && i < NUM_DIRECT; i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!
        freeMap->Clear(raw.dataSectors[i]);
    }
    if (i >= NUM_DIRECT){
        FileHeader *nextH = new FileHeader;
        nextH->FetchFrom(raw.nextHeader);
        nextH->Deallocate(freeMap);
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    DEBUG('f', "Fetch from sector %u\n", sector);
    synchDisk->ReadSector(sector, (char *) &raw);
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    DEBUG('f', "File Header Writing Back in sector %u with nextheader %i and syze %u and syzeof %u\n", sector, raw.nextHeader, raw.numBytes, sizeof(raw));
    synchDisk->WriteSector(sector, (char *) &raw);
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
    DEBUG('f', "File Header getting Byte Sector with Offset: '%d' Sector Number: '%d' \n", offset, offset / SECTOR_SIZE);
    // Si el offset es mayor que el tamaño del header se busca en el proximo header
    if(offset / SECTOR_SIZE >= NUM_DIRECT){
        DEBUG('f', "Termino primer hdr \n");
        FileHeader *nextH = new FileHeader;
        nextH->FetchFrom(raw.nextHeader);
        return nextH->ByteToSector(offset - SECTOR_SIZE * NUM_DIRECT);
    }
    return raw.dataSectors[offset / SECTOR_SIZE];
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    DEBUG('f', "File Header getting File Length\n");
    unsigned numBytes = raw.numBytes;
    if(raw.nextHeader != -1){
        FileHeader *nextH = new FileHeader;
        nextH->FetchFrom(raw.nextHeader);
        return numBytes + nextH->FileLength();
    }
    return numBytes;
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
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}
