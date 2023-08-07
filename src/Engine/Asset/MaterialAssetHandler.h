#pragma once
#include "AssetHandler.h"

#include <Engine/Core/Material.h>

#include <spdlog/spdlog.h>
using Logger = RefPtr<spdlog::logger>;

#include <filesystem>
namespace fs = std::filesystem;

enum MaterialLoadFlags : u8
{
};

class MaterialAssetHandler : public AssetHandler<Material>
{
public:
	void Initialize();

	static MaterialAssetHandler& Get();

	RefPtr<Material> GetDefaultMaterial() const { return m_pDefaultMaterial; }

	RefPtr<Material> TryGet(u64 hash) final override;
	RefPtr<Material> GetOrLoad(u64 hash, const fs::path& path, u8 flags = 0u);
private:
	friend class Application;
	static UniquePtr<MaterialAssetHandler> m_sMaterialAssetHandlerSingleton;

	RefPtr<Material> m_pDefaultMaterial;

	static Logger m_Logger;
};