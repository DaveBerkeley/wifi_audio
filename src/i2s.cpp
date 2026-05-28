
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

using namespace panglos;

#include "i2s.h"

    /*
     *
     */

class _I2S : public I2S
{
    i2s_chan_handle_t handle;

    bool error(const char *text, int err)
    {
        if (err == ESP_OK) return false;
        PO_ERROR("%s '%s' %d", text, lut(err_lut, err), err);
        return true;
    }

public:
    _I2S() { }

    bool init(gpio_num_t sck, gpio_num_t ws, gpio_num_t sd, uint32_t freq, bool byte_swap)
    {
        PO_DEBUG("sck=%d ws=%d sd=%d freq=%d", sck, ws, sd, (int) freq);
        // General channel configuration (handles DMA and role)
        i2s_chan_config_t chan_cfg = {
            .id = I2S_NUM_AUTO,
            .role = I2S_ROLE_SLAVE,
            .dma_desc_num = 8,        // Number of DMA descriptors
            .dma_frame_num = 512,     // Stay under 1023
            .auto_clear = true,       // Helps prevent audio artifacts : pads data with 0
        };

        esp_err_t err = i2s_new_channel(& chan_cfg, 0, & handle);
        if (error("i2s_new_channel()", err))
            return false;

        // I2S Standard Mode Configuration (handles protocol, clocks, and data format)
        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(freq),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = sck,
                .ws = ws,
                .dout = I2S_GPIO_UNUSED,
                .din = sd,
                .invert_flags = {
                    .mclk_inv = false,
                    .bclk_inv = false,
                    .ws_inv = false,
                },
            },
        };

        // byte-swap for systems that want little-endian native data
        std_cfg.slot_cfg.big_endian = byte_swap;

        // Apply the I2S configuration to the channel
        err = i2s_channel_init_std_mode(handle, & std_cfg);
        if (error("i2s_channel_init_std_mode()", err))
            return false;

        err = i2s_channel_enable(handle);
        if (error("i2s_channel_enable()", err))
            return false;

        return true;
    }

    virtual size_t read(void *dest, size_t bytes, int idx) override
    {
        UNUSED(idx);
        ASSERT(bytes <= max_read_bytes());
        // Blocking Read
        size_t bytes_read;
        const int TIMEOUT_MS = -1; // forever
        //const int TIMEOUT_MS = 1000;
        esp_err_t err = i2s_channel_read(handle, dest, bytes, & bytes_read, TIMEOUT_MS);
        if (error("i2s_channel_read()", err))
            return 0;

        return bytes_read;
    }

    virtual size_t max_read_bytes() override
    {
        return 4064; // Hard limit on DMA buffer size, rounded down to 32-byte boundary
        //return 4092; // Hard limit on DMA buffer size
    }

};

    /*
     *
     */

I2S *I2S::create(int sck, int ws, int sd, uint32_t freq, bool byte_swap)
{
    _I2S *i2s = new _I2S();
    bool ok = i2s->init((gpio_num_t) sck, (gpio_num_t) ws, (gpio_num_t) sd, freq, byte_swap);
    if (ok) return i2s;
    delete i2s;
    return 0;
}

//  FIN
