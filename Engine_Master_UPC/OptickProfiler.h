#pragma once

#if defined(ENGINE_ENABLE_OPTICK)
#include <optick.h>

#define PERF_FRAME(name) OPTICK_FRAME(name)
#define PERF_FUNCTION() OPTICK_EVENT()
#define PERF_RENDER(name) OPTICK_CATEGORY(name, Optick::Category::Rendering)
#define PERF_LOGIC(name) OPTICK_CATEGORY(name, Optick::Category::GameLogic)
#define PERF_INPUT(name) OPTICK_CATEGORY(name, Optick::Category::Input)
#else
#define PERF_FRAME(name) ((void)0)
#define PERF_FUNCTION() ((void)0)
#define PERF_RENDER(name) ((void)0)
#define PERF_LOGIC(name) ((void)0)
#define PERF_INPUT(name) ((void)0)
#endif