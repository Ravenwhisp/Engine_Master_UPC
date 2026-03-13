#include "Globals.h"
#include "ModuleFlyweight.h"

#include "Application.h"
#include "ResourcesModule.h"
#include "AssetsModule.h"

#include "Texture.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"

#include "ModelAsset.h"
#include "TextureAsset.h"

bool ModuleFlyweight::init()
{
    return true;
}

bool ModuleFlyweight::cleanUp()
{
    m_resources.clear();
    return true;
}

std::shared_ptr<Texture> ModuleFlyweight::createTexture(const TextureAsset& textureAsset)
{
    const UID uid = textureAsset.getId();

    if (auto cached = m_resources.getAs<Texture>(uid))
    {
        return cached;
    }


    auto texture = std::shared_ptr<Texture>(app->getResourcesModule()->createTexture(textureAsset));
    m_resources.insert(uid, texture);
    return texture;
}

std::shared_ptr<BasicMesh> ModuleFlyweight::createMesh(const MeshAsset& meshAsset)
{
    const UID uid = meshAsset.getId();

    if (auto cached = m_resources.getAs<BasicMesh>(uid))
    {
        return cached;
    }


    auto mesh = std::make_shared<BasicMesh>(uid, meshAsset);
    m_resources.insert(uid, mesh);
    return mesh;
}

std::shared_ptr<BasicMaterial> ModuleFlyweight::createMaterial(const MaterialAsset& materialAsset)
{
    const UID uid = materialAsset.getId();

    if (auto cached = m_resources.getAs<BasicMaterial>(uid))
    {
        return cached;
    }

    auto material = std::make_shared<BasicMaterial>(uid, materialAsset);
    m_resources.insert(uid, material);
    return material;
}