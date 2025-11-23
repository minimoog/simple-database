

#include "FileWrapper.hpp"
#include <cassert>
#include "logging.hpp"

FileWrapper::FileWrapper(const char* fpath, const char* mode)
{
#if USE_FF
    BYTE ffmode = ParseMode(mode);
    mFile = &mActualFile;
    FRESULT fRes;
    fRes = f_open(mFile, fpath, ffmode);
    if (fRes == FR_OK) {
        mFileLength = f_size(mFile);
    }
#else
    mFile = fopen(fpath, mode);
    if (mFile) {
        fseek(mFile, 0, SEEK_END);
        mFileLength = ftell(mFile);
        fseek(mFile , 0 , SEEK_SET);
    }
#endif
    else {
        LOG("Error opening file at path:");
        LOG(fpath);
        mFile = nullptr;
    }
}

bool FileWrapper::DidOpen() {
    return mFile != nullptr;
}

#if USE_FF
BYTE FileWrapper::ParseMode(const char * modeStr) {
    BYTE ffMode = 0;
    char c;
    while((c = *(modeStr++))) {
        if (c == 'b') continue;
        bool plus = false;
        if (*modeStr == '+') {
            plus = true;
            modeStr++;
        }
        switch(c) {
            case 'r':
                if (plus) { // r+
                    ffMode |= FA_READ | FA_WRITE;
                } else { // r
                    ffMode |= FA_READ;
                }
                break;
            case 'w':
                if (*modeStr == 'x') {
                    modeStr++;
                    if (*modeStr == '+') {
                        plus = true;
                        modeStr++;
                    }
                    if (plus) { // wx+ || w+x
                        ffMode |= FA_CREATE_NEW | FA_WRITE | FA_READ;
                    } else { // wx
                        ffMode |= FA_CREATE_NEW | FA_WRITE;
                    }
                } else {
                    if (plus) { // w+
                        ffMode |= FA_CREATE_ALWAYS | FA_READ | FA_WRITE;
                    } else { // w
                        ffMode |= FA_CREATE_ALWAYS | FA_WRITE;
                    }
                }
                break;
            case 'a':
                if (plus) { // a+
                    ffMode |= FA_OPEN_APPEND | FA_WRITE;
                } else { // a
                    ffMode |= FA_OPEN_APPEND | FA_WRITE | FA_READ;
                }
                break;
            default:
                //oops
                assert(false);
                return 0;
        }
    }

    return ffMode;
}
#endif

FileWrapper::~FileWrapper()
{
    Close();
}


void FileWrapper::Close()
{
    if (mFile != nullptr) {
#if USE_FF
        f_close(mFile);
#else
        fclose(mFile);
#endif
    }
}

uint32_t FileWrapper::Size()
{
    return mFileLength;
}


uint32_t FileWrapper::Pos()
{
    if (mFile == nullptr) return 0;
#if USE_FF
    return f_tell(mFile);
#else
    return ftell(mFile);
#endif
}

bool FileWrapper::Read(void* buf, uint32_t len)
{
    if (mFile == nullptr) return false;
#if USE_FF
    unsigned int x;
    FRESULT fRes;
    fRes = f_read(mFile, buf, len, &x);
    if (fRes != FR_OK) {
        LOG("Error reading file:");
        logging::Print(fRes);
        return false;
    }

    return len == x;
#else
    size_t lenRead = fread(buf, 1, len, mFile);
    if (lenRead < len) {
        LOG("Error reading file");
        return false;
    }
    return true;
#endif
}

bool FileWrapper::Write(const void* buf, uint32_t len)
{
    if (mFile == nullptr) return false;
    unsigned int x;

#if USE_FF
    FRESULT fRes;
    fRes = f_write(mFile, buf, len, &x);
    if (fRes == FR_OK) fRes = f_sync(mFile);

    if (fRes != FR_OK) {
        LOG("Error writing file");
        logging::Print(fRes);
        return false;
    }
#else
    x = fwrite(buf, sizeof(uint8_t), len, mFile);
#endif

    if (len != x) {
        LOG("Unable to write full file contents!");
        LOG("Flash is full?");
        LOG("Bytes to write:");
        logging::Print(len);
        LOG("Bytes written:");
        logging::Print(x);
        return false;
    }
    return true;
}

bool FileWrapper::Seek(uint32_t pos)
{
    if (mFile == nullptr) return false;
#if USE_FF
    FRESULT fRes;
    fRes = f_lseek(mFile, pos);
    if (fRes != FR_OK) {
        LOG("File Seek Error:");
        logging::Print(fRes);
        return false;
    }
#else
    if (0 != fseek(mFile, pos, SEEK_SET)) {
        LOG("File Seek Error");
        return false;
    }
#endif
    return true;
}
