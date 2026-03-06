#include "Globals.h"
#include "NavMeshGeometryExtractor.h"

#include "SceneModule.h"
#include "GameObject.h"
#include "ModelComponent.h"
#include "BasicMesh.h"

bool NavMeshGeometryExtractor::Extract(SceneModule& scene, TriangleSoup& out)
{
	out.vertices.clear();
	out.indices.clear();

	int vertexOffset = 0;

	const std::vector<GameObject*>& gameObjects = scene.getAllGameObjects();

	for (GameObject* object : gameObjects)
	{
		if (!object)
			continue;

		/*if (!object->GetStatic())
			continue;*/
		if (object->GetTag() != Tag::NAVIGATION)
			continue;

		Component* comp = object->GetComponent(ComponentType::MODEL);
		if (!comp)
			continue;

		ModelComponent* model = static_cast<ModelComponent*>(comp);
		if (!model)
			continue;

		const std::vector<BasicMesh*>& meshes = model->getMeshes();

		for (BasicMesh* mesh : meshes)
		{
			if (!mesh)
				continue;

			const std::vector<Vector3>& positions = mesh->getPositionsCPU();
			const std::vector<uint32_t>& indices = mesh->getIndicesCPU();

			if (positions.empty() || indices.empty())
				continue;

			Matrix worldMatrix = object->GetTransform()->getGlobalMatrix();

			for (const Vector3& p : positions)
			{
				Vector3 world = Vector3::Transform(p, worldMatrix);

				out.vertices.push_back(world.x);
				out.vertices.push_back(world.y);
				out.vertices.push_back(world.z);
			}

			for (uint32_t i = 0; i < indices.size(); i += 3)
			{
				out.indices.push_back(vertexOffset + indices[i]);
				out.indices.push_back(vertexOffset + indices[i + 1]);
				out.indices.push_back(vertexOffset + indices[i + 2]);
			}

			vertexOffset += (int)positions.size();
		}
	}

	return !out.vertices.empty();
}