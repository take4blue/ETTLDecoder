#include <stdio.h>
#include <string>
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

void DecodeBuffer::xFlash(uint64_t time)
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
        std::string d1Buffer;
        bool isBFlag = false;

        for (size_t i = 0; i < nPos_; i++) {
            if ((buffer_[i][0] == 0xff || buffer_[i][0] == 0x8e) && buffer_[i][1] == 0xff) {
                if (!d1Buffer.empty()) {
                    printf("\n           %s\n", d1Buffer.c_str());
                    d1Buffer.clear();
                }
                // ブロックデータの開始なので改行に変更する
                printf("%4d\n", i);
                isBFlag = false;
            }
            else if ((buffer_[i][1] & 0xF0) == 0xb0) {
                if (!d1Buffer.empty()) {
                    printf("\n           %s\n", d1Buffer.c_str());
                    d1Buffer.clear();
                }
                printf("%4d,%02x,%02x ", i, buffer_[i][0], buffer_[i][1]);
                isBFlag = true;
            }
            else if (isBFlag) {
                char work[10];
                sprintf(work, ",%02x", buffer_[i][0]);
                d1Buffer.append(work);
                printf(",%02x", buffer_[i][1]);
            }
            else {
                printf("%4d %02x %02x\n", i, buffer_[i][0], buffer_[i][1]);
            }
            if (xFlashTime_ > 0 && i == xFlashPostion_) {
                printf("XFlash = %llu\n", xFlashTime_);
            }
            if (preFlashPosition_ > 0 && i == preFlashPosition_) {
                printf("PreFlash\n");
            }
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
        if (nPos_ >= BufferSize_) {
            nPos_ = 0;
        }
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

uint8_t DecodeBuffer::lastData(size_t n) const
{
    if (nPos_ > 0 && n < 2) {
        return buffer_[nPos_ - 1][n];
    }
    else {
        return 0;
    }
}