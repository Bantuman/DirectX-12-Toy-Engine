#include "enginepch.h"

#include "Scene.h"

#include <Pipeline/CommandList.h>
#include <Device.h>
#include <Material.h>
#include <Mesh.h>
#include <Buffers/Texture.h>
#include <VertexTypes.h>

RefPtr<Model> Scene::AddModel(RefPtr<Model> model)
{
	m_Models.push_back(model);
	return model;
}
