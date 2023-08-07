#include <enginepch.h>
#include <Device.h>

#include "ConstantBuffer.h"
#include <Application.h>

ConstantBuffer::ConstantBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
    : Buffer(device, resource)
{
    m_SizeInBytes = GetD3D12ResourceDesc().Width;
}

ConstantBuffer::~ConstantBuffer() {}
//
//void ConstantBuffer::CreateViews(size_t numElements, size_t elementSize)
//{
//    m_SizeInBytes = numElements * elementSize;
//
//    D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc;
//    d3d12ConstantBufferViewDesc.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
//    d3d12ConstantBufferViewDesc.SizeInBytes = static_cast<UINT>(Math::AlignUp(m_SizeInBytes, 16));
//
//    auto device = Application::Get().GetDevice();
//
//    device->CreateConstantBufferView(&d3d12ConstantBufferViewDesc, m_ConstantBufferView.GetDescriptorHandle());
//}