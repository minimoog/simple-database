/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include <string.h>

#include "fakedisk.h"

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    (void) pdrv;
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    (void) pdrv;
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    (void) pdrv;

    uint32_t addr = sector * SectorSizeBytes;
    if (addr + (count * SectorSizeBytes) > SectorSizeBytes * TotalSectors) {
        return RES_PARERR;
    }

    memcpy(buff, VirtualDisk() + addr, count * SectorSizeBytes);

    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    (void) pdrv;

    uint32_t addr = sector * SectorSizeBytes;
    if (addr + (count * SectorSizeBytes) > SectorSizeBytes * TotalSectors) {
        return RES_PARERR;
    }

    memcpy(VirtualDisk() + addr, buff, count * SectorSizeBytes);
    
    return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    (void) pdrv;

    switch(cmd) 
    {
        case GET_SECTOR_COUNT:
            *(uint32_t*)buff = TotalSectors;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(uint16_t*)buff = SectorSizeBytes;
            return RES_OK;
        case GET_BLOCK_SIZE:
            //Retrieves erase block size of the flash memory media in unit of sector into the DWORD variable pointed by buff. The allowable value is 1 to 32768 in power of 2. Return 1 if the erase block size is unknown or non flash memory media. This command is used by only f_mkfs function and it attempts to align data area on the erase block boundary. It is required when FF_USE_MKFS == 1.
            *(uint32_t*)buff = 1;
            return RES_OK;
        case CTRL_SYNC:
            return RES_OK;
    }

    return RES_PARERR;
}
