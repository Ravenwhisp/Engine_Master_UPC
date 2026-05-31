#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

class AssetReference;
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

template<typename TBase, typename TDerived, typename... TCreateArgs>
class GenericAutoRegister
{
public:
	GenericAutoRegister(const char* name, const char* displayName)
	{
		GenericTypeFactory<TBase, TCreateArgs...>::registerType(name, displayName, createFunc);
	}

private:
	static std::unique_ptr<TBase> createFunc(TCreateArgs... args)
	{
		return std::make_unique<TDerived>(std::forward<TCreateArgs>(args)...);
	}
};

using ScriptFactory = GenericTypeFactory<Script, GameObject*>;
using DataContainerFactory = GenericTypeFactory<DataContainer, AssetReference&>;

#define DECLARE_SCRIPT(ScriptType)
#define DECLARE_DATACONTAINER(TypeName)

#define IMPLEMENT_SCRIPT(ScriptType) \
	static GenericAutoRegister<Script, ScriptType, GameObject*> \
		s_autoRegister_##ScriptType(#ScriptType, #ScriptType);

#define IMPLEMENT_DATACONTAINER(TypeName) \
	static GenericAutoRegister<DataContainer, TypeName, AssetReference&> \
		s_autoRegister_##TypeName(#TypeName, #TypeName);
