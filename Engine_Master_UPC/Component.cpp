#include "Globals.h"
#include "Component.h"

#include "GameObject.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

Transform* Component::getTransform()
{
    return m_owner->GetTransform();
}

void Component::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Output)
    {
        archive.serialize(m_uuid, "UID");
        ComponentType type = getType();
        archive.serializeStringEnum(type, "ComponentType", ComponentTypeToStringU32, StringToComponentTypeU32);
    }

    bool active = isActive();
    archive.serialize(active, "Active");
    if (archive.mode() == ArchiveMode::Input)
        setActive(active);
}
