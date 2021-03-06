#include "BufferFormatter.h"
#include "DecodeBuffer.h"
#include <algorithm>

using namespace Take4;

template <typename ... Args>
std::string format(const std::string& fmt, Args ... args)
{
    size_t len = std::snprintf(nullptr, 0, fmt.c_str(), args ...);
    std::vector<char> buf(len + 1);
    std::snprintf(&buf[0], len + 1, fmt.c_str(), args ...);
    return std::string(&buf[0], &buf[0] + len);
}

void BufferFormatter::outputSameValue(bool outputLf)
{
    if (!sameValue_.empty()) {
        auto line = format("   -%3d", sameValue_[1]);
        std::for_each(sameValue_.begin() + 2, sameValue_.end(),
            [&](size_t value) {
                line += format(",%3d", value);
            }
        );
        if (line != prevLine_) {
            prevLine_ = line;
            puts(line.c_str());
        }
        sameValue_.clear();
    }
    else if (outputLf) {
        prevLine_.clear();
        puts("");
    }
}

void BufferFormatter::compareAndOutput(size_t pos, const BlockData& work)
{
    if (xPos_ > 0 && pos >= xPos_) {
        if (xTime_ > 0) {
            outputSameValue();
            printf("Xflush,%lld\n\n", xTime_);
        }
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
            lastNo_++;
        }
    }
}

void BufferFormatter::process(const DecodeBuffer& data)
{
    if (data.nPos() > 0) {
        bool isBflag = false;
        BlockData work(0);

        xTime_ = data.xTime();
        xPos_ = data.xPos();

        for (size_t i = 0; i < data.nPos(); i++) {
            if ((data.data(i, true) == 0xff || data.data(i, true) == 0x8e) && data.data(i, false) == 0xff) {
                // ?????????????????????????????????????????????
                compareAndOutput(i, work);
                outputSameValue(true);
                work = BlockData(0);
                isBflag = false;
            }
            else if ((data.data(i, false) & 0xF0) == 0xB0 || 
                    (data.data(i, true) == 0x8e && (data.data(i, false) & 0xF0) >= 0xA0)) {
                compareAndOutput(i, work);
                // ?????????????????????????????????????????????
                work = BlockData(lastNo_);
                isBflag = true;
                work.push_back(data.data(i, true), data.data(i, false));
            }
            else if (isBflag) {
                work.push_back(data.data(i, true), data.data(i, false));
            }
        }
        // ?????????????????????????????????????????????
        compareAndOutput(data.nPos(), work);
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

BufferFormatter::BufferFormatter() : bdatas_(), lastNo_(1), sameValue_(), xPos_(0), xTime_(0), prevLine_()
{
}

BufferFormatter::~BufferFormatter()
{
}