#include "Globals.h"
#include "ImporterScript.h"

Asset* ImporterScript::createAssetInstance(const MD5Hash& uid) const
{
    return new ScriptAsset(uid);
}

bool ImporterScript::importNative(const std::filesystem::path& path, ScriptAsset* dst)
{
    return false;
}

uint64_t ImporterScript::saveTyped(const ScriptAsset* source, uint8_t** outBuffer)
{
    return 0;
}

void ImporterScript::loadTyped(const uint8_t* buffer, ScriptAsset* dst)
{
}
