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
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value json = getJSON(doc);
        doc.Swap(json);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        std::string jsonStr = buffer.GetString();
        archive.serialize(jsonStr);
    }
    else
    {
        std::string jsonStr;
        archive.serialize(jsonStr);
        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());
        if (!doc.HasParseError())
        {
            deserializeJSON(doc);
        }
    }
}
