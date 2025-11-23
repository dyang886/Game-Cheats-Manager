#include "unity.h"

namespace Unity {

	unity_GameObject_FindGameObjectsWithTag_t GameObject_FindGameObjectsWithTag;
	unity_GameObject_GetTransform_t GameObject_GetTransform;
	unity_Transform_GetPositionInjected_t Transform_GetPositionInjected;
	unity_Gamera_GetMain_t Gamera_GetMain;
	unity_Component_GetGameObject_t Component_GetGameObject;
	unity_Screen_GetWidth_t Screen_GetWidth;
	unity_Screen_GetHeight_t Screen_GetHeight;

	bool Initialize(MEMORY memory) {
		
		//GameObject_FindGameObjectsWithTag = find_function<unity_GameObject_FindGameObjectsWithTag_t>("UnityEngine.GameObject::FindGameObjectsWithTag(System.String)");
		GameObject_FindGameObjectsWithTag = find_function<unity_GameObject_FindGameObjectsWithTag_t>("UnityEngine.GameObject::Find(System.String)");
		GameObject_GetTransform = find_function<unity_GameObject_GetTransform_t>("UnityEngine.GameObject::get_transform(System.IntPtr)");
		Transform_GetPositionInjected = find_function<unity_Transform_GetPositionInjected_t>("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)");
		Gamera_GetMain = find_function<unity_Gamera_GetMain_t>("UnityEngine.Camera::get_main()");
		Component_GetGameObject = find_function<unity_Component_GetGameObject_t>("UnityEngine.Component::get_gameObject()");
		Screen_GetWidth = find_function<unity_Screen_GetWidth_t>("UnityEngine.Screen::get_width()");
		Screen_GetHeight = find_function<unity_Screen_GetHeight_t>("UnityEngine.Screen::get_height()");

		return true;
	}
};