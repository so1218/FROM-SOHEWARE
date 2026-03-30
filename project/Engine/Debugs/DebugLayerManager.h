#pragma once

namespace FE
{

class DebugLayerManager
{
public:
    static DebugLayerManager& Instance()
    {
        static DebugLayerManager instance;
        return instance;
    }

    void Initialize();

private:
    DebugLayerManager();
    ~DebugLayerManager();
    DebugLayerManager(const DebugLayerManager&) = delete;
    DebugLayerManager& operator=(const DebugLayerManager&) = delete;

#ifdef _DEBUG
    ID3D12Debug1* debugController_;
#endif
};

}