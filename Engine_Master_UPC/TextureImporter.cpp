#include "Globals.h"
#include "TextureImporter.h"

bool TextureImporter::loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out)
{
	const wchar_t* myPath = path.c_str();

	if (FAILED(LoadFromDDSFile(myPath, DDS_FLAGS_NONE, nullptr, out))) 
	{
		if (FAILED(LoadFromTGAFile(myPath, nullptr, out))) {
			if (FAILED(LoadFromWICFile(myPath, WIC_FLAGS_NONE, nullptr, out))) 
			{
				 return false;
			}
		}
	}
    return true;
}

void TextureImporter::importTyped(const DirectX::ScratchImage& source,TextureAsset* texture)
{
    if (source.GetImageCount() == 0) return;

    TexMetadata metaData = source.GetMetadata();
    if (metaData.dimension != TEX_DIMENSION_TEXTURE2D) return;

    ScratchImage mipImages;
    const DirectX::ScratchImage* finalSource = &source;

    if (metaData.mipLevels == 1 && (metaData.width > 1 || metaData.height > 1))
    {
        if (FAILED(GenerateMipMaps( source.GetImages(), source.GetImageCount(), metaData, TEX_FILTER_FANT | TEX_FILTER_SEPARATE_ALPHA, 0, mipImages)))
        {
            LOG("Failed to generate mipmaps for texture %d", texture->getId());
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

uint64_t TextureImporter::saveTyped(const TextureAsset* source, uint8_t** buffer)
{
    return 0;
}

void TextureImporter::loadTyped(const uint8_t* buffer, TextureAsset* texture)
{
}
