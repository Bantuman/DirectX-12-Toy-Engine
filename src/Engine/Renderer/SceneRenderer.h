#include <Engine/Core/Model.h>
#include <Engine/Core/Events.h>

#include "Renderer.h"
#include "VisibilityBufferRenderer.h"

class Scene;
class RenderTarget;
class Device;
class RootSignature;
class PipelineStateObject;
class CommandList;


class SceneRenderer : public Renderer
{
public:
	bool Initialize();
	void RenderScene(RenderTarget& renderTarget, const Scene& scene, RefPtr<CommandList> commandList = nullptr);
private:    
	UniquePtr<VisibilityBufferRenderer> m_VisibilityBufferRenderer;

	RefPtr<RootSignature> m_RootSignature;
	RefPtr<PipelineStateObject> m_PipelineState;
	RefPtr<Device> m_Device;
};