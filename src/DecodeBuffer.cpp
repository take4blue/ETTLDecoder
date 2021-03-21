#include <stdio.h>
#include "DecodeBuffer.h"
#include <esp_attr.h>

using namespace Take4;

DecodeBuffer::DecodeBuffer()
: nPos_(0)
, nLatch_(0)
, preFlashPosition_(0)
, xFlashPostion_(0)
, xFlashTime_(0)
{
}

DecodeBuffer::~DecodeBuffer()
{
    // gpio_isr_handler_remove(pin());
}

void DecodeBuffer::xFlash(long time)
{
    xFlashPostion_ = nPos_;
    xFlashTime_ = time;
}

void DecodeBuffer::preFlash()
{
    resetLatch();
    preFlashPosition_ = nPos_;
}

void DecodeBuffer::initialize()
{
    nPos_ = nLatch_ = 0;
    work_[0] = work_[1] = 0;
}

// 取り込んだデータの出力と初期化処理
void DecodeBuffer::printData()
{
    if (nPos_ > 0) {
        for (size_t i = 0; i < nPos_; i++) {
            printf("%4d %02x %02x\n", i, buffer_[i][0], buffer_[i][1]);
        }
        if (preFlashPosition_ > 0) {
            printf("PreFlash = %d\n", preFlashPosition_);
        }
        if (xFlashTime_ > 0) {
            printf("XFlash = %d - %ld\n", xFlashPostion_, xFlashTime_);
        }
        nPos_ = 0;
        nLatch_ = 0;
        preFlashPosition_ = 0;
        xFlashPostion_ = 0;
        xFlashTime_ = 0;
    }
}

// ラッチデータの登録
void DecodeBuffer::fixData()
{
    if (nLatch_ > 0) {
        buffer_[nPos_][0] = work_[0];
        buffer_[nPos_][1] = work_[1];
        work_[0] = work_[1] = 0;
        nPos_++;
        nLatch_ = 0;
    }
}

// D1, D2ピンのラッチとシフト処理
IRAM_ATTR bool DecodeBuffer::latch(int d1, int d2)
{
    work_[0] = (work_[0] << 1) | (d1 & 0x1);
    work_[1] = (work_[1] << 1) | (d2 & 0x1);
    nLatch_++;
    if (nLatch_ >= 8) {
        fixData();
        return true;
    }
    else {
        return false;
    }
}

void DecodeBuffer::resetLatch()
{
    work_[0] = work_[1] = 0;
    nLatch_ = 0;
}