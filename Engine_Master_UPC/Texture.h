#pragma once
#include "Resources.h"
#include "ICacheable.h"


enum class TextureView : uint8_t
{
    None = 0,
    SRV = 1 << 0,
    RTV = 1 << 1,
    DSV = 1 << 2,
    UAV = 1 << 3,
};

inline TextureView operator|(TextureView a, TextureView b)
{
    return static_cast<TextureView>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline TextureView operator&(TextureView a, TextureView b)
{
    return static_cast<TextureView>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct TextureDesc
{
    DXGI_FORMAT             format{ DXGI_FORMAT_UNKNOWN };
    uint32_t                width{ 1 };
    uint32_t                height{ 1 };
    uint16_t                arraySize{ 1 };
    uint16_t                mipLevels{ 1 };
    TextureView             views{ TextureView::SRV };
    D3D12_RESOURCE_STATES   initialState{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
    D3D12_CLEAR_VALUE       clearValue{};
    bool                    hasClearValue{ false };

    DXGI_FORMAT             srvFormat{ DXGI_FORMAT_UNKNOWN };
    DXGI_FORMAT             rtvFormat{ DXGI_FORMAT_UNKNOWN };
    DXGI_FORMAT             dsvFormat{ DXGI_FORMAT_UNKNOWN };
    DXGI_FORMAT             uavFormat{ DXGI_FORMAT_UNKNOWN };
    bool                    shaderVisibleSRV{ false };

};


class Texture : public Resource, public ICacheable
{
public:
    constexpr static uint32_t MAX_MIPS{ 14 };

    Texture() = delete;
    explicit Texture(UID uid, ID3D12Device4& device, const TextureDesc& desc);
    explicit Texture(UID uid, ID3D12Device4& device, ComPtr<ID3D12Resource> existingResource, TextureView views, DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;


    DescriptorHandle    getSRV() const;
    DescriptorHandle    getRTV(uint32_t mip = 0)    const;
    DescriptorHandle    getDSV()                    const;
    DescriptorHandle    getUAV(uint32_t mip = 0)    const;


    bool        hasSRV()    const { return hasView(TextureView::SRV); }
    bool        hasRTV()    const { return hasView(TextureView::RTV); }
    bool        hasDSV()    const { return hasView(TextureView::DSV); }
    bool        hasUAV()    const { return hasView(TextureView::UAV); }

    uint32_t    mipCount()  const { return m_mipCount; }
    TextureDesc getDesc()   const { return m_desc; }


    bool resize(uint32_t newWidth, uint32_t newHeight);

    void release()
    {
        m_Resource.Reset();
    }

private:

    void allocateResource();
    void createViews();
    void releaseViews();

    void createSRV();
    void createRTV();
    void createDSV();
    void createUAV();

    bool hasView(TextureView v) const
    {
        return (m_desc.views & v) != TextureView::None;
    }


    DXGI_FORMAT resolvedSRVFormat() const;
    DXGI_FORMAT resolvedRTVFormat() const;
    DXGI_FORMAT resolvedDSVFormat() const;
    DXGI_FORMAT resolvedUAVFormat() const;


    TextureDesc         m_desc{};
    uint32_t            m_mipCount{ 0 };

    DescriptorHandle    m_srv{};
    DescriptorHandle    m_rtv[MAX_MIPS]{};
    DescriptorHandle    m_dsv{};
    DescriptorHandle    m_uav[MAX_MIPS]{};
};


using RenderTexture = Texture;
using DepthBuffer = Texture;