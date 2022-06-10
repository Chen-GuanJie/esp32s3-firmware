#ifndef __GPIOS_H
#define __GPIOS_H
#include <stdio.h>
#include <string.h>

class gpios
{
private:
    /* data */
    uint8_t port_list[6];

public:
    gpios(uint8_t *pins, uint8_t num);
    ~gpios();
    void set(uint8_t gpio_num, uint32_t level);
};

#endif
