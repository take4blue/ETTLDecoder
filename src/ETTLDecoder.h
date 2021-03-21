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
        // ETTLデコーダークラス
        // X, CLKピンの割り込みはgpio_isr_handler_addを使うのでbeginの前にgpio_install_isr_serviceの呼び出しが必要
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
                DetectSleep,            // CLKがLOWのままなのでスリープとして認識された
                DetectDeepSleep,        // Sleepからさらに時間経過したとして認識
            };

            // 以下2つはどのタイマーを使うかの指定
            static const timer_group_t timerGroup_ = TIMER_GROUP_0;
            static const timer_idx_t timerEventIndex_ = TIMER_0;
            // X接点時間計測用タイマー
            static const timer_idx_t elapseXIndex_ = TIMER_1;

            gpio_num_t clk_;
            gpio_num_t x_;
            gpio_num_t d1_;
            gpio_num_t d2_;
            ClockActionFlag clkFlag_;
            TimerActionFlag timerFlag_;

            DecodeBuffer& buffer_;

            DebugPin& debug_;

            // タイマー割り込み関数
            static IRAM_ATTR void timerIntr(void* user);

            // タイマー割り込みから呼び出されるタイマー処理関する
            void timerAction();

            // Clkピンの割り込み処理
            static IRAM_ATTR void clkPinIntr(void* user);

            void clkPinAction();

            // Xピンの割り込み処理
            static IRAM_ATTR void xPinIntr(void* user);

            void xPinAction();

            void setTimer(TimerActionFlag flag);

        public:
            static const uint32_t timerDvider_ = 16;

            ETTLDecoder(gpio_num_t clk, gpio_num_t x, gpio_num_t d1, gpio_num_t d2, DecodeBuffer& buffer, DebugPin& debug);
            ~ETTLDecoder();

            void begin();

            void end();

            bool isDeepSleep() const;
        };
    } // namespace Esp32
    
} // namespace Take4
