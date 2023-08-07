#pragma once


#include <DirectXMath.h>  // For XMFLOAT2
using namespace DirectX;

class Texture;
class CommandList;

// Don't use scoped enums to avoid the explicit cast required to use these as
// array indices.
enum AttachmentPoint
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    NumAttachmentPoints
};

class RenderTarget
{
public:
    // Create an empty render target.
    RenderTarget();

    RenderTarget( const RenderTarget& copy ) = default;
    RenderTarget( RenderTarget&& copy )      = default;

    RenderTarget& operator=( const RenderTarget& other ) = default;
    RenderTarget& operator=( RenderTarget&& other ) = default;

    /**
     * Attach a texture to a given attachment point.
     *
     * @param attachmentPoint The point to attach the texture to.
     * @param [texture] Optional texture to bind to the render target. Specify nullptr to remove the texture.
     */
    void                     AttachTexture( AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture );
    std::shared_ptr<Texture> GetTexture( AttachmentPoint attachmentPoint ) const;

    // Resize all of the textures associated with the render target.
    void             Resize( DirectX::XMUINT2 size );
    void             Resize( u32 width, u32 height );
    DirectX::XMUINT2 GetSize() const;
    u32         GetWidth() const;
    u32         GetHeight() const;

    // Get a viewport for this render target.
    // The scale and bias parameters can be used to specify a split-screen
    // viewport (the bias parameter is normalized in the range [0...1]).
    // By default, a fullscreen viewport is returned.
    D3D12_VIEWPORT GetViewport( DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f },
                                float minDepth = 0.0f, float maxDepth = 1.0f ) const;

    // Get a list of the textures attached to the render target.
    // This method is primarily used by the CommandList when binding the
    // render target to the output merger stage of the rendering pipeline.
    const std::vector<std::shared_ptr<Texture>>& GetTextures() const;

    // Get the render target formats of the textures currently
    // attached to this render target object.
    // This is needed to configure the Pipeline state object.
    D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

    // Get the format of the attached depth/stencil buffer.
    DXGI_FORMAT GetDepthStencilFormat() const;

    // Get the sample description of the render target.
    DXGI_SAMPLE_DESC GetSampleDesc() const;

    void ClearDepth(RefPtr<CommandList> commandList);

    void Clear(RefPtr<CommandList> commandList, const XMFLOAT4& clearColor, bool clearDepth = true);

    // Reset all textures
    void Reset()
    {
        m_Textures = RenderTargetList( AttachmentPoint::NumAttachmentPoints );
    }

private:
    using RenderTargetList = std::vector<std::shared_ptr<Texture>>;
    RenderTargetList m_Textures;
    DirectX::XMUINT2                      m_Size;
};
