#pragma once
#include <DirectXMath.h>

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>
#include <vector>
#include <map>

// Helper functions
#include <Engine/Core/Helpers.h>

// D3D12 extension library.
#pragma warning(push, 0)
#include <Engine/Core/d3dx12.h>
#pragma warning(pop)

#include <d3d12.h>
#include <SimpleMath\SimpleMath.h>
using namespace DirectX::SimpleMath;

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

