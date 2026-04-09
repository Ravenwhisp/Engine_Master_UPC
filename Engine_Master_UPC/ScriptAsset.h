#pragma once
#include "Globals.h"
#include "Asset.h"

#include "TextureImage.h"

class ScriptAsset : public Asset
{
public:
	friend class ImporterScript;

	ScriptAsset() {}
	ScriptAsset(MD5Hash id) : Asset(id, AssetType::SCRIPT) {}
private:
};
