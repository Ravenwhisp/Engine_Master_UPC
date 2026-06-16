#pragma once

#define TAG_LIST(X) \
    X(DEFAULT)   \
    X(PLAYER)    \
    X(ENEMY)     \
    X(PROJECTILE)\
    X(NAVIGATION)\
    X(BREAKABLE)

#define TAG_ENUM(name) name,
#define TAG_SWITCH(name) case Tag::name: return #name;
#define TAG_IF(name) if (std::strcmp(s, #name) == 0) return Tag::name;

enum class Tag {
    TAG_LIST(TAG_ENUM)
    COUNT
};

inline const char* TagToString(Tag tag)
{
    switch (tag)
    {
        TAG_LIST(TAG_SWITCH)
    default:
        return "Unknown";
    }
}

inline Tag StringToTag(const char* s)
{
    TAG_LIST(TAG_IF)
    return Tag::DEFAULT;
}
