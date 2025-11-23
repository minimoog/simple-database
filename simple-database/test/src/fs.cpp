#include "fs.hpp"

#if USE_FF
#include "ff.h"
static FATFS fs;
#include <iostream>
#include <string.h>
#endif

std::string Path(const char * p) {
#if USE_FF
    return p;
#else
    return "." + std::string(p);
#endif
}

void initFS() {
#if USE_FF
    f_mount(&fs, "", 0);
    DIR root;
    FRESULT res = f_opendir(&root, "0:");
    if (FR_NO_FILESYSTEM == res)
    {
        MKFS_PARM mkfsParam;
        mkfsParam.fmt = (FM_FAT | FM_SFD);
        mkfsParam.n_fat = 0;
        mkfsParam.n_root = 0;
        mkfsParam.au_size = 4096*2;
        mkfsParam.align = 0;

        uint8_t fsWorkBuffer[FF_MAX_SS];
        res = f_mkfs("0", &mkfsParam , fsWorkBuffer, sizeof(fsWorkBuffer));

        if (res != FR_OK) {
            std::cout << "Error makinf file system" << std::endl;
        }

        res = f_mount(&fs, "", 0);
        if (res != FR_OK) {
            std::cout << "Error makinf file system" << std::endl;
        }
    } else {
        f_closedir(&root);
    }
#endif
}
