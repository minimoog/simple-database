#include "fakedisk.h"
#include <string.h>

static uint8_t virtual_disk[SectorSizeBytes * TotalSectors] = {0};
uint8_t* VirtualDisk()
{
    return virtual_disk;
}

void ResetDisk()
{
    memset(virtual_disk, 0, sizeof(virtual_disk));
}
