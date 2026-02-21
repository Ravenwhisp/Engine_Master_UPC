#include "Globals.h"
#include "BasicModel.h"



#include "Application.h"
#include "RenderModule.h"
#include "GameObject.h"
#include "Transform.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)


BasicModel::~BasicModel()
{
    for (int i = 0; i < m_meshes.size(); i++) 
    {
        delete m_meshes[i];
        m_meshes[i] = nullptr;
    }

    for (int i = 0; i < m_materials.size(); i++) 
    {
        delete m_materials[i];
        m_materials[i] = nullptr;
    }

}
void BasicModel::load(const char* fileName, const char* basePath)
{
    m_modelPath = fileName;
    m_basePath = basePath;

    m_hasBounds = false;

    m_boundingBox = Engine::BoundingBox(Vector3(FLT_MAX, FLT_MAX, FLT_MAX), Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX));

	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;
	bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
	if (loadOk)
	{
        for (tinygltf::Material material : model.materials) 
        {
            BasicMaterial* myMaterial = new BasicMaterial;
            myMaterial->load(model, material.pbrMetallicRoughness, basePath);
            m_materials.push_back(myMaterial);
        }

        for (tinygltf::Mesh mesh : model.meshes) 
        {
            for (tinygltf::Primitive primitive : mesh.primitives) 
            {
                BasicMesh* myMesh = new BasicMesh;
                myMesh->load(model, mesh, primitive);
                m_meshes.push_back(myMesh);

                if (myMesh->hasBounds())
                {
                    if (!m_hasBounds)
                    {
                        m_boundingBox.setMin(myMesh->getBoundsMin());
                        m_boundingBox.setMax(myMesh->getBoundsMax());
                        m_hasBounds = true;
                    }
                    else
                    {
                        const Vector3& mn = myMesh->getBoundsMin();
                        const Vector3& mx = myMesh->getBoundsMax();

                        Vector3 currentMin = m_boundingBox.getMin();
                        Vector3 currentMax = m_boundingBox.getMax();

                        Vector3 newMin(
							std::min(currentMin.x, mn.x),
							std::min(currentMin.y, mn.y),
							std::min(currentMin.z, mn.z)
						);

                        Vector3 newMax( 
                            std::max(currentMax.x, mx.x),
                            std::max(currentMax.y, mx.y),
                            std::max(currentMax.z, mx.z)
                        );

                        m_boundingBox.setMin(newMin);
                        m_boundingBox.setMax(newMax);
                    }
                }
            }
        }
	}
    else
    {
        LOG("Error loading %s: %s", fileName, error.c_str());
    }
}

rapidjson::Value BasicModel::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::MODEL), domTree.GetAllocator());

    return componentInfo;
}

