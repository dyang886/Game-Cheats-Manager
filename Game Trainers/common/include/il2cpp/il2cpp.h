#pragma once

#include "il2cpp_types.h"

#define IL2CPP_API_H                "il2cpp_api.h"
#define IL2CPP_TYPEDEF(r, n, p)      typedef r(__cdecl* n ## _t)p;
#define IL2CPP_EXTERN_DECLARATION(n) extern n ## _t n;
#define IL2CPP_DECLARATION(n)        n ## _t n;
#define IL2CPP_FIND_FUNCTION(n)      API:: ## n = memory.find_function<n ## _t>(game_assembly, #n);

#define DO_API(r, n, p) IL2CPP_TYPEDEF(r, n, p);
#include IL2CPP_API_H
#undef DO_API

#include "hook.h"


namespace IL2CPP
{
	extern Il2CppDomain* domain;

	bool Initialize(MEMORY memory);
	void Attach();

	namespace API {
		#define DO_API(r, n, p) IL2CPP_EXTERN_DECLARATION(n)
		#include IL2CPP_API_H
		#undef DO_API
	}

	struct METHOD;
	struct STRING;
	struct ASSEMBLY;
	struct CLASS;
	struct FIELD;
	struct NAMESPACE;
	struct ARRAY;
	struct STRING;
	struct OBJECT;
	struct TYPE;

	struct METHOD : MethodInfo {
		const char* Name();

		// Overload 1: Hook with original pointer return
		template <class T>
		HOOK* Hook(MEMORY memory, void* new_method, T* original) {
			auto new_hook = Hook<T>(memory, new_method);
			*original = new_hook->get_original<T>();

			return new_hook;
		}

		// Overload 2: Hook without original pointer return
		template <class T>
		HOOK* Hook(MEMORY memory, void* new_method) {
			auto new_hook = new HOOK(memory, this->FindFunction<T>(), new_method);

			new_hook->load();

			return new_hook;
		}

		template <class T>
		T FindFunction() {
			return (T)this->methodPointer;
		}
	};

	struct STRING : Il2CppString {
		const size_t Length();
		const wchar_t* WChars();
	};

	struct ASSEMBLY : Il2CppAssembly {
		NAMESPACE* Namespace(const char* assembly_name);

		Il2CppImage* Image();
		ASSEMBLY* List(size_t* size);
	};

	struct NAMESPACE {
		NAMESPACE(ASSEMBLY* assembly_, const char* name_);

		CLASS* Class(const char* class_name);

		ASSEMBLY* assembly;
		const char* name;
	};

	struct CLASS : Il2CppClass {
		TYPE* Type(const char* field_name);

		METHOD* Method(const char* method_name, int args_count = 0);
		//const MethodInfo* methods(void** iter);

		FIELD* Field(const char* field_name);
		//FieldInfo* fields(void** iter);

		const PropertyInfo* Property(const char* property_name);
		//const PropertyInfo* properties(void** iter);

		const char* Name();
		const char* Namespaze();
		const char* AssemblyName();
		const Il2CppImage* Image();
	};

	struct FIELD : FieldInfo {
		const char* Name();
		size_t Offset();
		void SetValue(OBJECT* obj, void* value);
		void SetStaticValue(void* value);

		TYPE* Type();

		template <class T>
		T GetValue(OBJECT* obj) {

			T ret;

			API::il2cpp_field_get_value(obj, this, &ret);

			return ret;
		};

		template <class T>
		T GetStaticValue() {

			T ret;

			API::il2cpp_field_static_get_value(this, &ret);

			return ret;
		};

		OBJECT* GetObject(OBJECT* obj);
		ARRAY*  GetArray(OBJECT* obj);
		STRING* GetString(OBJECT* obj);

		OBJECT* GetStaticObject();
		ARRAY*  GetStaticArray();
		STRING* GetStaticString();
	};

	struct TYPE : Il2CppType {
		CLASS* Class();
		const char* Name();
	};

	struct OBJECT : Il2CppObject {
		FIELD* Field(const char* field_name);

		template <class T>
		T GetValue(const char* field_name) {
			return Field(field_name)->GetValue<T>(this);
		};

		ARRAY* GetArray(const char* field_name);
		STRING* GetString(const char* field_name);
		OBJECT* GetObject(const char* field_name);

		void SetValue(const  char* field_name, void* value);

		CLASS* Class();
		TYPE* Type(const char* field_name);
	};

	struct ARRAY : Il2CppArray {
		size_t MaxLength();

		template <class T>
		T* GetArray() {
			return (T*)((uintptr_t)this + sizeof(Il2CppArray));
		}

		template <class T>
		T GetIndex(size_t id) {
			return GetArray<T>()[id];
		}

		OBJECT* GetObject(size_t id);
		STRING* GetString(size_t id);
	};

	ASSEMBLY* Assembly(const char* name);
	NAMESPACE* Namespace(ASSEMBLY* assembly, const char* name);
	STRING* String(const char* str);

	CLASS*  Class(const char* assembly_name, const char* namespace_name, const char* class_name);
	FIELD*  Field(const char* assembly_name, const char* namespace_name, const char* class_name, const char* field_name);
	METHOD* Method(const char* assembly_name, const char* namespace_name, const char* class_name, const char* method_name, int param_count);

	template<class T>
	T Function(const char* assembly_name, const char* namespace_name, const char* klass_name, const char* method_name, int param_count) {
		return Method(assembly_name, namespace_name, klass_name, method_name, param_count)->FindFunction<T>();
	}
};