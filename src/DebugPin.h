#pragma once
#include <driver/gpio.h>

namespace Take4
{
    class DebugPin
    {
    private:
        gpio_num_t pin_[4];

    public:
        DebugPin(gpio_num_t pin1, gpio_num_t pin2, gpio_num_t pin3, gpio_num_t pin4);
        ~DebugPin();

        void begin();

        void pulse(int pin, size_t num = 0);
    };

} // namespace Take4
