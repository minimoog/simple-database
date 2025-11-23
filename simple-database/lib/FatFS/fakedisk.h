#ifndef _FAKEDISK_H_
#define _FAKEDISK_H_

#include <stdint.h>

const uint16_t SectorSizeBytes = 1024;
const uint16_t TotalSectors = 1024 * 10;
uint8_t* VirtualDisk();
void ResetDisk();

#endif //_FAKEDISK_H_
