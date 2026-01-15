#include "AudioHandle.h"
#include "AudioPlayer.h"

using namespace FromEngine;

std::array<int, static_cast<size_t>(AudioID::count)> AudioHandle::audioIndices_ = {};
bool AudioHandle::initialized_ = false;

void AudioHandle::Initialize()
{
    if (initialized_) return;

    for (const auto& def : audioDefinitions_)
    {
        size_t index = static_cast<size_t>(def.id);
        audioIndices_[index] = AudioPlayer::GetInstance().Load(def.path);
    }

    initialized_ = true;
}

int AudioHandle::Get(AudioID id)
{
    assert(initialized_);
    return audioIndices_[static_cast<size_t>(id)];
}