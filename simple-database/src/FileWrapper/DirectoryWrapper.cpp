#include "DirectoryWrapper.hpp"
#include <string.h>

DirectoryWrapper::DirectoryWrapper(const char* path) {
    Open(path);
}

DirectoryWrapper::~DirectoryWrapper() {
    Close();
}

bool DirectoryWrapper::DidOpen() {
    return mDidOpen;
}

bool DirectoryWrapper::Open(const char* path) {
    strncpy(mPath, path, sizeof(mPath) - 1);
#if USE_FF
    // FF doesn't like trailing slashes
    size_t end = strlen(mPath) - 1;
    if ('/' == *(mPath + end)) *(mPath + end) = '\0';

    mDidOpen = FR_OK == f_opendir(&mDir, mPath);
    return mDidOpen;
#else
    mDidOpen = fs::exists(path);
    if (mDidOpen)
        mIter = fs::directory_iterator{path};
    return mDidOpen;
#endif
}

bool DirectoryWrapper::Close() {
#if USE_FF
    if (mDidOpen) {
        return FR_OK == f_closedir(&mDir);
    }
#endif
    return true;
}

bool DirectoryWrapper::BaseName(const char* path, char* name) {
#if USE_FF
    FILINFO fno;
    FRESULT res = f_stat(path, &fno);
    strcpy(name, fno.fname);
    return res == FR_OK;
#else
    if (!Exists(path)) return false;
    strcpy(name, fs::path(path).filename().c_str());
    return true;
#endif
}

bool DirectoryWrapper::Exists(const char* path) {
#if USE_FF
    FILINFO fno;
    FRESULT res = f_stat(path, &fno);
    return res == FR_OK;
#else
    return fs::exists(path);
#endif
}

bool DirectoryWrapper::Delete(const char* path) {
#if USE_FF
    FILINFO fno;
    FRESULT res = f_stat(path, &fno);
    if (res != FR_OK) return false;

    if (fno.fattrib & AM_DIR) {
        DirectoryWrapper dir{path};
        if (!dir.DidOpen()) return false;
        char path[PATH_MAX];
        bool isDir;

        while(dir.NextPath(path, isDir)) {
            if (!Delete(path)) return false;
        }
    }

    res = f_unlink(path);
    return FR_OK == res;
#else
    if (fs::is_directory(path)) {
        return fs::remove_all(path) != 0;
    }

    return fs::remove(path);
#endif
}

bool DirectoryWrapper::New(const char* path) {
#if USE_FF
    FRESULT res = f_mkdir(path);
    return FR_OK == res  || FR_EXIST == res;
#else
    return fs::create_directory(path);
#endif
}

bool DirectoryWrapper::NextPath(char* path, bool& isDir) {
    if (!mDidOpen) return false;
#if USE_FF
    FILINFO fno;
    FRESULT res = f_readdir(&mDir, &fno);
    if (res != FR_OK) {
        return false;
    }

    // no file found
    if (fno.fname[0] == '\0') {
        f_rewinddir(&mDir);
        return false;
    }

    strcpy(path, mPath);
    strcat(path, "/");
    strcat(path, fno.fname);

    isDir = fno.fattrib & AM_DIR;
#else
    if (mIter == fs::end(mIter)) {
        mIter = fs::begin(mIter);
        return false;
    }

    strcpy(path, mIter->path().c_str());
    isDir = mIter->is_directory();

    mIter++;
#endif

    return true;
}
