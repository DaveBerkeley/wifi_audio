
    /*
     *
     */

class ESP32_I2S : public I2S
{
    i2s_chan_handle_t handle;

    bool error(const char *text, int err);

public:
    ESP32_I2S();

    struct Config
    {
        gpio_num_t sck;
        gpio_num_t ws;
        gpio_num_t sd;
        uint32_t freq;
        uint32_t bits;
        uint32_t slot_bits;
        bool byte_swap;
    };

    bool init(const struct Config *config);

    virtual size_t read(void *dest, size_t bytes, int idx) override;
    virtual size_t max_read_bytes() override;

    static ESP32_I2S *create(const struct Config *config);
};

//  FIN
