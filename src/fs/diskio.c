/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <ff.h>		/* Basic definitions of FatFs */
#include <diskio.h> /* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
// #include "platform.h"
// #include "storage.h"

#include <ahci.h>
#include <fat32.h>

/* Example: Mapping of physical drive number for each drive */
// #define DEV_FLASH 0 /* Map FTL to physical drive 0 */
// #define DEV_MMC 1	/* Map MMC/SD card to physical drive 1 */
// #define DEV_USB 2	/* Map USB MSD to physical drive 2 */
#define DEV_ATA 0

static struct sata_controller_port_register *__g_sata_controller_port;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
	BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv)
	{
	default:
		return 0;
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
	BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	struct sata_device *sata_dev;

	switch (pdrv)
	{
	default:
		sata_dev = get_sata_device();
		__g_sata_controller_port = sata_dev->port;
		return 0;
		break;
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
	BYTE pdrv,	  /* Physical drive nmuber to identify the drive */
	BYTE *buff,	  /* Data buffer to store read data */
	LBA_t sector, /* Start sector in LBA */
	UINT count	  /* Number of sectors to read */
)
{
	switch (pdrv)
	{
	default:
		ahci_read(__g_sata_controller_port, sector, count, buff);
		return RES_OK;
	}

	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
	BYTE pdrv,		  /* Physical drive nmuber to identify the drive */
	const BYTE *buff, /* Data to be written */
	LBA_t sector,	  /* Start sector in LBA */
	UINT count		  /* Number of sectors to write */
)
{
	switch (pdrv)
	{
	default:
		ahci_write(__g_sata_controller_port, sector, count, (void *)buff);
		return RES_OK;
	}

	return RES_PARERR;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
	BYTE pdrv, /* Physical drive nmuber (0..) */
	BYTE cmd,  /* Control code */
	void *buff /* Buffer to send/receive control data */
)
{
	pdrv = cmd;
	buff = buff;
	switch (pdrv)
	{
	default:
		break;
	}

	return RES_PARERR;
}
