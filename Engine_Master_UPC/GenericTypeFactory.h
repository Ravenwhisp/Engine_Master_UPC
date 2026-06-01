#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

struct AssetReference;
class Script;
class GameObject;
class DataContainer;

template<typename TBase, typename... TCreateArgs>
class GenericTypeFactory
{
public:
	using Creator = std::unique_ptr<TBase>(*)(TCreateArgs...);

	struct Entry
	{
		std::string name;
		std::string displayName;
		Creator creator;
	};

	static void registerType(const std::string& name, const std::string& displayName, Creator creator)
	{
		for (auto& entry : m_registry)
		{
			if (entry.name == name)
			{
				entry.creator = creator;
				return;
			}
		}
		m_registry.push_back({ name, displayName, creator });
	}

	static std::unique_ptr<TBase> create(const std::string& name, TCreateArgs... args)
	{
		for (const auto& entry : m_registry)
		{
			if (entry.name == name)
			{
				return entry.creator(std::forward<TCreateArgs>(args)...);
			}
		}
		return nullptr;
	}

	static bool isRegistered(const std::string& name)
	{
		return std::any_of(m_registry.begin(), m_registry.end(),
			[&](const Entry& e) { return e.name == name; });
	}

	static const std::vector<Entry>& getAllRegistered()
	{
		return m_registry;
	}

private:
	static inline std::vector<Entry> m_registry;
};

using ScriptFactory = GenericTypeFactory<Script, GameObject*>;
using DataContainerFactory = GenericTypeFactory<DataContainer, AssetReference&>;


