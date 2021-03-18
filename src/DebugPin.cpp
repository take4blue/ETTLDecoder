#include "DebugPin.h"

using namespace Take4;

    
DebugPin::DebugPin(gpio_num_t pin1, gpio_num_t pin2, gpio_num_t pin3, gpio_num_t pin4)
{
    pin_[0] = pin1;
    pin_[1] = pin2;
    pin_[2] = pin3;
    pin_[3] = pin4;
}

DebugPin::~DebugPin()
{
}


void DebugPin::begin()
{
    gpio_config_t io_conf = {
        1ULL << pin_[0] | 1ULL << pin_[1] | 1ULL << pin_[2] | 1ULL << pin_[3],
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_DISABLE,
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void DebugPin::pulse(int pin, size_t num)
{
    for (auto i = 0; i < num; i++) {
        gpio_set_level(pin_[pin], 1);
        gpio_set_level(pin_[pin], 0);
    }
}