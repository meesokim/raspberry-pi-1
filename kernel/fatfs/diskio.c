/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs R0.16        (C)msx, 2026         */
/*-----------------------------------------------------------------------*/

#include <sys/stat.h>
#include <sys/time.h>

#include "emmc.h"
#ifdef HAVE_USPI
#include "uspi.h"
#endif

#include "ff.h"			/* FatFs definitions */
#include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define MMC		0	/* SD Card */
#define USB		1	/* USB Drive(s) starting from index 1 */


static struct emmc_block_dev *emmc_dev;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    switch (pdrv) {
        case MMC :
            if (emmc_dev == NULL)
            {
                return STA_NOINIT;
            }
            return 0;

#ifdef HAVE_USPI
        default :
        {
            int nDeviceIndex = pdrv - USB;
            if (nDeviceIndex < 0 || nDeviceIndex >= USPiMassStorageDeviceAvailable())
            {
                return STA_NODISK;
            }
            return 0;
        }
#endif
    }
    return STA_NODISK;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive number to identify the drive */
)
{
    switch (pdrv) {
        case MMC :
            if (emmc_dev == NULL)
            {
                if (sd_card_init((struct block_device **)&emmc_dev) != 0)
                {
                    return STA_NOINIT;
                }
            }
            return 0;

#ifdef HAVE_USPI
        default :
        {
            int nDeviceIndex = pdrv - USB;
            if (nDeviceIndex < 0 || nDeviceIndex >= USPiMassStorageDeviceAvailable())
            {
                return STA_NODISK;
            }
            return 0;
        }
#endif
    }
    return STA_NODISK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Sector address in LBA (ff16 uses LBA_t) */
    UINT count      /* Number of sectors to read */
)
{
    switch (pdrv) {
        case MMC :
        {
            size_t buf_size = count * emmc_dev->bd.block_size;
            /* sd_read takes DWORD for sector in current emmc.h, ff16 LBA_t is 32-bit by default */
            if (sd_read(buff, buf_size, (uint32_t)sector) < buf_size)
            {
                return RES_ERROR;
            }

            return RES_OK;
        }

#ifdef HAVE_USPI
        default :
        {
            int nDeviceIndex = pdrv - USB;
            if (nDeviceIndex < 0 || nDeviceIndex >= USPiMassStorageDeviceAvailable())
            {
                return RES_PARERR;
            }

            unsigned buf_size = count * USPI_BLOCK_SIZE;
            if (USPiMassStorageDeviceRead((uint32_t)sector * USPI_BLOCK_SIZE, buff, buf_size, nDeviceIndex) < buf_size)
            {
                return RES_ERROR;
            }

            return RES_OK;
        }
#endif
    }

    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Sector address in LBA */
    UINT count          /* Number of sectors to write */
)
{
    switch (pdrv) {
        case MMC :
        {
            size_t buf_size = count * emmc_dev->bd.block_size;
            if (sd_write((uint8_t *)buff, buf_size, (uint32_t)sector) < buf_size)
            {
                return RES_ERROR;
            }

            return RES_OK;
        }

#ifdef HAVE_USPI
        default :
        {
            int nDeviceIndex = pdrv - USB;
            if (nDeviceIndex < 0 || nDeviceIndex >= USPiMassStorageDeviceAvailable())
            {
                return RES_PARERR;
            }

            unsigned buf_size = count * USPI_BLOCK_SIZE;
            if (USPiMassStorageDeviceWrite((uint32_t)sector * USPI_BLOCK_SIZE, buff, buf_size, nDeviceIndex) < buf_size)
            {
                return RES_ERROR;
            }

            return RES_OK;
        }
#endif
    }

    return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    switch (pdrv) {
        case MMC :
            if (cmd == CTRL_SYNC)
            {
                return RES_OK;
            }
            if (cmd == GET_SECTOR_COUNT)
            {
                *(LBA_t *)buff = emmc_dev->bd.num_blocks;
                return RES_OK;
            }
            if (cmd == GET_SECTOR_SIZE)
            {
                *(WORD *)buff = (WORD)emmc_dev->bd.block_size;
                return RES_OK;
            }
            if (cmd == GET_BLOCK_SIZE)
            {
                *(DWORD *)buff = (DWORD)emmc_dev->bd.block_size;
                return RES_OK;
            }
            return RES_PARERR;

#ifdef HAVE_USPI
        default :
        {
            int nDeviceIndex = pdrv - USB;
            if (nDeviceIndex < 0 || nDeviceIndex >= USPiMassStorageDeviceAvailable())
            {
                return RES_PARERR;
            }
            if (cmd == CTRL_SYNC)
            {
                return RES_OK;
            }
            if (cmd == GET_SECTOR_COUNT)
            {
                *(LBA_t *)buff = USPiMassStorageDeviceGetCapacity(nDeviceIndex);
                return RES_OK;
            }
            if (cmd == GET_SECTOR_SIZE)
            {
                *(WORD *)buff = USPI_BLOCK_SIZE;
                return RES_OK;
            }
            if (cmd == GET_BLOCK_SIZE)
            {
                *(DWORD *)buff = USPI_BLOCK_SIZE;
                return RES_OK;
            }
            return RES_PARERR;
        }
#endif
    }

    return RES_PARERR;
}

#if !FF_FS_READONLY && !FF_FS_NORTC
DWORD get_fattime (void)
{
    time_t now = time(NULL);
    struct tm *ltm = localtime(&now);

    if (ltm == NULL) return 0; /* Fallback */

    return   ((DWORD)(ltm->tm_year - 80) << 25)
           | ((DWORD)(ltm->tm_mon + 1) << 21)
           | ((DWORD)ltm->tm_mday << 16)
           | ((DWORD)ltm->tm_hour << 11)
           | ((DWORD)ltm->tm_min << 5)
           | ((DWORD)ltm->tm_sec >> 1);
}
#endif
