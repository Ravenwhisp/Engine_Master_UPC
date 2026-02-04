#include "Globals.h"
#include "Model.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

namespace Emeika {
    Model::~Model()
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
    void Model::load(const char* fileName, const char* basePath)
	{
		tinygltf::TinyGLTF gltfContext;
		tinygltf::Model model;
		std::string error, warning;
		bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
		if (loadOk)
		{
            for (tinygltf::Material material : model.materials) 
            {
                Emeika::Material* myMaterial = new Emeika::Material;
                myMaterial->load(model, material.pbrMetallicRoughness, basePath);
                m_materials.push_back(myMaterial);
            }

            for (tinygltf::Mesh mesh : model.meshes) 
            {
                for (tinygltf::Primitive primitive : mesh.primitives) 
                {
                    Emeika::Mesh* myMesh = new Emeika::Mesh;
                    myMesh->load(model, mesh, primitive);
                    m_meshes.push_back(myMesh);
                }
            }
		}
        else
        {
            LOG("Error loading %s: %s", fileName, error.c_str());
        }
	}
}

