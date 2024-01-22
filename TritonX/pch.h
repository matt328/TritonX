#pragma once

#define WIN32_LEAN_AND_MEAN

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <iostream>
#include <functional>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>

#include <comdef.h>
#include <array>