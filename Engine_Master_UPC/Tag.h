#pragma once

#define TAG_LIST \
    X(DEFAULT)   \
    X(PLAYER)    \
    X(ENEMY)     \
    X(PROJECTILE)

enum class Tag {
#define X(name) name,
    TAG_LIST
#undef X
    COUNT
};

inline const char* TagToString(Tag tag)
{
    switch (tag)
    {
#define X(name) case Tag::name: return #name;
        TAG_LIST
#undef X
    default:
        return "Unknown";
    }
}

inline Tag StringToTag(const char* s)
{
#define X(name) if (strcmp(s, #name) == 0) return Tag::name;
    TAG_LIST
#undef X
        return Tag::DEFAULT;
}
