#pragma once

class IFieldContainer;
class IArchive;
class SceneReferenceResolver;

namespace FieldUtils
{
    void drawUi(IFieldContainer& container, char* base);
    void serialize(IFieldContainer& container, char* base, IArchive& archive);
    void clone(const IFieldContainer& source, const char* srcBase, IFieldContainer& target, char* dstBase);
    void fixReferences(IFieldContainer& container, char* base, const SceneReferenceResolver& resolver);
}
