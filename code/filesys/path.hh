#ifndef NACHOS_FILESYS_PATH__HH
#define NACHOS_FILESYS_PATH__HH

#include <list>
#include <string>

class Path {
public:
    void Merge(const char* subpath);

    inline std::list<std::string>& List() { return path; }

    std::string Split();

    std::string GetPath();

private:
    std::list<std::string> path;
};

#endif