#include "Globals.h"
#include "Material.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "tiny_gltf.h"

namespace Emeika {
	void Material::load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath)
	{
		Vector3 color = Vector3(float(material.baseColorFactor[0]), float(material.baseColorFactor[1]),
			float(material.baseColorFactor[2]));

		m_materialData.diffuseColour = color;
		m_materialData.specularColour = Vector3(0.1f, 0.1f, 0.1f);
		m_materialData.shininess = 32.0f;

		if (material.baseColorTexture.index >= 0)
		{
			const tinygltf::Texture& texture = model.textures[material.baseColorTexture.index];
			const tinygltf::Image& image = model.images[texture.source];
			if (!image.uri.empty())
			{
				m_textureColor = app->getResourcesModule()->createTexture2DFromFile(std::string(basePath) + image.uri, "Texture");
				m_materialData.hasDiffuseTex = true;
			}
		}
		else 
		{
			m_textureColor = app->getResourcesModule()->createNullTexture2D();
			m_materialData.hasDiffuseTex = false;
		}

		m_materialBuffer = app->getResourcesModule()->createDefaultBuffer(&m_materialData, alignUp(sizeof(MaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
	}
}

