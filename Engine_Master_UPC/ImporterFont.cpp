#include "Globals.h"
#include "ImporterFont.h"

#include "Application.h"
#include "ModuleFileSystem.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <fstream>

namespace fs = std::filesystem;


bool ImporterFont::loadExternal(const std::filesystem::path& path, std::vector<uint8_t>& out)
{
    const std::filesystem::path spritefontPath = runMakeSpriteFont(path);
    if (spritefontPath.empty())
    {
        return false;
    }

    out = app->getModuleFileSystem()->read(spritefontPath);
    app->getModuleFileSystem()->remove(spritefontPath);

    return !out.empty();
}

void ImporterFont::importTyped(const std::vector<uint8_t>& source, FontAsset* dst)
{
    dst->spriteFontData = source;
    dst->fontFamilyName = m_lastFontFamilyName;
}

uint64_t ImporterFont::saveTyped(const FontAsset* source, uint8_t** buffer)
{
    const uint64_t totalSize =
        sizeof(uint64_t) +
        sizeof(uint32_t) + source->fontFamilyName.size() +
        sizeof(uint64_t) +  
        source->spriteFontData.size();

    *buffer = new uint8_t[totalSize];
    BinaryWriter writer(*buffer);

    writer.string(source->m_uid);
    writer.string(source->fontFamilyName);
    writer.u64(static_cast<uint64_t>(source->spriteFontData.size()));
    writer.bytes(source->spriteFontData.data(), source->spriteFontData.size());

    return totalSize;
}


void ImporterFont::loadTyped(const uint8_t* buffer, FontAsset* dst)
{
    BinaryReader reader(buffer);

    dst->m_uid = reader.string();
    dst->fontFamilyName = reader.string();

    const uint64_t dataSize = reader.u64();
    dst->spriteFontData.resize(static_cast<size_t>(dataSize));
    reader.bytes(dst->spriteFontData.data(), dataSize);
}

#include <cstdint>

static std::string readTTFFamilyName(const fs::path& ttfPath)
{
    std::ifstream f(ttfPath, std::ios::binary);
    if (!f) return {};

    auto readU16 = [&]() -> uint16_t {
        uint8_t b[2]; f.read((char*)b, 2);
        return (b[0] << 8) | b[1];
        };
    auto readU32 = [&]() -> uint32_t {
        uint8_t b[4]; f.read((char*)b, 4);
        return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
        };

    readU32();
    const uint16_t numTables = readU16();
    readU16(); readU16(); readU16();

    uint32_t nameOffset = 0;
    for (uint16_t i = 0; i < numTables; i++)
    {
        char tag[4]; f.read(tag, 4);
        readU32();
        const uint32_t offset = readU32();
        readU32();
        if (std::strncmp(tag, "name", 4) == 0) { nameOffset = offset; break; }
    }

    if (!nameOffset) return {};

    f.seekg(nameOffset);
    readU16();
    const uint16_t count = readU16();
    const uint16_t stringOffset = readU16();
    const uint32_t storageBase = nameOffset + stringOffset;

    for (uint16_t i = 0; i < count; i++)
    {
        const uint16_t platformID = readU16();
        const uint16_t encodingID = readU16();
        readU16();
        const uint16_t nameID = readU16();
        const uint16_t length = readU16();
        const uint16_t offset = readU16();

        if (nameID != 1) continue;

        const std::streampos cur = f.tellg();
        f.seekg(storageBase + offset);

        std::string result;
        if (platformID == 3 && encodingID == 1) // Windows UTF-16 BE
        {
            for (uint16_t j = 0; j < length / 2; j++)
                result += static_cast<char>(readU16() & 0x7F);
        }
        else if (platformID == 1) // Mac ASCII
        {
            result.resize(length);
            f.read(result.data(), length);
        }

        f.seekg(cur);
        if (!result.empty()) return result;
    }

    return {};
}

fs::path ImporterFont::runMakeSpriteFont(const fs::path& ttfPath)
{
    const fs::path outPath = ttfPath.parent_path() / (ttfPath.stem().string() + "_temp.spritefont");

    m_lastFontFamilyName = readTTFFamilyName(ttfPath);
    if (m_lastFontFamilyName.empty())
    {
        DEBUG_ERROR("FontImporter: could not read font family name from", ttfPath.string().c_str());
        return {};
    }

    wchar_t exeBuffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
    const fs::path makeSpriteFont = fs::path(exeBuffer).parent_path() / "MakeSpriteFont.exe";
    const fs::path makeSpriteFontDir = makeSpriteFont.parent_path();

    if (!fs::exists(makeSpriteFont))
    {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe not found at", makeSpriteFont.string().c_str());
        return {};
    }

    if (AddFontResourceExW(ttfPath.wstring().c_str(), 0, nullptr) == 0)
    {
        DEBUG_ERROR("FontImporter: failed to install font", ttfPath.string().c_str());
        return {};
    }
    SendNotifyMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

    std::ostringstream cmd;
    cmd << "cd /d \"" << makeSpriteFontDir.string() << "\""
        << " && \"" << makeSpriteFont.string() << "\""
        << " \"" << m_lastFontFamilyName << "\""
        << " \"" << outPath.string() << "\" 2>&1";

    FILE* pipe = _popen(cmd.str().c_str(), "r");
    std::ostringstream output;
    if (pipe)
    {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) output << buf;
    }
    else {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe failed:", output.str().c_str());
        return {};
    }
    const int result = _pclose(pipe);

    RemoveFontResourceExW(ttfPath.wstring().c_str(), 0, nullptr);
    SendNotifyMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

    if (result != 0)
    {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe failed:", output.str().c_str());
        return {};
    }

    return outPath;
}

Asset* ImporterFont::createAssetInstance(const MD5Hash& uid) const
{
    return new FontAsset(uid);
}