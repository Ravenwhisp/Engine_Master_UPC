#pragma once

#include <rapidjson/document.h>

class IFieldContainer;
class SceneReferenceResolver;

namespace FieldUtils
{
    void drawUi(IFieldContainer& container, char* base);
    void serialize(const IFieldContainer& container, const char* base, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void deserialize(IFieldContainer& container, char* base, const rapidjson::Value& fieldsJson);
    void clone(const IFieldContainer& source, const char* srcBase, IFieldContainer& target, char* dstBase);
    void fixReferences(IFieldContainer& container, char* base, const SceneReferenceResolver& resolver);
}
