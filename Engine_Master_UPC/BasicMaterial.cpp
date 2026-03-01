#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ResourcesModule.h"
#include "tiny_gltf.h"

bool BasicMaterial::load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath)
{
	Vector3 color = Vector3(float(material.baseColorFactor[0]), float(material.baseColorFactor[1]), float(material.baseColorFactor[2]));

	m_materialData.diffuseColour = color;
	m_materialData.specularColour = Vector3(0.1f, 0.1f, 0.1f);
	m_materialData.shininess = 32.0f;

	if (material.baseColorTexture.index >= 0)
	{
		const tinygltf::Texture& texture = model.textures[material.baseColorTexture.index];
		const tinygltf::Image& image = model.images[texture.source];
		std::string texturePath = std::string(basePath) + image.uri;
		if (!image.uri.empty() && app->getResourcesModule())
		{
			// We also need to check if a different material (gltf file) is trying to load this same texture
			if (app->getResourcesModule()->isTextureLoaded(texturePath) && !app->getResourcesModule()->getLoadedTexture(texturePath).expired())
			{
				std::shared_ptr<Texture> loadedTexture = app->getResourcesModule()->getLoadedTexture(texturePath).lock();
				// Since m_loadedTextures has a weak_ptr, it can have expired references, so we need to check if there's actually data inside
				if (loadedTexture)
				{
					std::string message = std::string("Texture ") + texturePath + std::string(" already loaded in memory, not loading it from disk.");
					DEBUG_LOG(message.c_str());
					m_textureColor = loadedTexture;
					m_materialData.hasDiffuseTex = true;
				}
				else
				{
					std::string error = std::string("Something is very wrong. Expected texture ") + texturePath + std::string(" to be expired, but it's not, and lock() returned nullptr...");
					DEBUG_LOG(error.c_str());
				}
			}
			else
			{
				app->getResourcesModule()->markTextureAsNotLoaded(texturePath);
				m_textureColor = app->getResourcesModule()->createTexture2DFromFile(texturePath);
				if (!m_textureColor)
				{
					return false;
				}
				m_materialData.hasDiffuseTex = true;
				app->getResourcesModule()->markTextureAsLoaded(texturePath, m_textureColor);
			}
		}
	}
	else
	{
		m_textureColor = app->getResourcesModule()->createNullTexture2D();
		m_materialData.hasDiffuseTex = false;
	}

	m_materialBuffer = app->getResourcesModule()->createDefaultBuffer(&m_materialData, alignUp(sizeof(MaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");

	return true;
}