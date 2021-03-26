#include "BufferFormatter.h"
#include "DecodeBuffer.h"
#include <algorithm>

using namespace Take4;

void BufferFormatter::outputSameValue(bool outputLf)
{
    if (!sameValue_.empty()) {
        printf("%5d-%5d", sameValue_[0], sameValue_[1]);
        std::for_each(sameValue_.begin() + 2, sameValue_.end(), [](size_t value) { printf(",%5d", value);});
        puts("");
        sameValue_.clear();
    }
    else if (outputLf) {
        puts("");
    }
}

void BufferFormatter::compareAndOutput(const BlockData& work)
{
    if (xPos_ > 0 && work.no() >= xPos_) {
        outputSameValue();
        printf("Xflush,%d,%lld\n\n", xPos_, xTime_);
        xPos_ = 0;
    }
    if (work.no() != 0) {
        auto result = std::find(bdatas_.begin(), bdatas_.end(), work);
        if (result != bdatas_.end()) {
            if (sameValue_.empty()) {
                sameValue_.push_back(work.no());
                sameValue_.push_back(result->no());
            }
            else {
                sameValue_.push_back(result->no());
            }
        }
        else {
            outputSameValue();
            work.print();
            bdatas_.push_back(work);
        }
    }
}

void BufferFormatter::process(const DecodeBuffer& data)
{
    if (data.nPos() > 0) {
        bool isBflag = false;
        BlockData work(0);

        xTime_ = data.xTime();
        xPos_ = data.xPos() + lastNo_;

        for (size_t i = 0; i < data.nPos(); i++) {
            if ((data.data(i, true) == 0xff || data.data(i, true) == 0x8e) && data.data(i, false) == 0xff) {
                // ここまでのデータの比較及び出力
                compareAndOutput(work);
                outputSameValue(true);
                work = BlockData(0);
                isBflag = false;
            }
            else if ((data.data(i, false) & 0xF0) == 0xB0) {
                compareAndOutput(work);
                // ここまでのデータの比較及び出力
                work = BlockData(i + lastNo_);
                isBflag = true;
                work.push_back(data.data(i, true), data.data(i, false));
            }
            else if (isBflag) {
                work.push_back(data.data(i, true), data.data(i, false));
            }
        }
        // ここまでのデータの比較及び出力
        compareAndOutput(work);
        lastNo_ += data.nPos() + 1;
    }
}

void BufferFormatter::clear()
{
    bdatas_.clear();
    sameValue_.clear();
    lastNo_ = 0;
    xPos_ = 0;
    xTime_ = 0;
}

BufferFormatter::BufferFormatter() : bdatas_(), lastNo_(0), sameValue_(), xPos_(0), xTime_(0)
{
}

BufferFormatter::~BufferFormatter()
{
}