#pragma once

#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <sdkddkver.h>

#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN

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

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <comdef.h>
#include <array>
#include <sstream>
#include "d3dx12.h"