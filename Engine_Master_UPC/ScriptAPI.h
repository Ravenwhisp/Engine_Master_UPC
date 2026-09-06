#pragma once

#include <chrono>

#include "EngineAPI.h"
#include "Script.h"
#include "GameObject.h"
#include "Transform.h"

#include "GenericTypeFactory.h"
#include "FieldInfo.h"
#include "FieldMacros.h"
#include "ComponentRef.h"
#include "AssetReference.h"

#define DECLARE_SCRIPT(ScriptType)

#define IMPLEMENT_SCRIPT(ScriptType) \
	namespace { \
		static std::unique_ptr<Script> s_create_##ScriptType(GameObject* owner) { \
			return std::make_unique<ScriptType>(owner); \
		} \
		static bool s_registered_##ScriptType = ( \
			::registerScript(#ScriptType, &s_create_##ScriptType), \
			true \
		); \
	}

namespace ScriptProfilerAPI
{
    // Scoped measurement usable directly from any Script subclass. The script
    // identity comes from ScriptComponent, so callers only provide a label.
    class Scope
    {
    public:
        Scope(const Script* script, const char* scopeName)
            : m_script(script), m_scopeName(scopeName), m_enabled(isEnabled())
        {
            if (m_enabled)
            {
                m_start = Clock::now();
            }
        }

        ~Scope()
        {
            if (!m_enabled || !m_script || !m_scopeName)
            {
                return;
            }

            const std::string& scriptName = m_script->getProfilerName();
            recordScope(
                scriptName.empty() ? "<unnamed>" : scriptName.c_str(),
                m_scopeName,
                m_script->getOwner(),
                std::chrono::duration<float, std::milli>(Clock::now() - m_start).count());
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        using Clock = std::chrono::steady_clock;

        const Script* m_script = nullptr;
        const char* m_scopeName = nullptr;
        bool m_enabled = false;
        Clock::time_point m_start{};
    };
}

#define SCRIPT_PROFILE_JOIN_INNER(left, right) left##right
#define SCRIPT_PROFILE_JOIN(left, right) SCRIPT_PROFILE_JOIN_INNER(left, right)
#define SCRIPT_PROFILE_SCOPE(scopeName) \
    ::ScriptProfilerAPI::Scope SCRIPT_PROFILE_JOIN(scriptProfileScope_, __LINE__)(this, scopeName)
