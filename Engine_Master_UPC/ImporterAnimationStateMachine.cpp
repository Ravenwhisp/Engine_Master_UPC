#include "Globals.h"
#include "ImporterAnimationStateMachine.h"
#include "JsonArchive.h"


bool ImporterAnimationStateMachine::saveNative(const AnimationStateMachineAsset* asset, const std::filesystem::path& path)
{
    JsonArchive archive(ArchiveMode::Output);
    const_cast<AnimationStateMachineAsset*>(asset)->serialize(archive);
    return archive.saveFile(path);
}

bool ImporterAnimationStateMachine::importNative(const std::filesystem::path& path, AnimationStateMachineAsset* dst)
{
    if (!dst)
        return false;

    JsonArchive archive(ArchiveMode::Input);
    if (!archive.loadFile(path))
    {
        DEBUG_ERROR("[ImporterAnimationStateMachine] Failed to load '%s'.", path.string().c_str());
        return false;
    }

    dst->serialize(archive);
    return true;
}
