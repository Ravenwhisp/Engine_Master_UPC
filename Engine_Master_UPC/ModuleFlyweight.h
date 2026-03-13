#pragma once
#include "Module.h"
#include "WeakCache.h"
#include "ICacheable.h"

class BasicMesh;
class BasicMaterial;
class Texture;
class MeshAsset;
class MaterialAsset;
class TextureAsset;

// Responsible for reusing GPU representations of assets via the Flyweight pattern.
// Delegates raw GPU allocation to ResourcesModule — only concerns itself with
// cache lookups and shared ownership.
class ModuleFlyweight : public Module
{
public:
    bool init()    override;
    bool cleanUp() override;

    std::shared_ptr<Texture>       createTexture(const TextureAsset& textureAsset);
    std::shared_ptr<BasicMesh>     createMesh(const MeshAsset& meshAsset);
    std::shared_ptr<BasicMaterial> createMaterial(const MaterialAsset& materialAsset);

private:

    WeakCache<UID, ICacheable> m_resources;
};