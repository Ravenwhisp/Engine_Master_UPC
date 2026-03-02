#include "Globals.h"
#include "FontImporter.h"

namespace fs = std::filesystem;


bool FontImporter::loadExternal(const fs::path& path, std::vector<uint8_t>& out)
{
    const fs::path spritefontPath = runMakeSpriteFont(path);
    if (spritefontPath.empty())
    {
        return false;
    }

    std::ifstream file(spritefontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        DEBUG_ERROR("FontImporter: cannot open generated .spritefont at", spritefontPath.c_str());
        return false;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    out.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(out.data()), size))
    {
        DEBUG_ERROR("FontImporter: failed to read .spritefont data");
        return false;
    }

    std::error_code ec;
    fs::remove(spritefontPath, ec);

    return true;
}



void FontImporter::importTyped(const std::vector<uint8_t>& source, FontAsset* dst)
{
    dst->spriteFontData = source;
    dst->fontFamilyName = m_lastFontFamilyName;
}

uint64_t FontImporter::saveTyped(const FontAsset* source, uint8_t** buffer)
{


    const uint64_t totalSize =
        sizeof(uint64_t) +
        sizeof(uint32_t) + source->fontFamilyName.size() +
        sizeof(uint64_t) +  
        source->spriteFontData.size();

    *buffer = new uint8_t[totalSize];
    BinaryWriter writer(*buffer);

    writer.u64(source->m_uid);
    writer.string(source->fontFamilyName);
    writer.u64(static_cast<uint64_t>(source->spriteFontData.size()));
    writer.bytes(source->spriteFontData.data(), source->spriteFontData.size());

    return totalSize;
}


void FontImporter::loadTyped(const uint8_t* buffer, FontAsset* dst)
{
    BinaryReader reader(buffer);

    dst->m_uid = reader.u64();
    dst->fontFamilyName = reader.string();

    const uint64_t dataSize = reader.u64();
    dst->spriteFontData.resize(static_cast<size_t>(dataSize));
    reader.bytes(dst->spriteFontData.data(), dataSize);
}


fs::path FontImporter::runMakeSpriteFont(const fs::path& ttfPath)
{
    const fs::path outPath = ttfPath.parent_path() / (ttfPath.stem().string() + "_temp.spritefont");

    m_lastFontFamilyName = ttfPath.stem().string();

    wchar_t exeBuffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
    const fs::path makeSpriteFont = fs::path(exeBuffer).parent_path() / "MakeSpriteFont.exe";

    if (!fs::exists(makeSpriteFont))
    {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe not found at", makeSpriteFont.string().c_str());
        return {};
    }

    std::ostringstream cmd;
    cmd << '"' << makeSpriteFont.string() << '"'
        << " \"" << ttfPath.string() << '"'
        << " \"" << outPath.string() << '"';

    DEBUG_LOG("FontImporter: running:", cmd.str().c_str());

    const int result = std::system(cmd.str().c_str());
    if (result != 0)
    {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe exited with code", result);
        return {};
    }

    if (!fs::exists(outPath))
    {
        DEBUG_ERROR("FontImporter: MakeSpriteFont.exe did not produce output at",
            outPath.c_str());
        return {};
    }

    return outPath;
}