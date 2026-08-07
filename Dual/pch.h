//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0603
#endif
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// GDI and bitmap definitions are retained for simple native dialogs.
// #define NODRAWTEXT
// #define NOGDI
// #define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <exception>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <tuple>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// DX namespace is already defined in HEIN_Engine's pch.h

// DirectXTK
#include "Keyboard.h"
#include "Mouse.h"
#include "SimpleMath.h"
#include "CommonStates.h"
#include "Effects.h"
#include "DDSTextureLoader.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "Model.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"
#include "DirectXHelpers.h"
#include "GeometricPrimitive.h"

// DirectXTK_Utilities
#include "../../../External/Engine/DirectXTK_Utilities/DebugDraw.h"

#pragma comment(lib, "d3dcompiler.lib")
