#pragma once

class Mesh;
class CommandList;

#include <DirectXMath.h>
using namespace DirectX;

class Renderer
{
public:
	virtual void RenderIndexedMesh(CommandList& commandList, const Mesh& mesh, const XMMATRIX& transform);
	virtual void RenderInstancedIndexedMesh(CommandList& commandList, const Mesh& mesh, const std::vector<XMMATRIX>& instances);
};