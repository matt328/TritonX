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
#include <wrl.h>
#include <iostream>
