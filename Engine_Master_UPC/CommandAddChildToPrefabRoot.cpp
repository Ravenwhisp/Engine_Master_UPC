#include "Globals.h"
#include "CommandAddChildToPrefabRoot.h"

CommandAddChildToPrefabRoot::CommandAddChildToPrefabRoot(Scene* targetScene, GameObject* parent)
{
}

void CommandAddChildToPrefabRoot::run()
{
}

GameObject* CommandAddChildToPrefabRoot::getResult() const
{
    return m_result;
}
