#include "il2cpp.h"


namespace IL2CPP {

	Il2CppDomain* domain;

	namespace API {
		#define DO_API(r, n, p) IL2CPP_DECLARATION(n)
		#include IL2CPP_API_H
		#undef DO_API
	}

	bool Initialize(MEMORY memory)
	{
		void* game_assembly = memory.find_module("GameAssembly.dll");

		if (!game_assembly) {
			printf("[!] CRITICAL: Could not find GameAssembly.dll module!\n");
			return false;
		}

		#define DO_API(r, n, p) IL2CPP_FIND_FUNCTION(n)
		#include IL2CPP_API_H
		#undef DO_API

		domain = API::il2cpp_domain_get();

		return true;
	}

	void Attach() {
		API::il2cpp_thread_attach(domain);
	}
	
	// ASSEMBLY

	Il2CppImage* ASSEMBLY::Image() {
		return this->image;
	}

	NAMESPACE* ASSEMBLY::Namespace(const char* namespace_name) {
		return new NAMESPACE(this, namespace_name);
	}

	ASSEMBLY* ASSEMBLY::List(size_t* size) {
		return (ASSEMBLY*)IL2CPP::API::il2cpp_domain_get_assemblies(domain, size);
	}

	// NAMESPACE

	NAMESPACE::NAMESPACE(ASSEMBLY* assembly_, const char* name_) {
		assembly = assembly_;
		name = name_;
	}

	CLASS* NAMESPACE::Class(const char* class_name) {
		return (CLASS*)API::il2cpp_class_from_name(assembly->Image(), name, class_name);
	}

	// CLASS

	TYPE* CLASS::Type(const char* field_name) {
		return Field(field_name)->Type();
	}

	METHOD* CLASS::Method(const char* method_name, int args_count) {
		return (METHOD*)API::il2cpp_class_get_method_from_name(klass, method_name, args_count);
	};

	FIELD* CLASS::Field(const char* field_name) {
		return (FIELD*)API::il2cpp_class_get_field_from_name(klass, field_name);
	}

	const char* CLASS::Name() {
		return  IL2CPP::API::il2cpp_class_get_name(klass);
	}

	const char* CLASS::Namespaze() {
		return  IL2CPP::API::il2cpp_class_get_namespace(klass);
	}

	const char* CLASS::AssemblyName() {
		return  IL2CPP::API::il2cpp_class_get_assemblyname(klass);
	}

	const Il2CppImage* CLASS::Image() {
		return IL2CPP::API::il2cpp_class_get_image(klass);
	}

	// FIELD

	const char* FIELD::Name() {
		return IL2CPP::API::il2cpp_field_get_name(this);
	}

	TYPE* FIELD::Type() {
		return (TYPE*)API::il2cpp_field_get_type(this);
	}

	size_t FIELD::Offset() {
		return API::il2cpp_field_get_offset(this);
	}

	void FIELD::SetValue(OBJECT* obj, void* value) {
		API::il2cpp_field_set_value(obj, this, value);
	}

	void FIELD::SetStaticValue(void* value) {
		API::il2cpp_field_static_set_value(this, value);
	}

	OBJECT* FIELD::GetObject(OBJECT* obj) {
		return GetValue<OBJECT*>(obj);
	}

	ARRAY* FIELD::GetArray(OBJECT* obj) {
		return GetValue<ARRAY*>(obj);
	}

	STRING* FIELD::GetString(OBJECT* obj) {
		return GetValue<STRING*>(obj);
	}

	OBJECT* FIELD::GetStaticObject() {
		return GetStaticValue<OBJECT*>();
	}

	ARRAY* FIELD::GetStaticArray() {
		return GetStaticValue<ARRAY*>();
	}

	STRING* FIELD::GetStaticString() {
		return GetStaticValue<STRING*>();
	}

	// Method

	const char* METHOD::Name() {
		return API::il2cpp_method_get_name(this);
	}

	// Type

	CLASS* TYPE::Class() {
		return (CLASS*)API::il2cpp_type_get_class_or_element_class(this);
	}

	const char* TYPE::Name() {
		return API::il2cpp_type_get_name(this);
	}

	// IL2CPP_OBJECT

	FIELD* OBJECT::Field(const char* field_name) {
		return Class()->Field(field_name);
	}

	OBJECT* OBJECT::GetObject(const char* field_name) {
		return Field(field_name)->GetObject(this);
	}

	ARRAY* OBJECT::GetArray(const char* field_name) {
		return Field(field_name)->GetArray(this);
	}

	STRING* OBJECT::GetString(const char* field_name) {
		return Field(field_name)->GetString(this);
	}

	CLASS* OBJECT::Class() {
		return (CLASS*)API::il2cpp_object_get_class(this);
	}

	TYPE* OBJECT::Type(const char* field_name) {
		return Field(field_name)->Type();
	}

	void OBJECT::SetValue(const char* field_name, void* value) {
		Field(field_name)->SetValue(this, value);
	}

	// IL2CPP_ARRAY

	size_t ARRAY::MaxLength() {
		return this->max_length;
	}

	OBJECT* ARRAY::GetObject(size_t id) {
		return GetIndex<OBJECT*>(id);
	}

	STRING* ARRAY::GetString(size_t id) {
		return GetIndex<STRING*>(id);
	}


	const size_t STRING::Length() {
		return API::il2cpp_string_length(this);
	}

	const wchar_t* STRING::WChars() {
		return (wchar_t*)API::il2cpp_string_chars(this);
	}

	ASSEMBLY* Assembly(const char* assembly_name) {
		return (ASSEMBLY*)API::il2cpp_domain_assembly_open(domain, assembly_name);
	}

	STRING* String(const char* str){
		return (STRING*)API::il2cpp_string_new(str);
	}

	CLASS* Class(const char* assembly_name, const char* namespace_name, const char* class_name) {
		return Assembly(assembly_name)->Namespace(namespace_name)->Class(class_name);
	}

	FIELD* Field(const char* assembly_name, const char* namespace_name, const char* class_name, const char* field_name) {
		return Class(assembly_name, namespace_name, class_name)->Field(field_name);
	}

	METHOD* Method(const char* assembly_name, const char* namespace_name, const char* class_name, const char* method_name, int param_count) {
		return Class(assembly_name, namespace_name, class_name)->Method(method_name, param_count);
	}
}
