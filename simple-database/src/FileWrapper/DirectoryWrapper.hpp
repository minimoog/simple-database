#ifndef _DIRECTORYWRAPPER_HPP_
#define _DIRECTORYWRAPPER_HPP_

#if USE_FF
#include "ff.h"
#ifndef PATH_MAX
#define PATH_MAX 128
#endif //PATH_MAX
#else //USE_FF
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif //PATH_MAX
#include <filesystem>
namespace fs = std::filesystem;
#endif //USE_FF


#include "FileWrapper.hpp"

class DirectoryWrapper
{
    public:
        static inline const char * separator()
        {
#if defined(_WIN32) && !USE_FF
            return "\\";
#else
            return "/";
#endif
        }

        DirectoryWrapper() {}

        explicit DirectoryWrapper(const char* path);
        ~DirectoryWrapper();

        bool NextPath(char* path, bool& isDir);
        static bool New(const char* path);
        static bool Delete(const char* path);
        static bool Exists(const char* path);
        static bool BaseName(const char* path, char* name);

        bool Open(const char* path);
        bool Close();
        bool DidOpen();

    private:
        bool mDidOpen = false;
        char mPath[PATH_MAX] = {0};
#if USE_FF
        DIR mDir;
#else
        fs::directory_iterator mIter;
#endif
};

#endif //_DIRECTORYWRAPPER_HPP_
