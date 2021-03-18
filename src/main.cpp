#include <stdint.h>
#include <driver/gpio.h>
#include "DebugPin.h"
#include "DecodeBuffer.h"
#include "ETTLDecoder.h"

const gpio_num_t PIN_X = GPIO_NUM_32;
const gpio_num_t PIN_CLK = GPIO_NUM_33;
const gpio_num_t PIN_D1 = GPIO_NUM_34;
const gpio_num_t PIN_D2 = GPIO_NUM_35;

Take4::DebugPin pin(GPIO_NUM_13, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_15);
Take4::DecodeBuffer buffer;
Take4::Esp32::ETTLDecoder pinAction(PIN_CLK, PIN_X, PIN_D1, PIN_D2, buffer, pin);


extern "C" void app_main()
{
    pinAction.begin();
    buffer.initialize();
    pin.begin();

    for(;;) {
        if (pinAction.isDeepSleep()) {
            buffer.printData();
        }
    }
}