#ifndef INCLUDE_FILESYSTEM_H_
#define INCLUDE_FILESYSTEM_H_

#include <string>
#include <vector>

#include "ff.h"

namespace fex
{

    class Filesystem
    {
    public:
        Filesystem() = default;

        Filesystem(Filesystem &&other) = default;
        Filesystem &operator=(Filesystem &&other) = default;

        bool Initialize();
        bool Mount();
        bool Unmount();
        void EraseAll();
        std::vector<std::string> List(std::string path);

        bool AddFile(const std::string& filename, const std::string& contents);
        std::string ReadFile(const std::string& filename);
        bool FileExists(const std::string& filename);
        bool DeleteFile(const std::string& filename);

    private:
        FRESULT ListAcc(char *path, std::vector<std::string> *out);

        FATFS fs_;
    };

}

#endif