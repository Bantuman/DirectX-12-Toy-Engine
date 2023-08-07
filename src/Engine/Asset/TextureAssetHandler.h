#pragma once
#include "AssetHandler.h"

#include <Engine/Buffers/Texture.h>

#include <spdlog/spdlog.h>
using Logger = RefPtr<spdlog::logger>;

#include <filesystem>
namespace fs = std::filesystem;

enum TextureLoadFlags : u8
{
	TL_FORCE_SRGB_SPACE = 1 << 0,
	TL_VOLUME_TEXTURE = 1 << 1
};

class TextureAssetHandler : public AssetHandler<Texture>
{
public:
	void Initialize();

	static TextureAssetHandler& Get();

	RefPtr<Texture> GetDefaultTexture() const { return m_pDefaultTexture; }

	RefPtr<Texture> TryGet(u64 hash) final override;
	RefPtr<Texture> GetOrLoad(u64 hash, const fs::path& path, u8 flags = 0u);
private:
	friend class Application;
	static UniquePtr<TextureAssetHandler> m_sTextureAssetHandlerSingleton;

	RefPtr<Texture> m_pDefaultTexture;

	static inline RefPtr<Texture> LoadImpl2DTexture(const fs::path& path, u8 flags);
	static Logger m_Logger;
};