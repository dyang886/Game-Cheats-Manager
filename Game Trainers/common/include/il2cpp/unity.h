#pragma once

#include "memory.h"
#include "il2cpp.h"


// Unity Types
struct vec3;
struct Rect;
struct GUIContent;
struct GUIStyle;

// Unity API
typedef void** (__cdecl* unity_GameObject_FindGameObjectsWithTag_t)(Il2CppString*);
typedef  void* (__cdecl* unity_GameObject_GetTransform_t)(void*);
typedef  void* (__cdecl* unity_Transform_GetPositionInjected_t)(vec3&);
typedef  void* (__cdecl* unity_Gamera_GetMain_t)();
typedef  void* (__cdecl* unity_Component_GetGameObject_t)(const char*);
typedef    int (__cdecl* unity_Screen_GetWidth_t)();
typedef    int (__cdecl* unity_Screen_GetHeight_t)();

namespace Unity
{
	template <class T>
	T find_function(const char* function_name) {
		return (T)IL2CPP::API::il2cpp_resolve_icall(function_name);
	}

	bool Initialize(MEMORY memory);

	extern unity_GameObject_FindGameObjectsWithTag_t GameObject_FindGameObjectsWithTag;
	extern unity_GameObject_GetTransform_t GameObject_GetTransform;
	extern unity_Transform_GetPositionInjected_t Transform_GetPositionInjected;
	extern unity_Gamera_GetMain_t Gamera_GetMain;
	extern unity_Component_GetGameObject_t Component_GetGameObject;
	extern unity_Screen_GetWidth_t Screen_GetWidth;
	extern unity_Screen_GetHeight_t Screen_GetHeight;
};

