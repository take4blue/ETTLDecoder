#pragma once
#include <stdint.h>
#include <stdlib.h>

namespace Take4
{
    class DecodeBuffer {
    private:
        static const size_t BufferSize_ = 10 * 1024;

        uint8_t buffer_[BufferSize_][2];
        uint8_t work_[2];
        size_t nPos_;
        size_t nLatch_;

        size_t preFlashPosition_;
        size_t xFlashPostion_;
        uint64_t xFlashTime_;

    public:
        DecodeBuffer();

        ~DecodeBuffer();

        // X接点の同期時間を設定する
        void xFlash(uint64_t time);

        // バッファのデータを出力する
        void printData();

        // プリフラッシュ情報を設定する
        void preFlash();

        // バッファなどすべてのデータをクリアする
        void initialize();

        // d1, d2をラッチしてバイトデータを作成する
        // returnがtrueの場合、8ビット目のラッチ(つまりデータ確定)を示す
        bool latch(int d1, int d2);

        // ラッチの状態情報をいったんクリアする
        void resetLatch();

        // ラッチ中のデータを確定しバッファに詰め込む
        void fixData();

        // 最後に受け取ったデータを取得する
        // 0 <= n <= 1
        uint8_t lastData(size_t n) const;
    };
}
