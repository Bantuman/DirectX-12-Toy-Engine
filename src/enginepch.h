#pragma once

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0608
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW
#include <comdef.h> // For _com_error class (used to decode HR result codes).

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <DDSTextureLoader.h>

using namespace DirectX;

#include <SimpleMath\SimpleMath.h>
using namespace DirectX::SimpleMath;

// D3D12 extension library.
#pragma warning(push, 0)
#include <d3dx12.h>
#pragma warning(pop)

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <set>

#include <filesystem>
namespace fs = std::filesystem;

// Helper functions
#include <Engine/Core/Helpers.h>
#include <Engine/Core/Defines.h>

// spdlog
#include <spdlog/async.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Common lock type
using scoped_lock = std::lock_guard<std::mutex>;

