#include <enginepch.h>
#include "TextureAssetHandler.h"

#include <Engine/Core/Application.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Engine/Buffers/Texture.h>
#include <Pipeline/ResourceStateTracker.h>

Logger TextureAssetHandler::m_Logger;

inline RefPtr<Texture> TextureAssetHandler::LoadImpl2DTexture(const fs::path& path, u8 flags)
{
    auto device = Application::Get().GetDevice();
    std::shared_ptr<Texture> texture;
    if (!fs::exists(path))
    {
        m_Logger->critical("LoadImpl2DTexture: Couldn't find file {}", path.string());
        return nullptr;
    }

    //std::lock_guard<std::mutex> lock(m_TextureCacheMutex);
  /*  auto                        iter = m_TextureCache.find(path);
    if (iter != m_TextureCache.end())
    {
        texture = device->CreateTexture(iter->second);
    }*/
    //else
    {
        TexMetadata  metadata;
        ScratchImage scratchImage;

        if (path.extension() == ".dds")
        {
            ThrowIfFailed(LoadFromDDSFile(path.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratchImage));
        }
        else if (path.extension() == ".hdr")
        {
            ThrowIfFailed(LoadFromHDRFile(path.c_str(), &metadata, scratchImage));
        }
        else if (path.extension() == ".tga")
        {
            ThrowIfFailed(LoadFromTGAFile(path.c_str(), &metadata, scratchImage));
        }
        else
        {
            ThrowIfFailed(LoadFromWICFile(path.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratchImage));
        }

        // Force the texture format to be sRGB to conver12 t to linear when sampling the texture in a shader.
        if (flags & TL_FORCE_SRGB_SPACE)
        {
            metadata.format = MakeSRGB(metadata.format);
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE1D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE2D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE3D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.depth));
            break;
        default:
            throw std::exception("Invalid texture dimension.");
            break;
        }

        auto                                   d3d12Device = device->GetD3D12Device();
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        ThrowIfFailed(d3d12Device->CreateCommittedResource(
            ADDR(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), D3D12_HEAP_FLAG_NONE, &textureDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureResource)));

        texture = device->CreateTexture(textureResource);
        texture->SetName(path.filename());

        // Update the global state tracker.
        ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const Image* pImages = scratchImage.GetImages();
        for (int i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto& subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        auto& commandQueue = device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
        auto  commandList = commandQueue.GetCommandList();

        commandList->CopyTextureSubresource(texture, 0, static_cast<u32>(subresources.size()), subresources.data());

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            commandList->GenerateMips(texture);
        }

        commandQueue.ExecuteCommandList(commandList);

        // Add the texture resource to the texture cache.
        //m_TextureCache[path] = textureResource.Get();
    }
    m_Logger->info("Successfully loaded {}", path.string());
    return texture;
}

void TextureAssetHandler::Initialize()
{
	m_Logger = Application::Get().CreateLogger("TextureAssetHandler");

    m_pDefaultTexture = GetOrLoad("DefaultTexture"_hs, "Resource/default_texture.dds");
}

TextureAssetHandler& TextureAssetHandler::Get()
{
    assert(m_sTextureAssetHandlerSingleton);
    return *m_sTextureAssetHandlerSingleton;
}

RefPtr<Texture> TextureAssetHandler::TryGet(u64 hash)
{
	return AssetHandler<Texture>::TryGet(hash);
}

RefPtr<Texture> TextureAssetHandler::GetOrLoad(u64 hash, const fs::path& path, u8 flags)
{
	return AssetHandler<Texture>::InternalGetOrLoad(hash, [&path, flags]() -> RefPtr<Texture> {
		if (flags & TL_VOLUME_TEXTURE)
		{
			assert(false && "3D Textures are currently unimplemented");
			//LoadImpl3DTexture(path, flags & TL_SRGB_SPACE);
			return nullptr;
		}
		auto texture = LoadImpl2DTexture(path, flags);
		if (!texture)
		{
            m_Logger->critical("GetOrLoad: Failed to load texture {} with flags {}", path.string(), flags);
		}
		return texture;
	});
}
