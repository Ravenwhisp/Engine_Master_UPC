#pragma once
#include "AssetType.h"

struct AssetDialogFilter
{
    const char* filterSpec;
    const char* defaultExtension;
};

inline AssetDialogFilter getDialogFilter(AssetType type)
{
    switch (type)
    {
    case AssetType::ANIMATION_STATE_MACHINE:
        return { "Scene Files (*.statemachine)\0*.statemachine\0All Files\0*.*\0",    "statemachine" };
    case AssetType::MATERIAL:
        return { "Material Files (*.mat)\0*.mat\0All Files\0*.*\0",     "mat" };
    case AssetType::ANIMATION:
        return { "Animation Files (*.anim)\0*.anim\0All Files\0*.*\0",  "anim" };
    case AssetType::SCENE:
        return { "Scene Files (*.scene)\0*.scene\0All Files\0*.*\0",    "scene" };
    default:
        return { "Asset Files (*.asset)\0*.asset\0All Files\0*.*\0",    "asset" };
    }
}