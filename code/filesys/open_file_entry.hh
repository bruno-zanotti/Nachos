#include "directory_entry.hh"

/// The following class defines an entry in the system open files table
class OpenFileEntry {
public:

    // File name
    char name[FILE_NAME_MAX_LEN+1];

    /// The number of the file.
    unsigned fileID;

    // Quantity of process using the file.
    unsigned users;
    
};