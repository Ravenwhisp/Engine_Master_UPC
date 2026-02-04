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
        for (int i = 0; i < _meshes.size(); i++) {
            delete _meshes[i];
            _meshes[i] = nullptr;
        }

        for (int i = 0; i < _materials.size(); i++) {
            delete _materials[i];
            _materials[i] = nullptr;
        }

    }
    void Model::Load(const char* fileName, const char* basePath)
	{
		tinygltf::TinyGLTF gltfContext;
		tinygltf::Model model;
		std::string error, warning;
		bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
		if (loadOk)
		{
            // First, load all materials for the entire model
            for (tinygltf::Material material : model.materials) {
                Emeika::Material* myMaterial = new Emeika::Material;
                myMaterial->Load(model, material.pbrMetallicRoughness, basePath);
                _materials.push_back(myMaterial);
            }

            // Then, load all meshes
            for (tinygltf::Mesh mesh : model.meshes) {
                // Create for each primitive a mesh
                for (tinygltf::Primitive primitive : mesh.primitives) {
                    Emeika::Mesh* myMesh = new Emeika::Mesh;
                    myMesh->Load(model, mesh, primitive);
                    _meshes.push_back(myMesh);
                }
            }
		}
		else LOG("Error loading %s: %s", fileName, error.c_str());

	}
}

