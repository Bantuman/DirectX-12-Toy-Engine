#include <enginepch.h>
#include "Renderer.h"

#include <Engine/Core/Device.h>
#include <Engine/Core/Scene.h>
#include <Engine/Core/Application.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/RootSignature.h>
#include <Engine/Pipeline/PipelineStateObject.h>
#include <Engine/Buffers/Texture.h>
#include <Engine/Core/RenderTarget.h>
#include <Engine/Core/Mesh.h>
#include <Engine/Core/Material.h>

#include <Buffers/IndexBuffer.h>
#include <Buffers/VertexBuffer.h>

#include <Engine/Asset/MaterialAssetHandler.h>

void Renderer::RenderIndexedMesh(CommandList& commandList, const Mesh& mesh, const XMMATRIX& transform)
{
    commandList.SetPrimitiveTopology(mesh.GetPrimitiveTopology());

    std::vector<XMMATRIX> instances = { transform };
    commandList.SetGraphicsDynamicStructuredBuffer(GenericRootParameters::InstancesSB, instances);

    for (size_t i = 0; i < mesh.m_Primitives.size(); ++i)
    {
        auto& primitive = mesh.m_Primitives[i];

        if (primitive.m_MaterialIndex != ~0u && !mesh.m_Materials.empty() && mesh.m_Materials[primitive.m_MaterialIndex])
        {
            auto material = mesh.m_Materials[primitive.m_MaterialIndex];
            commandList.SetGraphicsDynamicConstantBuffer(1, material->GetMaterialProperties());
            commandList.SetShaderResourceView(GenericRootParameters::Textures, 0, material->GetTexture(Material::TextureType::Diffuse), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        else
        {
            auto material = MaterialAssetHandler::Get().GetDefaultMaterial();
            commandList.SetGraphicsDynamicConstantBuffer(1, material->GetMaterialProperties());
            commandList.SetShaderResourceView(GenericRootParameters::Textures, 0, material->GetTexture(Material::TextureType::Diffuse), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        commandList.SetVertexBuffer(0, primitive.m_VertexBuffer);

        if (auto indexCount = primitive.m_IndexBuffer->GetNumIndices(); indexCount > 0)
        {
            commandList.SetIndexBuffer(primitive.m_IndexBuffer);
            commandList.DrawIndexed(indexCount, 1);
        }
        else if (auto vertexCount = primitive.m_VertexBuffer->GetNumVertices(); vertexCount > 0)
        {
            commandList.Draw(vertexCount);
        }
    }
}

void Renderer::RenderInstancedIndexedMesh(CommandList& commandList, const Mesh& mesh, const std::vector<XMMATRIX>& instances)
{
    commandList.SetPrimitiveTopology(mesh.GetPrimitiveTopology());
    commandList.SetGraphicsDynamicStructuredBuffer(GenericRootParameters::InstancesSB, instances);

    for (size_t i = 0; i < mesh.m_Primitives.size(); ++i)
    {
        auto& primitive = mesh.m_Primitives[i];

        if (primitive.m_MaterialIndex != ~0u && !mesh.m_Materials.empty() && mesh.m_Materials[primitive.m_MaterialIndex])
        {
            auto material = mesh.m_Materials[primitive.m_MaterialIndex];
            commandList.SetGraphicsDynamicConstantBuffer(GenericRootParameters::MaterialCB, material->GetMaterialProperties());
            commandList.SetShaderResourceView(GenericRootParameters::Textures, 0, material->GetTexture(Material::TextureType::Diffuse), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        else
        {
            auto material = MaterialAssetHandler::Get().GetDefaultMaterial();
            commandList.SetGraphicsDynamicConstantBuffer(GenericRootParameters::MaterialCB, material->GetMaterialProperties());
            commandList.SetShaderResourceView(GenericRootParameters::Textures, 0, material->GetTexture(Material::TextureType::Diffuse), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        commandList.SetVertexBuffer(0, primitive.m_VertexBuffer);

        if (auto indexCount = primitive.m_IndexBuffer->GetNumIndices(); indexCount > 0)
        {
            commandList.SetIndexBuffer(primitive.m_IndexBuffer);
            commandList.DrawIndexed(indexCount, instances.size());
        }
        else if (auto vertexCount = primitive.m_VertexBuffer->GetNumVertices(); vertexCount > 0)
        {
            commandList.Draw(vertexCount, instances.size());
        }
    }
}
