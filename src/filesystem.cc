#include "filesystem.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>

#include "ff.h"
#include "flash.h"

#define DRIVE_NAME "MiRage"

namespace fex
{
    bool Filesystem::Initialize()
    {
        FRESULT fr;

        if (!Mount())
        {
            printf("Initialzing filesystem.\n");

            char buffer[4095];
            fr = f_mkfs("0", FM_ANY, 0, buffer, 4096);
            if (fr != FR_OK)
            {
                printf("Failed to make filesystem: Err(%d\n)", fr);
                return false;
            }

            f_setlabel(DRIVE_NAME);

            AddFile("README.txt", 
                    "Copy .kmf (keymap file) files into this directory to assign key maps.\n\n"
                    "After copying over the keymaps power cycle the keyboard for them to take effect.\n"
                    "To reflash keymaps either bind a key to XXXX or press and hold all 4 corner keys");

            printf("Initialized filesystem!\n");
            return true;
        }

        f_setlabel(DRIVE_NAME);

        if (!Unmount())
        {
            printf("Failed to unmount filesystem\n");
            return false;
        }

        printf("Filesystem already initialized!\n");
        return true;
    }

    bool Filesystem::Mount()
    {
        FRESULT fr = f_mount(&fs_, "", 1);
        if (fr != FR_OK)
        {
            printf("Failed to mount filesystem: Err(%d).\n", fr);
            return false;
        }
        return true;
    }

    bool Filesystem::Unmount()
    {
        FRESULT fr = f_unmount("");
        if (fr != FR_OK)
        {
            printf("Failed to unmount filesystem: Err(%d)\n", fr);
            return false;
        }
        return true;
    }

    std::vector<std::string> Filesystem::List(std::string path)
    {
        std::vector<std::string> out;

        std::vector<char> copy(path.begin(), path.end());
        copy.push_back('\0');
        if (ListAcc(copy.data(), &out) != FR_OK)
        {
            printf("Failed in acc\n");
            return {};
        }

        printf("Returning out\n");
        return out;
    }

    FRESULT Filesystem::ListAcc(char *path, std::vector<std::string> *out)
    {
        FRESULT res;
        DIR dir;
        UINT i;
        static FILINFO fno;

        printf("open dir\n");
        res = f_opendir(&dir, path); /* Open the directory */
        printf("end open dir\n");
        if (res != FR_OK)
        {
            printf("Failed to open dir: Err(%d).\n", res);
            return res;
        }

        printf("begin loop\n");
        for (;;)
        {
            printf("begin readdir\n");
            res = f_readdir(&dir, &fno); /* Read a directory item */
            printf("end readdir\n");
            if (res != FR_OK || fno.fname[0] == 0)
                break; /* Break on error or end of dir */
            printf("if 1 exit\n");
            if (fno.fattrib & AM_DIR)
            { /* It is a directory */
                continue;
                printf("at directory\n");
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                printf("recursing\n");
                res = ListAcc(path, out);
                printf("end recursing\n");
                if (res != FR_OK)
                    break;
                printf("end recursing if\n");
                path[i] = 0;
            }
            else
            { /* It is a file. */
                printf("at file\n");
                char buff[256];
                printf("path: '%s', fno: '%s'\n", path, fno.fname);
                snprintf(buff, 255, "%s/%s", path, fno.fname);
                out->push_back(std::string(buff));
                printf("adding file %s\n", buff);
            }
            printf("end loop\n");
        }

        printf("begin closedir\n");
        f_closedir(&dir);
        printf("begin closedir\n");

        return res;
    }

    void Filesystem::EraseAll()
    {
        flash_erase(FATFS_OFFSET, FATFS_SIZE);
    }

    bool Filesystem::AddFile(const std::string &filename, const std::string &contents)
    {
        FIL fp;
        FRESULT fr;

        fr = f_open(&fp, filename.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
        if (fr != FR_OK)
        {
            return false;
        }

        UINT bw;
        fr = f_write(&fp, contents.c_str(), contents.size(), &bw);

        f_close(&fp);
        return fr == FR_OK;
    }

    bool Filesystem::FileExists(const std::string &filename)
    {
        return f_stat(filename.c_str(), NULL) == FR_OK;
    }

    bool Filesystem::DeleteFile(const std::string &filename)
    {
        return f_unlink(filename.c_str()) == FR_OK;
    }

    std::string Filesystem::ReadFile(const std::string &filename)
    {

        FIL fp;
        FRESULT fr;

        fr = f_open(&fp, filename.c_str(), FA_READ);
        if (fr != FR_OK)
        {
            return "";
        }

        std::string out = "";
        char buffer[4097];
        UINT br = 0;
        do {
            fr = f_read(&fp, buffer, 4096, &br);
            UINT index = MIN(br, 4096);
            buffer[index] = '\0';
            out += buffer;

        } while(br == 4096);
        
        return out;
    }
}
