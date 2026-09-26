#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include "AssetId.h"

#include <charconv>
#include <string>

struct PostProcessSettings : public ISerializable
{
    float exposure = 0.0f;

    // Bloom
    bool  bloomEnabled = false;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    float bloomClamp = 0.1f;   // max brightness a pixel may contribute to bloom

    // Colour grading 
    bool    lutEnabled = false;
    AssetId lutAsset;
    float   lutStrength = 1.0f;

    // Chromatic aberration
    bool  chromaticAberrationEnabled = false;
    float chromaticAberrationStrength = 1.0f;

    // Heartbeat / low-health "damage screen" effect.
    bool  heartbeatEnabled = false;
    float healthThreshold = 0.5f;   
    float health = 1.0f;     
    float separation = 0.0f; 

    // Death fade: desaturate to grey, then fade fully to black. 
    bool  deathFadeActive = false; 
    float deathGreyDuration = 1.5f; 
    float deathBlackDuration = 1.5f;

    bool  outlineEnabled = false;
    float outlineThickness = 1.5f;
    float outlineThreshold = 0.15f;
    float outlineIntensity = 0.6f;
    float outlineColorR = 0.05f;
    float outlineColorG = 0.04f;
    float outlineColorB = 0.05f;
    float outlineWobble = 1.0f;
    float outlineNoiseScale = 90.0f;
    float outlineBreakup = 0.5f;
    float outlineNormalThreshold = 0.35f;

    void serialize(IArchive& archive) override
    {
        archive.serialize(exposure, "exposure");

        archive.serialize(bloomEnabled, "bloomEnabled");
        archive.serialize(bloomThreshold, "bloomThreshold");
        archive.serialize(bloomIntensity, "bloomIntensity");
        archive.serialize(bloomClamp, "bloomClamp");

        archive.serialize(lutEnabled, "lutEnabled");

        // This remains one serialized string to preserve the binary layout of
        // scenes produced before LUTs became library assets. The value is an
        // opaque AssetId token, never a filesystem path.
        std::string serializedLutAsset;
        if (archive.mode() == ArchiveMode::Output && lutAsset.isValid())
        {
            serializedLutAsset = std::to_string(lutAsset.m_uid) + "|" + lutAsset.m_libId + "|" + std::to_string(static_cast<uint32_t>(lutAsset.m_type));
        }
        archive.serialize(serializedLutAsset, "lutAsset");
        if (archive.mode() == ArchiveMode::Input && !serializedLutAsset.empty())
        {
            const size_t firstSeparator = serializedLutAsset.find('|');
            const size_t secondSeparator = serializedLutAsset.find('|', firstSeparator == std::string::npos ? firstSeparator : firstSeparator + 1);
            if (firstSeparator != std::string::npos && secondSeparator != std::string::npos)
            {
                uint64_t uid = 0;
                uint32_t type = static_cast<uint32_t>(AssetType::UNKNOWN);
                const char* begin = serializedLutAsset.data();
                const char* end = begin + serializedLutAsset.size();
                const auto uidResult = std::from_chars(begin, begin + firstSeparator, uid);
                const auto typeResult = std::from_chars(begin + secondSeparator + 1, end, type);
                if (uidResult.ec == std::errc() && typeResult.ec == std::errc())
                {
                    lutAsset.m_uid = static_cast<UID>(uid);
                    lutAsset.m_libId = serializedLutAsset.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
                    lutAsset.m_type = static_cast<AssetType>(type);
                }
            }
        }
        archive.serialize(lutStrength, "lutStrength");

        archive.serialize(chromaticAberrationEnabled, "chromaticAberrationEnabled");
        archive.serialize(chromaticAberrationStrength, "chromaticAberrationStrength");

        archive.serialize(heartbeatEnabled, "heartbeatEnabled");
        archive.serialize(healthThreshold, "healthThreshold");

        archive.serialize(deathGreyDuration, "deathGreyDuration");
        archive.serialize(deathBlackDuration, "deathBlackDuration");

        archive.serialize(outlineEnabled, "outlineEnabled");
        archive.serialize(outlineThickness, "outlineThickness");
        archive.serialize(outlineThreshold, "outlineThreshold");
        archive.serialize(outlineIntensity, "outlineIntensity");
        archive.serialize(outlineColorR, "outlineColorR");
        archive.serialize(outlineColorG, "outlineColorG");
        archive.serialize(outlineColorB, "outlineColorB");
        archive.serialize(outlineWobble, "outlineWobble");
        archive.serialize(outlineNoiseScale, "outlineNoiseScale");
        archive.serialize(outlineBreakup, "outlineBreakup");
        archive.serialize(outlineNormalThreshold, "outlineNormalThreshold");
    }
};
