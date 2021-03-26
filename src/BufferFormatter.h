#pragma once
#include <stdint.h>
#include <vector>
#include "BlockData.h"

namespace Take4
{
    class DecodeBuffer;
    class BufferFormatter {
    private:
        std::vector<BlockData> bdatas_;
        size_t lastNo_;
        std::vector<size_t> sameValue_;
        size_t xPos_;
        uint64_t xTime_;

        void compareAndOutput(const BlockData& data);

        void outputSameValue(bool outputLf = false);

    public:
        BufferFormatter();
        ~BufferFormatter();

        void clear();

        void process(const DecodeBuffer& data);
    };
} // namespace Take4
