//
// pch.h
// Precompiled header for Dual_Gameplay static library.
//

#pragma once

#ifndef HEIN_WITH_EDITOR
#define HEIN_WITH_EDITOR 1
#endif

#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0603
#endif
#include <sdkddkver.h>

#define NOMINMAX
#define NOMCX
#define NOSERVICE
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
#include <filesystem>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <tuple>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

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

// Engine DirectXTK Utilities
#include "DirectXTK_Utilities/DebugDraw.h"

// Game Constants
#include "Framework/GameConstants.h"

#pragma comment(lib, "d3dcompiler.lib")
