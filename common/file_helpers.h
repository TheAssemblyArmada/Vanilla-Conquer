#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H

#include <vector>
#include <string>

class FileSystem
{
public:
    struct FileInfo
    {
        std::string name;
        time_t time_write;
    };

    typedef std::vector<FileInfo> FileInfos;

public:
    FileSystem();
    virtual ~FileSystem();

    static FileSystem& Shared();

    FileInfos ListFiles(const std::string& mask) const;
    bool FileDelete(const std::string& path) const;
};

#endif // FILE_HELPERS_H