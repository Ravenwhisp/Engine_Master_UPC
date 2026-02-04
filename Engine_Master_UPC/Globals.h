#pragma once

#define NOMINMAX
#define INITGUID

#ifdef _DEBUG
#define USE_PIX 1
#else
#define USE_PIX 0
#endif 

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

#include "SimpleMath.h"

#if USE_PIX
#include "WinPixEventRuntime/pix3.h"
#endif 

#include <assert.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

#define LOG(format, ...) log(__FILE__, __LINE__, format, __VA_ARGS__);
void log(const char file[], int line, const char* format, ...);

#define FRAMES_IN_FLIGHT 3

#include "debug_draw.hpp"

inline const ddVec3& ddConvert(const Vector3& v) { return reinterpret_cast<const ddVec3&>(v); }
inline const ddMat4x4& ddConvert(const Matrix& m) { return reinterpret_cast<const ddMat4x4&>(m); }

inline size_t alignUp(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

#if USE_PIX
#define BEGIN_EVENT(commandList, text)  PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, text)
#define END_EVENT(commandList) PIXEndEvent(commandList)
#define SET_MARKER(commandList, text) PIXSetMarker(commandList, PIX_COLOR_DEFAULT, text)
#else
#define BEGIN_EVENT(commandList, text)
#define END_EVENT(commandList)
#define SET_MARKER(commandList, text) 
#endif  

#include <imgui.h>
#include <imgui_internal.h>



//Assert that COM call to D3D API succeeded
#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)												\
if(FAILED(x)) {													\
	char line_number[32];										\
	sprintf_s(line_number, "%u", __LINE__);						\
    OutputDebugStringA("Error in: ");							\
	OutputDebugStringA(__FILE__);								\
	OutputDebugStringA("\nLine: ");							\
	OutputDebugStringA(line_number);							\
	OutputDebugStringA("\n");									\
	OutputDebugStringA(#x);										\
	OutputDebugStringA("\n");									\
	__debugbreak();												\
} 
#endif
#else
#ifndef DXCall
#define DXCall(x) x
#endif

#endif // DEBUG

#ifdef _DEBUG
//Sets the name of the COM object and outputs a debug string in Visual Studio's output window
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
#endif


