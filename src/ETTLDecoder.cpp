#include "ETTLDecoder.h"
#include "driver/timer.h"
#include "DecodeBuffer.h"
#include "DebugPin.h"

using namespace Take4::Esp32;

// 1秒あたりのカウント数
// 80MHz(APB周波数)/16(timerDivider_)だと5000000ULLになる。
static const uint64_t secTimerScale_ = TIMER_BASE_CLK / ETTLDecoder::timerDvider_;
// 下のはμ秒あたりのカウント数
static const uint64_t microTimerScale_ = secTimerScale_ / 1000000;
        
ETTLDecoder::ETTLDecoder(gpio_num_t clk, gpio_num_t x, gpio_num_t d1, gpio_num_t d2, DecodeBuffer& buffer, DebugPin& debug)
: clk_(clk)
, x_(x)
, d1_(d1)
, d2_(d2)
, clkFlag_(ClockActionFlag::DeepSleep)
, timerFlag_(TimerActionFlag::None)
, buffer_(buffer)
, mesureXTime_(false)
, debug_(debug)
{
    prevTime_.tv_sec = prevTime_.tv_usec = 0;
}

ETTLDecoder::~ETTLDecoder()
{
}

IRAM_ATTR void ETTLDecoder::timerIntr(void* user)
{
    ETTLDecoder* work = (ETTLDecoder*)user;
    work->timerAction();
}

IRAM_ATTR void ETTLDecoder::gpioIntr(void* user)
{
    ETTLDecoder* work = (ETTLDecoder*)user;
    work->pinAction();
}

void ETTLDecoder::timerAction()
{
    switch(timerFlag_) {
    case None:
        debug_.pulse(0, 1);
        break;

    case NotDetectNextLatch:
        debug_.pulse(0, 1);
        buffer_.resetLatch();
        break;

    case DetectPreFlash:
        debug_.pulse(0, 2);
        clkFlag_ = PreFlash;
        setTimer(DetectSleep);
        break;

    case DetectXFlash:
        {
            timezone tz;
            gettimeofday(&prevTime_, &tz);
            mesureXTime_ = true;
            debug_.pulse(0, 3);
        }
        break;

    case DetectSleep:
        debug_.pulse(0, 4);
        clkFlag_ = Sleep;
        setTimer(DetectDeepSleep);
        break;

    case DetectDeepSleep:
        debug_.pulse(0, 5);
        clkFlag_ = DeepSleep;
        break;
    }
}

void ETTLDecoder::pinAction()
{
    if (mesureXTime_) {
        if (gpio_get_level(x_) == 1) {
            debug_.pulse(1, 6);
            mesureXTime_ = false;
            timezone tz;
            timeval current;
            gettimeofday(&current, &tz);
            auto elapS = current.tv_sec - prevTime_.tv_sec;
            buffer_.xFlash(elapS * 1000000 + current.tv_usec - prevTime_.tv_usec);
        }
    }
    else {
        switch (clkFlag_) {
        case DeepSleep:
        case Sleep:
            if (gpio_get_level(clk_) == 1) {
                debug_.pulse(1, 4);
                clkFlag_ = Active;
                buffer_.resetLatch();
            }
            else if (gpio_get_level(x_) == 0) {
                debug_.pulse(1, 5);
                setTimer(DetectXFlash);
            }
            break;

        case Active:
            if (gpio_get_level(clk_) == 0) {
                debug_.pulse(2, 1);
                clkFlag_ = Latch;
                setTimer(DetectPreFlash);
            }
            break;

        case Latch:
            if (gpio_get_level(clk_) == 1) {
                debug_.pulse(1, 1);

                setTimer(None);
                buffer_.latch(gpio_get_level(d1_), gpio_get_level(d2_));
                clkFlag_ = Active;
            }
            break;

        case PreFlash:
            if (gpio_get_level(clk_) == 1) {
                debug_.pulse(1, 2);

                setTimer(None);
                buffer_.preFlash();
                clkFlag_ = Active;
            }
            break;
        }
    }
}

bool ETTLDecoder::isDeepSleep() const
{
    return clkFlag_ == DeepSleep;
}

void ETTLDecoder::setTimer(TimerActionFlag flag)
{
    timerFlag_ = flag;
    if (flag == None) {
        timer_pause(timerGroup_, timerIndex_);
    }
    else {
        uint64_t timer = 0;
        switch(flag) {
        case NotDetectNextLatch:
            timer = 100;
            break;
        case DetectPreFlash:
            timer = 50;
            break;
        case DetectXFlash:
            timer = 4000;
            break;
        case DetectSleep:
            timer = 1000;
            break;
        case DetectDeepSleep:
            timer = 100000;
            break;
        default:
            return;
        }
        timer_set_counter_value(timerGroup_, timerIndex_,
            0x00000000ULL);
        timer_set_alarm_value(timerGroup_, timerIndex_,
            timer * microTimerScale_);
        timer_start(timerGroup_, timerIndex_);
    }
}

void ETTLDecoder::begin()
{
    buffer_.initialize();

    // タイマー割り込みの初期化
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_DIS,
        .divider = timerDvider_,
    }; // default clock source is APB
    timer_init(timerGroup_, timerIndex_, &config);
    timer_isr_register(timerGroup_, timerIndex_,
        ETTLDecoder::timerIntr,(void *) this, ESP_INTR_FLAG_IRAM, nullptr);
    setTimer(None);

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << clk_ | 1ULL << x_ | 1ULL << d1_ | 1ULL << d2_,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_isr_register(ETTLDecoder::gpioIntr, (void*) this, ESP_INTR_FLAG_IRAM, nullptr);
    gpio_set_intr_type(clk_, GPIO_INTR_ANYEDGE);
    gpio_intr_enable(clk_);
    gpio_set_intr_type(x_, GPIO_INTR_ANYEDGE);
    gpio_intr_enable(x_);
}