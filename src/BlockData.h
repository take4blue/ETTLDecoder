#pragma once
#include <stdint.h>
#include <vector>
#include <iostream>

namespace Take4
{
    class BlockData
    {
    public:
        BlockData(size_t no);
        ~BlockData();

        BlockData(const BlockData& target);

        bool operator == (const BlockData& target) const;

        BlockData& operator=(const BlockData &target); 

        size_t no() const;

        void push_back(uint8_t d1, uint8_t d2);

        void print() const;

        static void add(std::string& data, uint8_t value);

    private:
        size_t no_;
        std::vector<uint8_t> d1_;
        std::vector<uint8_t> d2_;
    };
} // namespace Take4
