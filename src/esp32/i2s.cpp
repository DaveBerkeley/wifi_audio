
#include "esp_attr.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

using namespace panglos;

#include "../i2s.h"
#include "esp32/i2s.h"

    /*
     *
     */

static IRAM_ATTR bool rx_overflow(i2s_chan_handle_t, i2s_event_data_t *, void *ctx)
{
    ASSERT(ctx);
    ESP32_I2S *i2s = (ESP32_I2S *) ctx;
    i2s->on_rx_error();
    return false;
}

    /*
     *
     */

bool ESP32_I2S::error(const char *text, int err)
{
    if (err == ESP_OK) return false;
    PO_ERROR("%s '%s' %d", text, lut(err_lut, err), err);
    return true;
}

    /*
     *
     */

ESP32_I2S::ESP32_I2S()
:   errors(0),
    rd_bytes(0),
    running(false)
{
}

bool ESP32_I2S::init(const ESP32_I2S::Config *config)
{
    ASSERT(config);
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
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->freq),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->sck,
            .ws = config->ws,
            .dout = I2S_GPIO_UNUSED,
            .din = config->sd,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // byte-swap for systems that want little-endian native data
    std_cfg.slot_cfg.big_endian = config->byte_swap;

    // set the bits per audio sample and the I2S bits per half frame
    std_cfg.slot_cfg.data_bit_width = (i2s_data_bit_width_t) config->bits;
    std_cfg.slot_cfg.slot_bit_width = (i2s_slot_bit_width_t) config->slot_bits;

    // Apply the I2S configuration to the channel
    err = i2s_channel_init_std_mode(handle, & std_cfg);
    if (error("i2s_channel_init_std_mode()", err))
        return false;

    i2s_event_callbacks_t cbs = {
        .on_recv = NULL,
        .on_recv_q_ovf = rx_overflow,
        .on_sent = NULL,
        .on_send_q_ovf = NULL,
    };
    err = i2s_channel_register_event_callback(handle, & cbs, this);
    if (error("i2s_channel_register_event_callback()", err))
        return false;

    err = i2s_channel_enable(handle);
    if (error("i2s_channel_enable()", err))
        return false;

    return true;
}

size_t ESP32_I2S::read(void *dest, size_t bytes, int) 
{
    ASSERT(bytes <= max_read_bytes());
    // Blocking Read
    size_t bytes_read;
    const int TIMEOUT_MS = -1; // forever
    //const int TIMEOUT_MS = 1000;
    esp_err_t err = i2s_channel_read(handle, dest, bytes, & bytes_read, TIMEOUT_MS);
    if (error("i2s_channel_read()", err))
        return 0;

    running = true;
    rd_bytes += bytes_read;
    return bytes_read;
}

size_t ESP32_I2S::max_read_bytes() 
{
    return 4064; // Hard limit on DMA buffer size, rounded down to 32-byte boundary
    //return 4092; // Hard limit on DMA buffer size
}

void ESP32_I2S::on_rx_error()
{
    if (!running) return;
    errors += 1;
}

    /*
     *
     */

ESP32_I2S *ESP32_I2S::create(const struct ESP32_I2S::Config *config)
{
    PO_DEBUG("sck=%d ws=%d sd=%d freq=%d bits=%d slot=%d", 
            config->sck, config->ws, config->sd, 
            (int) config->freq,
            config->bits, config->slot_bits);

    ESP32_I2S *i2s = new ESP32_I2S();
    bool ok = i2s->init(config);
    if (ok) return i2s;
    delete i2s;
    return 0;
}

//  FIN
