#pragma once

class Device;

class RootSignature
{
public:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> GetD3D12RootSignature() const
    {
        return m_RootSignature;
    }

    const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const
    {
        return m_RootSignatureDesc;
    }

    u32 GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
    u32 GetNumDescriptors(u32 rootIndex) const;

    friend struct std::default_delete<RootSignature>;

    RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

    virtual ~RootSignature();

private:
    void Destroy();
    void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

    Device& m_Device;
    D3D12_ROOT_SIGNATURE_DESC1                  m_RootSignatureDesc;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

    // Need to know the number of descriptors per descriptor table.
    // A maximum of 32 descriptor tables are supported (since a 32-bit
    // mask is used to represent the descriptor tables in the root signature.
    u32 m_NumDescriptorsPerTable[32];

    // A bit mask that represents the root parameter indices that are
    // descriptor tables for Samplers.
    u32 m_SamplerTableBitMask;
    // A bit mask that represents the root parameter indices that are
    // CBV, UAV, and SRV descriptor tables.
    u32 m_DescriptorTableBitMask;
};