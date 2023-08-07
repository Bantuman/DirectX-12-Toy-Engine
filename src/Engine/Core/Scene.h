#pragma once

#include <functional>
#include <map>

#include <Engine/Core/Model.h>
#include <Engine/Core/Camera.h>

class CommandList;
class Device;
class Mesh;
class Material;
class Visitor;

class Scene
{
public:
    Scene() = default;
    ~Scene() = default;
    
    const std::vector<RefPtr<Model>>& GetModels() const { return m_Models; }

    Camera& GetCameraRef() { return m_Camera; }
    const Camera& GetCameraRef() const { return m_Camera; }

    RefPtr<Model> AddModel(RefPtr<Model> model);

protected:
    friend class CommandList;
    friend class SceneRenderer;

private:
    Camera m_Camera;

    std::vector<RefPtr<Model>> m_Models;
};
