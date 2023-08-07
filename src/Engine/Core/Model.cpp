#include <enginepch.h>
#include "Model.h"

#include <Engine/Core/Mesh.h>
#include <Engine/Asset/MeshAssetHandler.h>

Model::Model(const fs::path& path, const Matrix& transform)
	: Model(fast_string_hash(path.string().c_str()), path, transform)
{}

Model::Model(u64 hash, const fs::path& path, const Matrix& transform)
	: m_Mesh(MeshAssetHandler::Get().GetOrLoad(hash, path))
	, m_Transform(transform)
{}

Model::Model(u64 hash, const Matrix& transform)
	: m_Mesh(MeshAssetHandler::Get().TryGet(hash))
	, m_Transform(transform)
{
	if (!m_Mesh)
	{ 
		spdlog::critical("Model::Model(u64 hash, transform) m_Mesh was nullptr after constructing");
	}
	assert(m_Mesh);
}

void Model::SetScale(float scalar)
{
	m_Scale = Matrix::CreateScale(scalar);
}

void Model::SetScale(const Vector3& scale)
{
	m_Scale = Matrix::CreateScale(scale);
}

void Model::SetScale(const Matrix& scale)
{
	m_Scale = scale;
}

void Model::Rotate(const Quaternion& quat)
{
	m_Rotation = Matrix::Transform(m_Rotation, quat);
}

void Model::SetRotation(const Quaternion& quat)
{
	m_Rotation = Matrix::Transform(m_Transform, quat);
}

void Model::SetMesh(const fs::path& path)
{
	m_Mesh = MeshAssetHandler::Get().GetOrLoad(fast_string_hash(path.string().c_str()), path);
}

void Model::TrySetMesh(u64 hash)
{
	auto mesh = MeshAssetHandler::Get().TryGet(hash);
	if (mesh)
	{
		m_Mesh = mesh;
	}
	else
	{
		spdlog::warn("TrySetMesh: Couldn't find mesh with hash {}", hash);
	}
}
