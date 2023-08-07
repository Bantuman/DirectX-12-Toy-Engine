#include <enginepch.h>
#include "MaterialAssetHandler.h"

#include <Engine/Core/Application.h>
#include <Engine/Buffers/Texture.h>
#include <Engine/Pipeline/CommandList.h>
#include <Engine/Pipeline/CommandQueue.h>
#include <Pipeline/ResourceStateTracker.h>
#include <Engine/Asset/TextureAssetHandler.h>

Logger MaterialAssetHandler::m_Logger;

void MaterialAssetHandler::Initialize()
{
    m_Logger = Application::Get().CreateLogger("MaterialAssetHandler");

    m_pDefaultMaterial = MakeRef<Material>();
    m_pDefaultMaterial->SetTexture(Material::TextureType::Diffuse, TextureAssetHandler::Get().GetDefaultTexture());
    m_pDefaultMaterial->SetTexture(Material::TextureType::Normal, TextureAssetHandler::Get().GetDefaultTexture());
    m_pDefaultMaterial->SetTexture(Material::TextureType::Emissive, TextureAssetHandler::Get().GetDefaultTexture());
    // TODO: Set default normal map and material texture
}

MaterialAssetHandler& MaterialAssetHandler::Get()
{
    assert(m_sMaterialAssetHandlerSingleton);
    return *m_sMaterialAssetHandlerSingleton;
}

RefPtr<Material> MaterialAssetHandler::TryGet(u64 hash)
{
    return AssetHandler<Material>::TryGet(hash);
}

RefPtr<Material> MaterialAssetHandler::GetOrLoad(u64 hash, const fs::path& path, u8 flags)
{
    return AssetHandler<Material>::InternalGetOrLoad(hash, [&path, flags]() -> RefPtr<Material> {
        assert(false && "Not implemented");
        return nullptr;
    });
}
