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
    // Subclasses override this - base just handles empty/no-op
}
