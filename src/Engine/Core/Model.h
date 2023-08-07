#pragma once

#include <filesystem>
namespace fs = std::filesystem;

class Mesh;

class Model
{
public:
	Model(const fs::path& path, const Matrix& transform = {});
	Model(u64 hash, const fs::path& path, const Matrix& transform = {});
	Model(u64 hash, const Matrix& transform = {});

	const Matrix& GetTransform() const { return m_Transform; }
	void SetTransform(const Matrix& transform) { m_Transform = transform; }

	void SetScale(float scalar);
	void SetScale(const Vector3& scale);
	void SetScale(const Matrix& scale);
	const Matrix& GetScale() const { return m_Scale; }

	void Rotate(const Quaternion& quat);
	void SetRotation(const Quaternion& quat);
	const Matrix& GetRotation() const { return m_Rotation; }

	virtual void SetMesh(const fs::path& path);
	void TrySetMesh(u64 hash);
	void SetMesh(RefPtr<Mesh> mesh) { m_Mesh = mesh; }
	RefPtr<Mesh> GetMesh() const { return m_Mesh; }

private:
	RefPtr<Mesh> m_Mesh;
	Matrix m_Transform;
	Matrix m_Rotation;
	Matrix m_Scale;
};