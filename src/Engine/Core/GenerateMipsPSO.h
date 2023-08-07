#pragma once

#include <Memory/Descriptors/DescriptorAllocation.h>

class Device;
class PipelineStateObject;
class RootSignature;

struct alignas( 16 ) GenerateMipsCB
{
    u32          SrcMipLevel;   // Texture level of source mip
    u32          NumMipLevels;  // Number of OutMips to write: [1-4]
    u32          SrcDimension;  // Width and height of the source texture are even or odd.
    u32          IsSRGB;        // Must apply gamma correction to sRGB textures.
    DirectX::XMFLOAT2 TexelSize;     // 1.0 / OutMip1.Dimensions
};

// I don't use scoped enums (C++11) to avoid the explicit cast that is required to
// treat these as root indices.
namespace GenerateMips
{
enum
{
    GenerateMipsCB,
    SrcMip,
    OutMip,
    NumRootParameters
};
}

class GenerateMipsPSO
{
public:
    GenerateMipsPSO( Device& device );

    std::shared_ptr<RootSignature> GetRootSignature() const
    {
        return m_RootSignature;
    }

    std::shared_ptr<PipelineStateObject> GetPipelineState() const
    {
        return m_PipelineState;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
    {
        return m_DefaultUAV.GetDescriptorHandle();
    }

private:
    std::shared_ptr<RootSignature>       m_RootSignature;
    std::shared_ptr<PipelineStateObject> m_PipelineState;
    // Default (no resource) UAV's to pad the unused UAV descriptors.
    // If generating less than 4 mip map levels, the unused mip maps
    // need to be padded with default UAVs (to keep the DX12 runtime happy).
    DescriptorAllocation m_DefaultUAV;
};
