#pragma once
#include <cstdint>      
#include <queue>        
#include <unordered_set>
#include <stdexcept>  

class SRVAllocator
{
public:
    SRVAllocator(uint32_t maxDescriptors)
        : maxDescriptors_(maxDescriptors)
    {
        // index 0 はImGui用に予約
        for (uint32_t i = 1; i < maxDescriptors_; ++i)
        {
            freeIndices_.push(i);
        }
    }

    uint32_t Allocate()
    {
        if (freeIndices_.empty())
        {
            throw std::runtime_error("SRVAllocator: no available SRV slots.");
        }
        uint32_t index = freeIndices_.front();
        freeIndices_.pop();
        usedIndices_.insert(index);
        return index;
    }

    void Free(uint32_t index) 
    {
        if (index == 0) return; // ImGui予約スロット
        if (usedIndices_.erase(index) > 0)
        {
            freeIndices_.push(index);
        }
    }

    bool IsInUse(uint32_t index) const 
    {
        return usedIndices_.count(index) > 0;
    }

    // 現在使用中のインデックス数
    uint32_t GetUsedCount() const
    {
        return static_cast<uint32_t>(usedIndices_.size());
    }

    // 空いているインデックス数
    uint32_t GetFreeCount() const
    {
        return static_cast<uint32_t>(freeIndices_.size());
    }

    // 最大インデックス数（ImGui含む）
    uint32_t GetMaxDescriptors() const
    {
        return maxDescriptors_;
    }


private:
    uint32_t maxDescriptors_;
    std::queue<uint32_t> freeIndices_;
    std::unordered_set<uint32_t> usedIndices_;
};