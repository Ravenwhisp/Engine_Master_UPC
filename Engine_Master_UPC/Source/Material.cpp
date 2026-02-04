#include "Globals.h"
#include "Material.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "tiny_gltf.h"

namespace Emeika {
	void Material::Load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material,
		const char* basePath)
	{
		auto color = Vector3(float(material.baseColorFactor[0]), float(material.baseColorFactor[1]),
			float(material.baseColorFactor[2]));

		materialData.diffuseColour = color;
		materialData.specularColour = Vector3(0.1f, 0.1f, 0.1f);
		materialData.shininess = 32.0f;

		if (material.baseColorTexture.index >= 0)
		{
			const tinygltf::Texture& texture = model.textures[material.baseColorTexture.index];
			const tinygltf::Image& image = model.images[texture.source];
			if (!image.uri.empty())
			{
				_textureColor = app->GetResourcesModule()->CreateTexture2DFromFile(std::string(basePath) + image.uri, "Texture");
				materialData.hasDiffuseTex = true;
			}
		}
		else {
			_textureColor = app->GetResourcesModule()->CreateNullTexture2D();
			materialData.hasDiffuseTex = false;
		}

		materialBuffer = app->GetResourcesModule()->CreateDefaultBuffer(&materialData, alignUp(sizeof(MaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
	}
}

