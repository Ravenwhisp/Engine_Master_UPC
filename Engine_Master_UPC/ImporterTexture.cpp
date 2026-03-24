#include "Globals.h"
#include "ImporterTexture.h"

#include <WindowLogger.h>

#include "BinaryReader.h"
#include "BinaryWriter.h"

using namespace DirectX;

bool ImporterTexture::loadExternal(const std::filesystem::path& path, ScratchImage& out)
{
    const wchar_t* widePath = path.c_str();

    if (SUCCEEDED(LoadFromDDSFile(widePath, DDS_FLAGS_NONE, nullptr, out)))
        return true;

    if (SUCCEEDED(LoadFromTGAFile(widePath, nullptr, out)))
        return true;

    if (SUCCEEDED(LoadFromWICFile(widePath, WIC_FLAGS_NONE, nullptr, out)))
        return true;

    DEBUG_ERROR("[ImporterTexture] Failed to load texture from '%s'.", path.string().c_str());
    return false;
}


void ImporterTexture::importTyped(const ScratchImage& source, TextureAsset* texture)
{
    if (source.GetImageCount() == 0)
    {
        DEBUG_ERROR("[ImporterTexture] Image count is 0 — nothing to import.");
        return;
    }

    TexMetadata meta = source.GetMetadata();

    if (meta.dimension != TEX_DIMENSION_TEXTURE2D)
    {
        DEBUG_ERROR("[ImporterTexture] Only 2-D textures are supported.");
        return;
    }

    //Step 1: decompress if the source is a block-compressed format
    ScratchImage decompressed;
    const ScratchImage* working = &source;

    if (IsCompressed(meta.format))
    {
        HRESULT hr = Decompress(
            source.GetImages(),
            source.GetImageCount(),
            meta,
            DXGI_FORMAT_R8G8B8A8_UNORM,    // decompress to a plain UNORM target
            decompressed
        );

        if (FAILED(hr))
        {
            DEBUG_ERROR("[ImporterTexture] Failed to decompress format %u (HRESULT: %08X).", static_cast<unsigned>(meta.format), static_cast<unsigned>(hr));
            return;
        }

        working = &decompressed;
        meta = decompressed.GetMetadata();
    }

    //Step 2: convert to the canonical engine format
    ScratchImage converted;
    if (meta.format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
    {
        HRESULT hr = Convert(
            working->GetImages(),
            working->GetImageCount(),
            meta,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            TEX_FILTER_DEFAULT,
            TEX_THRESHOLD_DEFAULT,
            converted
        );

        if (FAILED(hr))
        {
            DEBUG_ERROR("[ImporterTexture] Failed to convert format %u -> R8G8B8A8_UNORM_SRGB " "(HRESULT: %08X). Falling back to unconverted data.", static_cast<unsigned>(meta.format), static_cast<unsigned>(hr));
            // Fall back: keep working data as-is so we still get something usable.
        }
        else
        {
            working = &converted;
            meta = converted.GetMetadata();
        }
    }

    //Step 3: generate mip chain
    ScratchImage mipped;
    const ScratchImage* final = working;

    if (meta.mipLevels == 1 && meta.width > 1 && meta.height > 1)
    {
        HRESULT hr = GenerateMipMaps(
            working->GetImages(),
            working->GetImageCount(),
            meta,
            TEX_FILTER_FANT | TEX_FILTER_SEPARATE_ALPHA,
            0,          // 0 = full mip chain down to 1×1
            mipped
        );

        if (FAILED(hr))
        {
            DEBUG_WARN("[ImporterTexture] Failed to generate mipmaps (HRESULT: %08X) — " "importing without mips.", static_cast<unsigned>(hr));
        }
        else
        {
            final = &mipped;
            meta = mipped.GetMetadata();
        }
    }

    // Step 4: copy pixel data into the TextureAsset
    const uint32_t mipCount = static_cast<uint32_t>(meta.mipLevels);
    const uint32_t arraySize = static_cast<uint32_t>(meta.arraySize);

    texture->images.clear();
    texture->images.reserve(mipCount * arraySize);

    for (size_t item = 0; item < meta.arraySize; ++item)
    {
        for (size_t level = 0; level < meta.mipLevels; ++level)
        {
            const Image* sub = final->GetImage(level, item, 0);
            if (!sub)
            {
                DEBUG_ERROR("[ImporterTexture] GetImage failed at item %zu level %zu.", item, level);
                continue;
            }

            TextureImage tImg;
            tImg.rowPitch = static_cast<uint32_t>(sub->rowPitch);
            tImg.slicePitch = static_cast<uint32_t>(sub->slicePitch);
            tImg.pixels.resize(sub->slicePitch);
            std::memcpy(tImg.pixels.data(), sub->pixels, sub->slicePitch);

            texture->images.push_back(std::move(tImg));
        }
    }

    texture->width = static_cast<uint32_t>(meta.width);
    texture->height = static_cast<uint32_t>(meta.height);
    texture->mipCount = mipCount;
    texture->arraySize = arraySize;
    texture->imageCount = mipCount * arraySize;
    texture->format = meta.format;
}


uint64_t ImporterTexture::saveTyped(const TextureAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(uint32_t) + source->m_uid.size();    // uid string (NOT sizeof(uint64_t))
    size += 6 * sizeof(uint32_t);                        // width, height, mipCount, arraySize, imageCount, format

    for (const TextureImage& img : source->images)
    {
        size += 3 * sizeof(uint32_t);                    // rowPitch, slicePitch, dataSize
        size += img.pixels.size();
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.string(source->m_uid);

    writer.u32(source->width);
    writer.u32(source->height);
    writer.u32(source->mipCount);
    writer.u32(source->arraySize);
    writer.u32(source->imageCount);
    writer.u32(static_cast<uint32_t>(source->format));

    for (const TextureImage& img : source->images)
    {
        writer.u32(img.rowPitch);
        writer.u32(img.slicePitch);
        writer.u32(static_cast<uint32_t>(img.pixels.size()));
        writer.bytes(img.pixels.data(), img.pixels.size());
    }

    *outBuffer = buffer;
    return size;
}

void ImporterTexture::loadTyped(const uint8_t* buffer, TextureAsset* texture)
{
    BinaryReader reader(buffer);

    texture->m_uid = reader.string();

    texture->width = reader.u32();
    texture->height = reader.u32();
    texture->mipCount = reader.u32();
    texture->arraySize = reader.u32();
    texture->imageCount = reader.u32();
    texture->format = static_cast<DXGI_FORMAT>(reader.u32());

    texture->images.clear();
    texture->images.reserve(texture->imageCount);

    for (uint32_t i = 0; i < texture->imageCount; ++i)
    {
        TextureImage img;
        img.rowPitch = reader.u32();
        img.slicePitch = reader.u32();

        const uint32_t dataSize = reader.u32();
        img.pixels.resize(dataSize);
        reader.bytes(img.pixels.data(), dataSize);

        texture->images.push_back(std::move(img));
    }
}