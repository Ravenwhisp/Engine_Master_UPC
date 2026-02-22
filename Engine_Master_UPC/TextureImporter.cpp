#include "Globals.h"
#include "TextureImporter.h"
#include <Logger.h>

bool TextureImporter::loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out)
{
	const wchar_t* myPath = path.c_str();

	if (FAILED(LoadFromDDSFile(myPath, DDS_FLAGS_NONE, nullptr, out))) 
	{
		if (FAILED(LoadFromTGAFile(myPath, nullptr, out))) {
			if (FAILED(LoadFromWICFile(myPath, WIC_FLAGS_NONE, nullptr, out))) 
			{
                 LOG_WARNING("[TextureImporter] Failed to import the texture at the following path:", path.c_str());
				 return false;
			}
		}
	}
    return true;
}

void TextureImporter::importTyped(const DirectX::ScratchImage& source,TextureAsset* texture)
{
    if (source.GetImageCount() == 0)
    {
        LOG_ERROR("[TextureImporter] Couldn't import the image since it's count is 0.");
        return;
    }

    TexMetadata metaData = source.GetMetadata();
    if (metaData.dimension != TEX_DIMENSION_TEXTURE2D)
    {
        LOG_ERROR("[TextureImporter] Texture Importer riught now doesn't support 1D or 3D textures.");
        return;
    }

    ScratchImage mipImages;
    const DirectX::ScratchImage* finalSource = &source;

    if (metaData.mipLevels == 1 && (metaData.width > 1 || metaData.height > 1))
    {
        if (FAILED(GenerateMipMaps( source.GetImages(), source.GetImageCount(), metaData, TEX_FILTER_FANT | TEX_FILTER_SEPARATE_ALPHA, 0, mipImages)))
        {
            LOG("[TextureImporter] Failed to generate mipmaps for texture %d", texture->getId());
        }
        else
        {
            finalSource = &mipImages;
        }
    }

    const DirectX::Image* images = finalSource->GetImages();
    const TexMetadata& meta = finalSource->GetMetadata();

    const uint32_t mipCount = static_cast<uint32_t>(meta.mipLevels);
    const uint32_t arraySize = static_cast<uint32_t>(meta.arraySize);

    texture->images.clear();
    texture->images.reserve(meta.mipLevels * meta.arraySize);

    for (size_t item = 0; item < metaData.arraySize; ++item)
    {
        for (size_t level = 0; level < metaData.mipLevels; ++level)
        {
            const Image* subImg = source.GetImage(level, item, 0);

            TextureImage tImg;
            tImg.rowPitch = static_cast<uint32_t>(subImg->rowPitch);
            tImg.slicePitch = static_cast<uint32_t>(subImg->slicePitch);
            tImg.pixels.resize(subImg->slicePitch);

            std::memcpy(tImg.pixels.data(), subImg->pixels, subImg->slicePitch);

            texture->images.push_back(std::move(tImg));
        }
    }

    texture->width = metaData.width;
    texture->height = metaData.height;
    texture->mipCount = metaData.mipLevels;
    texture->arraySize = metaData.arraySize;
    texture->format = metaData.format;
    texture->imageCount = source.GetImageCount();
}

uint64_t TextureImporter::saveTyped(const TextureAsset* source, uint8_t** outBuffer)
{

    uint64_t size = 0;
    size += sizeof(uint64_t);
    size += 6 * sizeof(uint32_t);

    for (const auto& img : source->images) {
        size += 3 * sizeof(uint32_t);
        size += img.pixels.size();
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.u64(source->m_uid);

    writer.u32(source->width);
    writer.u32(source->height);
    writer.u32(source->mipCount);
    writer.u32(source->arraySize);
    writer.u32(source->imageCount);
    writer.u32(static_cast<uint32_t>(source->format));

    for (const auto& img : source->images) {
        writer.u32(img.rowPitch);
        writer.u32(img.slicePitch);
        writer.u32(static_cast<uint32_t>(img.pixels.size()));
        writer.bytes(img.pixels.data(), img.pixels.size());
    }

    *outBuffer = buffer;
    return size;
}

void TextureImporter::loadTyped(const uint8_t* buffer, TextureAsset* texture)
{
    BinaryReader reader(buffer);

    texture->m_uid = reader.u64();

    texture->width = reader.u32();
    texture->height = reader.u32();
    texture->mipCount = reader.u32();
    texture->arraySize = reader.u32();
    texture->imageCount = reader.u32();
    texture->format = static_cast<DXGI_FORMAT>(reader.u32());

    texture->images.clear();
    texture->images.reserve(texture->imageCount);

    for (uint32_t i = 0; i < texture->imageCount; ++i) {
        TextureImage img;
        img.rowPitch = reader.u32();
        img.slicePitch = reader.u32();

        uint32_t dataSize = reader.u32();
        img.pixels.resize(dataSize);
        reader.bytes(img.pixels.data(), dataSize);

        texture->images.push_back(std::move(img));
    }
}