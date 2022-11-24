/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
#include "ffconf.h"
#include "fileff.h"
#include "file_diskio.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>

namespace YAPFS
{

DWORD get_fattime(void) {
    return time(nullptr);
}

static YAPFS::FATFS _fs;
static std::vector<uint8_t>* storage;

DSTATUS disk_initialize (
    BYTE drv                /* Physical drive nmuber (0..) */
){
    return 0;
}

DSTATUS disk_status (
    BYTE drv        /* Physical drive nmuber (0..) */
)
{
    return 0;
}

DRESULT disk_read (
    BYTE drv,        /* Physical drive nmuber (0..) */
    BYTE *buffer,        /* Data buffer to store read data */
    DWORD sector,    /* Sector address (LBA) */
    BYTE count        /* Number of sectors to read (1..255) */
){
    // std::cout << "Reading " << sector << std::endl;
    // for (int i = 0; i < count * 512; ++i) {
    //     auto c = buffer[i];
    //     if (c < ' ') std::cout << (int) c;
    //     else if (c >= 127) std::cout << '#';
    //     else std::cout << c;
    // }
    // std::cout << std::endl;
    memcpy(buffer, storage->data() + sector * 512, count * 512);
    return RES_OK;
}

DRESULT disk_write (
    BYTE drv,            /* Physical drive nmuber (0..) */
    const BYTE *buffer,    /* Data to be written */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count            /* Number of sectors to write (1..255) */
)
{
    // std::cout << "Writing " << sector << std::endl;
    // for (int i = 0; i < count * 512; ++i) {
    //     auto c = buffer[i];
    //     if (c < ' ') std::cout << ' ';
    //     else if (c >= 127) std::cout << '#';
    //     else std::cout << c;
    // }
    // std::cout << std::endl;

    memcpy(storage->data() + sector * 512, buffer, count * 512);
    return RES_OK;
}

DRESULT disk_ioctl (
    BYTE drv,        /* Physical drive nmuber (0..) */
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    switch(ctrl) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            if(storage) {
                *((DWORD*)buff) = storage->size() / 512;
                return RES_OK;
            }
            return RES_ERROR;
        case GET_BLOCK_SIZE:
            *((DWORD*)buff) = 1; // default when not known
            return RES_OK;
    }
    return RES_PARERR;
}

} // namespace YAPFS

void initYAPFS(std::vector<uint8_t>& storage) {
    YAPFS::storage = &storage;
    if (auto error = YAPFS::f_mount(0, &YAPFS::_fs)) {
        std::cout << "Could not mount storage " << error << std::endl;
    }
    if (auto error = YAPFS::f_mkfs(0, 1, 0)) {
        std::cout << "Could not make fs " << error << std::endl;
    }
}
