#include <enginepch.h>
#include "MeshAssetHandler.h"

#include <Engine/Core/Application.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Buffers/Texture.h>
#include <Pipeline/ResourceStateTracker.h>

#include <Engine/Buffers/VertexBuffer.h>
#include <Engine/Buffers/IndexBuffer.h>
#include <Engine/Core/VertexTypes.h>
#include <Engine/Core/Material.h>

#include <Engine/Asset/TextureAssetHandler.h>
#include <Engine/Asset/MaterialAssetHandler.h>

#include <gltf\parser.hpp>
#include <gltf\types.hpp>
#include <gltf\tools.hpp>

Logger MeshAssetHandler::m_Logger;

inline std::vector<RefPtr<Mesh>> MeshAssetHandler::LoadImplStaticMesh(const fs::path& path, u8 flags)
{
    auto device = Application::Get().GetDevice();

    fastgltf::Parser parser;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadGLBBuffers |
        fastgltf::Options::LoadExternalBuffers;

    auto gltf = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
    if (parser.getError() != fastgltf::Error::None) {
        m_Logger->critical("fastgltf::loadGLTF failed to load {}", path.string());
    }

    if (gltf->parse() != fastgltf::Error::None) {
        m_Logger->critical("fastgltf::parse failed at parsing {}", path.string());
    }

    if (gltf->validate() != fastgltf::Error::None) {
        m_Logger->critical("fastgltf::validate failed on {}", path.string());
    }

    struct ImportBuffer
    {
        std::vector<u8>* bytes;
    };
    std::vector<ImportBuffer> buffers;

    std::unique_ptr<fastgltf::Asset> asset = gltf->getParsedAsset();
    for (auto& buffer : asset->buffers)
    {
        m_Logger->info("visiting buffer");
        std::visit(fastgltf::visitor{
            [](auto& arg) {}, // Covers FilePathWithOffset, BufferView, ... which are all not possible
            [&](fastgltf::sources::Vector& vector) {
                m_Logger->info("got buffer of type {} with size {}", (int)vector.mimeType, vector.bytes.size());
                buffers.emplace_back(&vector.bytes);
            },
            [&](fastgltf::sources::CustomBuffer&) {
                m_Logger->info("got custom buffer");
            },
            }, buffer.data);
    }

    std::vector<RefPtr<Texture>> textures;
    
    int i = 0;
    for (auto& image : asset->images)
    {
        std::visit(fastgltf::visitor{
           [](auto& arg) {},
           [&](fastgltf::sources::URI& filePath) {
              if (filePath.uri.isLocalPath())
              {
                   i++;
                   const std::string imagepath(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks C++.
                   textures.emplace_back(TextureAssetHandler::Get().GetOrLoad(i, path.parent_path() / fs::path(imagepath)));
               }
           },
            }, image.data);
    }



    auto& commandQueue = device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto  commandList = commandQueue.GetCommandList();

    std::vector<RefPtr<Mesh>> meshes;

    for (auto& mesh : asset->meshes)
    {
        RefPtr<Mesh> result = MakeRef<Mesh>();
        result->m_Materials.resize(asset->materials.size());

        m_Logger->info("Found mesh {}", mesh.name);

        result->m_Primitives.resize(mesh.primitives.size());
        bool failedImport = false;

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it)
        {
            auto positionIt = it->findAttribute("POSITION");
            if (positionIt == it->attributes.end())
                continue;

            auto normalIt = it->findAttribute("NORMAL");
            if (normalIt == it->attributes.end())
            {
                m_Logger->critical("We only support geometry with precalculated normals for now {}", path.string());
                continue;
            }

            if (!it->indicesAccessor.has_value()) {
                m_Logger->critical("We only support indexed geometry {}", path.string());
                failedImport = true;
                break;
            }

            // Get the output primitive
            auto primitiveIndex = std::distance(mesh.primitives.begin(), it);
            auto& primitive = result->m_Primitives[primitiveIndex];

            primitive.m_MaterialIndex = it->materialIndex.value_or(~0);

            if (primitive.m_MaterialIndex != ~0)
            {
                auto& material = asset->materials[primitive.m_MaterialIndex];
                auto& outMaterial = result->m_Materials[primitive.m_MaterialIndex];
                if (outMaterial == nullptr)
                {
                    outMaterial = MakeRef<Material>(Material::WhiteDiffuse);
                }
                auto properties = outMaterial->GetMaterialProperties();

                properties.Reflectance = XMFLOAT4(material.pbrData->metallicFactor, material.pbrData->metallicFactor, material.pbrData->metallicFactor, material.pbrData->metallicFactor);

                auto AssignTexture = [&outMaterial, &textures, &asset](Material::TextureType type, std::optional<fastgltf::TextureInfo>& texture) {
                    if (texture.has_value())
                    {
                        auto imageIndex = asset->textures[texture.value().textureIndex].imageIndex.value();
                        outMaterial->SetTexture(type, textures[imageIndex]);
                    }
                    else
                    {
                        outMaterial->SetTexture(type, TextureAssetHandler::Get().GetDefaultTexture());
                    }
                };

                if (material.pbrData.has_value())
                {
                    AssignTexture(Material::TextureType::Diffuse, material.pbrData.value().baseColorTexture);
                    AssignTexture(Material::TextureType::Normal, material.normalTexture);
                    AssignTexture(Material::TextureType::Emissive, material.emissiveTexture);
                }

                outMaterial->SetMaterialProperties(properties);
            }
            else
            {
                result->m_Materials[primitive.m_MaterialIndex] = MaterialAssetHandler::Get().GetDefaultMaterial();
            }

            auto& positionAccessor = asset->accessors[positionIt->second];
            if (!positionAccessor.bufferViewIndex.has_value())
                continue;

            auto& positionView = asset->bufferViews[positionAccessor.bufferViewIndex.value()];

            auto& normalAccessor = asset->accessors[normalIt->second];
            if (!normalAccessor.bufferViewIndex.has_value())
                continue;

            auto& normalView = asset->bufferViews[normalAccessor.bufferViewIndex.value()];

            using VertexCollection = std::vector<VertexPositionNormalTangentBitangentTexture>;
            using IndexCollection = std::vector<u32>;

            u32 vertexCount = positionAccessor.count;
            u32 positionStride = positionView.byteStride.value_or(fastgltf::getElementByteSize(positionAccessor.type, positionAccessor.componentType));
            u32 normalStride = normalView.byteStride.value_or(fastgltf::getElementByteSize(normalAccessor.type, normalAccessor.componentType));

            // Optional datafields
            u32 texcoordStride = 0;
            u32 tangentStride = 0;

            VertexCollection vertices;
            size_t positionOffset = positionView.byteOffset + positionAccessor.byteOffset;
            size_t normalOffset = normalView.byteOffset + normalAccessor.byteOffset;

            // Optional datafields
            size_t texcoordOffset = 0;
            size_t texcoordBufferIndex = positionView.bufferIndex;

            size_t tangentOffset = 0;
            size_t tangentBufferIndex = positionView.bufferIndex;

            auto texcoordIt = it->findAttribute("TEXCOORD_0");
            if (texcoordIt != it->attributes.end())
            {
                auto& texcoordAccessor = asset->accessors[texcoordIt->second];
                if (texcoordAccessor.bufferViewIndex.has_value())
                {
                    auto& texcoordView = asset->bufferViews[texcoordAccessor.bufferViewIndex.value()];
                    texcoordOffset = texcoordView.byteOffset + texcoordAccessor.byteOffset;
                    texcoordStride = texcoordView.byteStride.value_or(fastgltf::getElementByteSize(texcoordAccessor.type, texcoordAccessor.componentType));
                    texcoordBufferIndex = texcoordView.bufferIndex;
                }
            }
            else
            {
                m_Logger->warn("No TEXCOORD_0 attribute for {} is supported but weird, will default to (0, 0)", path.string());
            }

            auto tangentIt = it->findAttribute("TANGENT");
            if (tangentIt != it->attributes.end())
            {
                auto& tangentAccessor = asset->accessors[tangentIt->second];
                if (tangentAccessor.bufferViewIndex.has_value())
                {
                    auto& tangentView = asset->bufferViews[tangentAccessor.bufferViewIndex.value()];
                    tangentOffset = tangentView.byteOffset + tangentAccessor.byteOffset;
                    tangentStride = tangentView.byteStride.value_or(fastgltf::getElementByteSize(tangentAccessor.type, tangentAccessor.componentType));
                    tangentBufferIndex = tangentView.bufferIndex;
                }
            }
            else
            {
                m_Logger->warn("No TANGENT attribute for {} is supported but weird, will default to (0, 0, 0)", path.string());
            }

            for (size_t vi = 0; vi < vertexCount; ++vi)
            {
                XMFLOAT3 position = XMFLOAT3(reinterpret_cast<float*>(buffers[positionView.bufferIndex].bytes->data() + positionOffset + (vi * positionStride)));
                XMFLOAT3 normal = XMFLOAT3(reinterpret_cast<float*>(buffers[normalView.bufferIndex].bytes->data() + normalOffset + (vi * normalStride)));

                XMFLOAT2 texcoords = XMFLOAT2{ 0, 0 };
                if (texcoordStride)
                {
                    texcoords = XMFLOAT2(reinterpret_cast<float*>(buffers[texcoordBufferIndex].bytes->data() + texcoordOffset + (vi * texcoordStride)));
                }

                XMFLOAT3 tangent = XMFLOAT3{ 0, 0, 0 };
                if (tangentStride)
                {
                    tangent = XMFLOAT3(reinterpret_cast<float*>(buffers[tangentBufferIndex].bytes->data() + tangentOffset + (vi * tangentStride)));
                }

                XMFLOAT3 bitangent = XMFLOAT3{ 0, 0, 0 };
                XMStoreFloat3(&bitangent, XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&tangent)));

                vertices.emplace_back(position, normal, XMFLOAT3{ texcoords.x, texcoords.y, 0 }, tangent, bitangent);
            }

            auto& indexAccessor = asset->accessors[it->indicesAccessor.value()];
            if (!indexAccessor.bufferViewIndex.has_value())
            {
                m_Logger->critical("Late: we only support indexed geometry {}", path.string());
                failedImport = true;
                break;
            }

            primitive.m_VertexBuffer = commandList->CopyVertexBuffer(vertices);
            primitive.m_VertexContainer = std::move(vertices);
            u32 indexCount = static_cast<u32>(indexAccessor.count);
            auto& indexView = asset->bufferViews[indexAccessor.bufferViewIndex.value()];
            size_t indexOffset = indexView.byteOffset + indexAccessor.byteOffset;
            u32 indexStride = indexView.byteStride.value_or(fastgltf::getElementByteSize(indexAccessor.type, indexAccessor.componentType));

            IndexCollection indices(indexCount);
            for (size_t i = 0; i < indexCount; ++i)
            {
                u16* index = reinterpret_cast<u16*>(buffers[indexView.bufferIndex].bytes->data() + indexOffset + (i * indexStride));
                indices[i] = *index;
            }
            primitive.m_IndexBuffer = commandList->CopyIndexBuffer(indices);
            primitive.m_IndexContainer = std::move(indices);

        }
        if (failedImport)
        {
            result.reset();
        }
        else
        {
            meshes.push_back(result);
        }
    }

    commandQueue.ExecuteCommandList(commandList);

    return meshes;
}

inline RefPtr<Mesh> MeshAssetHandler::LoadImplSkinnedMesh(const fs::path& path, u8 flags)
{
    return RefPtr<Mesh>();
}

void MeshAssetHandler::Initialize()
{
    m_Logger = Application::Get().CreateLogger("MeshAssetHandler");
}

MeshAssetHandler& MeshAssetHandler::Get()
{
    assert(m_sMeshAssetHandlerSingleton);
    return *m_sMeshAssetHandlerSingleton;
}

RefPtr<Mesh> MeshAssetHandler::TryGet(u64 hash)
{
    return AssetHandler<Mesh>::TryGet(hash);
}

RefPtr<Mesh> MeshAssetHandler::GetOrLoad(u64 hash, const fs::path& path, u8 flags)
{
    return AssetHandler<Mesh>::InternalGetOrLoad(hash, [&path, flags]() -> RefPtr<Mesh> {
        if (flags & ML_SKINNED)
        {
            assert(false && "Skinned meshes are currently unimplemented");
            //LoadImpl3DTexture(path, flags & TL_SRGB_SPACE);
            return nullptr;
        }
        auto mesh = LoadImplStaticMesh(path, flags);
        if (mesh.empty())
        {
            m_Logger->critical("GetOrLoad: Failed to load mesh {} with flags {}", path.string(), flags);
        }
        return mesh.front();
    });
}

RefPtr<Scene> MeshAssetHandler::LoadScene(const fs::path& path, u8 flags)
{
    auto meshes = LoadImplStaticMesh(path, flags);
    if (meshes.empty())
    {
        m_Logger->critical("GetOrLoad: Failed to load mesh {} with flags {}", path.string(), flags);
    }

    auto scene = MakeRef<Scene>();

 /*   for (auto& mesh : meshes)
    {
    }*/

    return scene;
}
