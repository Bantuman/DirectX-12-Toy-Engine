#pragma once
#include "AssetHandler.h"

#include <Engine/Core/Mesh.h>
#include <Engine/Core/Scene.h>

#include <spdlog/spdlog.h>
using Logger = RefPtr<spdlog::logger>;

#include <filesystem>
namespace fs = std::filesystem;

enum MeshLoadFlags : u8
{
	ML_SKINNED = 1 << 1
};

class MeshAssetHandler : public AssetHandler<Mesh>
{
public:
	void Initialize();

	static MeshAssetHandler& Get();

	RefPtr<Mesh> TryGet(u64 hash) final override;
	RefPtr<Mesh> GetOrLoad(u64 hash, const fs::path& path, u8 flags = 0u);

	RefPtr<Scene> LoadScene(const fs::path& path, u8 flags = 0u);

private:
	friend class Application;
	static UniquePtr<MeshAssetHandler> m_sMeshAssetHandlerSingleton;

	static inline std::vector<RefPtr<Mesh>> LoadImplStaticMesh(const fs::path& path, u8 flags);
	static inline RefPtr<Mesh> LoadImplSkinnedMesh(const fs::path& path, u8 flags);

	static Logger m_Logger;
};