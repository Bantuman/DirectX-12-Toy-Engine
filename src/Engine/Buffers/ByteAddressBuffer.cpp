#include "enginepch.h"

#include "ByteAddressBuffer.h"

#include <Device.h>

ByteAddressBuffer::ByteAddressBuffer( Device& device, const D3D12_RESOURCE_DESC& resDesc )
: Buffer( device, resDesc )
{}

ByteAddressBuffer::ByteAddressBuffer( Device& device, ComPtr<ID3D12Resource> resource )
: Buffer( device, resource )
{}

