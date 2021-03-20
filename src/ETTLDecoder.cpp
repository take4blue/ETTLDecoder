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
, debug_(debug)
{
}

ETTLDecoder::~ETTLDecoder()
{
}

IRAM_ATTR void ETTLDecoder::timerIntr(void* user)
{
    auto work = (ETTLDecoder*)user;
    work->timerAction();
}

IRAM_ATTR void ETTLDecoder::clkPinIntr(void* user)
{
    auto work = (ETTLDecoder*)user;
    work->clkPinAction();
}

IRAM_ATTR void ETTLDecoder::xPinIntr(void* user)
{
    auto work = (ETTLDecoder*)user;
    work->xPinAction();
}

void ETTLDecoder::timerAction()
{
    timer_spinlock_take(timerGroup_);
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

    case DetectSleep:
        debug_.pulse(0, 4);
        clkFlag_ = Sleep;
        gpio_intr_enable(x_);
        setTimer(DetectDeepSleep);
        break;

    case DetectDeepSleep:
        debug_.pulse(0, 5);
        clkFlag_ = DeepSleep;
        setTimer(None);
        break;
    }
    timer_group_clr_intr_status_in_isr(timerGroup_, timerEventIndex_);
    timer_group_enable_alarm_in_isr(timerGroup_, timerEventIndex_);
    timer_spinlock_give(timerGroup_);
}

void ETTLDecoder::clkPinAction()
{
    auto clkLevel = gpio_get_level(clk_);
    switch (clkFlag_) {
    case DeepSleep:
    case Sleep:
        if (clkLevel == 1) {
            debug_.pulse(1, 4);
            clkFlag_ = Active;
            gpio_intr_disable(x_);
            buffer_.resetLatch();
        }
        break;

    case Active:
        if (clkLevel == 0) {
            debug_.pulse(2, 1);
            clkFlag_ = Latch;
            setTimer(DetectPreFlash);
        }
        break;

    case Latch:
        if (clkLevel == 1) {
            debug_.pulse(1, 1);
            setTimer(None);
            buffer_.latch(gpio_get_level(d1_), gpio_get_level(d2_));
            clkFlag_ = Active;
        }
        break;

    case PreFlash:
        if (clkLevel == 1) {
            debug_.pulse(1, 2);
            setTimer(None);
            buffer_.preFlash();
            clkFlag_ = Active;
        }
        break;
    }
}

void ETTLDecoder::xPinAction()
{
    if (gpio_get_level(x_) == 0) {
        debug_.pulse(1, 5);
        // X接点計測開始
        timer_set_counter_value(timerGroup_, elapseXIndex_, 0ULL);
        timer_start(timerGroup_, elapseXIndex_);
    }
    else {
        // X接点経過時間計算
        timer_pause(timerGroup_, elapseXIndex_);
        uint64_t value;
        timer_get_counter_value(timerGroup_, elapseXIndex_, &value);
        buffer_.xFlash(value / secTimerScale_ / 1000);
    }
}

bool ETTLDecoder::isDeepSleep() const
{
    return clkFlag_ == DeepSleep;
}

void ETTLDecoder::setTimer(TimerActionFlag flag)
{
    timerFlag_ = flag;
    if (timerFlag_ == None) {
        timer_pause(timerGroup_, timerEventIndex_);
        timer_set_counter_value(timerGroup_, timerEventIndex_,
            microTimerScale_);
    }
    else {
        uint64_t timer = 0;
        switch(timerFlag_) {
        case NotDetectNextLatch:
            timer = 100;
            break;
        case DetectPreFlash:
            timer = 50;
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
        timer_set_counter_value(timerGroup_, timerEventIndex_,
            timer * microTimerScale_);
        timer_start(timerGroup_, timerEventIndex_);
    }
}

void ETTLDecoder::begin()
{
    buffer_.initialize();

    // タイマー割り込みの初期化
    {
        timer_config_t config = {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_DOWN,
            .auto_reload = TIMER_AUTORELOAD_DIS,
            .divider = timerDvider_,
        }; // default clock source is APB
        timer_init(timerGroup_, timerEventIndex_, &config);
        timer_isr_register(timerGroup_, timerEventIndex_,
            ETTLDecoder::timerIntr, (void *)this, ESP_INTR_FLAG_IRAM, nullptr);
        timer_set_alarm_value(timerGroup_, timerEventIndex_, 0ULL);
        setTimer(None);
    }
    {
        timer_config_t config = {
            .alarm_en = TIMER_ALARM_DIS,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = TIMER_AUTORELOAD_DIS,
            .divider = timerDvider_,
        }; // default clock source is APB
        timer_init(timerGroup_, elapseXIndex_, &config);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << clk_ | 1ULL << x_ | 1ULL << d1_ | 1ULL << d2_,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_intr_type(clk_, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(x_, GPIO_INTR_ANYEDGE);

    gpio_isr_handler_add(clk_, ETTLDecoder::clkPinIntr, (void*)this);
    gpio_isr_handler_add(x_, ETTLDecoder::xPinIntr, (void*)this);
    // gpio_intr_enable(x_);
    gpio_intr_enable(clk_);
}