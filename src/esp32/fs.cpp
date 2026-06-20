
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "panglos/debug.h"

#include "panglos/arch.h"
#include "panglos/hal.h"

#include "fs.h"

#define MOUNT_POINT "/sdcard"

// WORK IN PROGRESS

static bool error(const char *text, esp_err_t err)
{
    if (err == ESP_OK) return true;
    PO_ERROR("%s %s", text, lut(panglos::err_lut, (err)));
    return false;
}

bool init_fs(struct SpiPins *pins)
{
    ASSERT(pins);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;

    PO_DEBUG("Initializing SD card");
    PO_DEBUG("Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    //host.unaligned_multi_block_rw_max_chunk_size = 8;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = pins->mosi,
        .miso_io_num = pins->miso,
        .sclk_io_num = pins->ck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    const spi_host_device_t slot = (spi_host_device_t) host.slot;
    esp_err_t ret = spi_bus_initialize(slot, & bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        return ("Failed to initialize bus", ret);
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = pins->cs;
    slot_config.host_id = slot;

    PO_DEBUG("Mounting filesystem");

    ret = esp_vfs_fat_sdspi_mount(mount_point, & host, & slot_config, & mount_config, & card);
    if (ret != ESP_OK)
    {
        return ("Failed to mount filesystem", ret);
    }

    PO_DEBUG("Filesystem mounted");
    return true;
}

//  FIN
