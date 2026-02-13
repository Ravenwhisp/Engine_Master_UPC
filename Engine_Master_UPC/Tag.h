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
