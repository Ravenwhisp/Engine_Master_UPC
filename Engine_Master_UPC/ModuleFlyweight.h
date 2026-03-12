#pragma once
#include "Module.h"
#include "UID.h"
#include "ICacheable.h"

class BasicMesh;
class BasicMaterial;
class Texture;
class MeshAsset;
class MaterialAsset;
class TextureAsset;

// Responsible for reusing GPU representations of assets via the flyweight pattern.
// Delegates raw GPU allocation to ResourcesModule — only concerns itself with
// cache lookups and shared ownership.
//
// Holds weak_ptrs — callers share ownership via shared_ptr.
// Resources are freed automatically when the last caller releases their shared_ptr.
class ModuleFlyweight : public Module
{
public:
	bool init()    override;
	bool cleanUp() override;

	std::shared_ptr<Texture>       createTexture(const TextureAsset& textureAsset);

	std::shared_ptr<BasicMesh>     createMesh(const MeshAsset& meshAsset);
	std::shared_ptr<BasicMaterial> createMaterial(const MaterialAsset& materialAsset);

private:

	// Returns a live shared_ptr if the resource is still alive, nullptr otherwise.
	// Prunes expired entries on miss to keep the map lean.
	template<typename T>
	std::shared_ptr<T> getResource(UID uid)
	{
		auto it = m_resources.find(uid);
		if (it == m_resources.end())
			return nullptr;

		auto shared = it->second.lock();
		if (!shared)
		{
			m_resources.erase(it);
			return nullptr;
		}
		return std::static_pointer_cast<T>(shared);
	}

	void registerResource(UID uid, std::shared_ptr<ICacheable> resource)
	{
		m_resources.emplace(uid, resource);
	}

	// weak_ptr map — no ownership, purely a lookup cache.
	// Entries expire naturally when callers release their shared_ptrs.
	std::unordered_map<UID, std::weak_ptr<ICacheable>> m_resources;
};