#include "pch.h"
#include "StageCatalog.h"

namespace StageCatalog
{

namespace
{

const std::vector<StageDefinition> kStages =
{
    { "Stage1", "STAGE 1" },
    { "Stage2", "STAGE 2" },
};

int selectedIndex = 0;
bool isJustSwitched = false;

}

const std::vector<StageDefinition>& GetStages()
{
    return kStages;
}

int GetSelectedIndex()
{
    return selectedIndex;
}

const StageDefinition& GetSelectedStage()
{
    return kStages[selectedIndex];
}

void Select(int index)
{
    const int stageCount = static_cast<int>(kStages.size());
    const int wrappedIndex = ((index % stageCount) + stageCount) % stageCount;
    if (wrappedIndex == selectedIndex)
    {
        return;
    }
    selectedIndex = wrappedIndex;
    isJustSwitched = true;
}

bool TakeJustSwitched()
{
    const bool wasJustSwitched = isJustSwitched;
    isJustSwitched = false;
    return wasJustSwitched;
}

}
