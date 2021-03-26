#include "BlockData.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace Take4;

BlockData::BlockData(size_t no) : no_(no)
{
}
BlockData::~BlockData()
{
}

BlockData::BlockData(const BlockData& target)
{
    no_ = target.no_;
    d1_ = target.d1_;
    d2_ = target.d2_;
}

BlockData& BlockData::operator=(const BlockData &target)
{
    no_ = target.no_;
    d1_ = target.d1_;
    d2_ = target.d2_;
    return *this;
}

bool BlockData::operator == (const BlockData& target) const
{
    return d1_ == target.d1_ && d2_ == target.d2_;
}

size_t BlockData::no() const
{
    return no_;
}

void BlockData::push_back(uint8_t d1, uint8_t d2)
{
    d1_.push_back(d1);
    d2_.push_back(d2);
}

void BlockData::add(std::string& data, uint8_t value)
{
    char buffer[10];
    sprintf(buffer, ",%02x", value);
    data += buffer;
}

void BlockData::print() const
{
    std::string buffer;
    std::for_each(d1_.begin(), d1_.end(),
        [&](uint8_t val) {
            BlockData::add(buffer, val);
        }
    );
    printf("%5d%s\n", no_, buffer.c_str());
    buffer.clear();
    std::for_each(d2_.begin(), d2_.end(),
        [&](uint8_t val) {
            BlockData::add(buffer, val);
        }
    );
    printf("     %s\n", buffer.c_str());
}
