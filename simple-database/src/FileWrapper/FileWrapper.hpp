
#ifndef _FILEWRAPPER_HPP_
#define _FILEWRAPPER_HPP_
#include <stdint.h>

#if USE_FF
#include "ff.h"
#else
#include <cstdio>
#endif

class FileWrapper
{
public:
    explicit FileWrapper(const char* fpath, const char * mode = "r");
    ~FileWrapper();

#if USE_FF
    static BYTE ParseMode(const char* modeStr);
#endif

    bool DidOpen();
    void Close();
    bool Read(void* buf, uint32_t len);
    bool Write(const void* buf, uint32_t len);
    bool Seek(uint32_t pos);
    uint32_t Pos();
    uint32_t Size();

private:
#if USE_FF
    FIL* mFile;
    FIL mActualFile;
#else
    FILE* mFile;
#endif

    uint32_t mFileLength = 0;
};

#endif //_FILEWRAPPER_HPP_
