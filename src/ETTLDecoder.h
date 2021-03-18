#pragma once
#include <driver/gpio.h>
#include <sys/time.h>
#include <hal/timer_types.h>

namespace Take4
{
    class DebugPin;
    class DecodeBuffer;
    namespace Esp32
    {
        class ETTLDecoder
        {
        private:
            // 現在の状態
            enum ClockActionFlag {
                DeepSleep,
                Sleep,
                Active,
                Latch,
                PreFlash,
            };

            // タイマー割り込みが発生した時の状態を示す
            enum TimerActionFlag {
                None,
                NotDetectNextLatch,     // 次のラッチがされなかった
                DetectPreFlash,         // プリフラッシュと認識されたCLKのパルス
                DetectXFlash,           // X接点の同調フラッシュと認識されたパルス
                DetectSleep,            // CLKがLOWのままなのでスリープとして認識された
                DetectDeepSleep,        // Sleepからさらに時間経過したとして認識
            };

            // 以下2つはどのタイマーを使うかの指定
            static const timer_idx_t timerIndex_ = TIMER_0;
            static const timer_group_t timerGroup_ = TIMER_GROUP_0;

            gpio_num_t clk_;
            gpio_num_t x_;
            gpio_num_t d1_;
            gpio_num_t d2_;
            ClockActionFlag clkFlag_;
            TimerActionFlag timerFlag_;

            DecodeBuffer& buffer_;

            timeval prevTime_;
            bool mesureXTime_;
            DebugPin& debug_;

            // タイマー割り込み関数
            static IRAM_ATTR void timerIntr(void* user);

            // タイマー割り込みから呼び出されるタイマー処理関する
            void timerAction();

            // GPIO状態変化時の割り込み関数
            static IRAM_ATTR void gpioIntr(void* user);

            void pinAction();

            void setTimer(TimerActionFlag flag);

        public:
            static const uint32_t timerDvider_ = 16;

            ETTLDecoder(gpio_num_t clk, gpio_num_t x, gpio_num_t d1, gpio_num_t d2, DecodeBuffer& buffer, DebugPin& debug);
            ~ETTLDecoder();

            void begin();

            bool isDeepSleep() const;
        };
    } // namespace Esp32
    
} // namespace Take4
