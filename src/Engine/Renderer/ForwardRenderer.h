#include <Engine/Core/Model.h>
#include <Engine/Core/Events.h>

#include "Renderer.h"

class Scene;
class RenderTarget;
class Device;
class RootSignature;
class PipelineStateObject;
class CommandList;

class ForwardRenderer : Renderer
{
public:
	bool Initialize();
	void BeginFrame();
	void RenderScene(RenderTarget& renderTarget, const Scene& scene, RefPtr<CommandList> commandList = nullptr);
	void EndFrame();
private:
	RefPtr<RootSignature> m_RootSignature;
	RefPtr<PipelineStateObject> m_PipelineState;
	RefPtr<Device> m_Device;
};