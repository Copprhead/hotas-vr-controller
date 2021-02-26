#pragma once

#ifdef HOTASVRCONTROLLERDRIVER_EXPORTS
#define HOTASVRCONTROLLERDRIVER_API extern "C" __declspec(dllexport)
#else
#define HOTASVRCONTROLLERDRIVER_API extern "C" __declspec(dllimport)
#endif
