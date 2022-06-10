#include "gpios.h"
#include "driver/gpio.h"
gpios::gpios(uint8_t *pins, uint8_t num)
{
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.21
    io_conf.pin_bit_mask = 1ULL << *pins;
    for (uint8_t i = 0; i < num - 1; i++)
    {
        io_conf.pin_bit_mask = io_conf.pin_bit_mask | 1ULL << pins[i + 1];
    }
    // disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

gpios::~gpios()
{
}
void gpios::set(uint8_t gpio_num, uint32_t level)
{
    gpio_set_level((gpio_num_t)port_list[gpio_num], level);
}
