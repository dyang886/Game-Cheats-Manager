#pragma once

#if defined(_WIN64) || defined(_WIN32)
#  include <Windows.h>
#  define APICALL             __fastcall
#  define GetAddress          GetProcAddress
#  define GetHandle( module ) GetModuleHandleW( module )
#else
#  include <dlfcn.h>
#  define APICALL
#  define GetAddress          dlsym
#  define GetHandle( module ) dlopen( (char *) module, RTLD_LAZY )
#endif

#include <cstdint>
#include "./table_defs.h"

#define DEF_API( name, ret_type, args )        \
  using name##_t = ret_type( APICALL * ) args; \
  inline name##_t name = nullptr;

#define DEF_ADDR( name, il2cpp_export, handle ) \
  name = reinterpret_cast<name##_t>( GetAddress( handle, il2cpp_export ) );

class Image;
class Class;
class Method;
class Type;

namespace Il2cpp
{
  inline bool initialized { false };

  // debugger
  DEF_API( is_debugger_attached, bool, (void) );

  // internal calls and runtime
  DEF_API( add_icall, void, ( const char * name, const Method * method_ptr ) );
  DEF_API( resolve_icall, void, ( const char * name ) );
  DEF_API( init_class, void, ( void * klass ) );
  DEF_API( init_object, void, ( void * obj ) );

  // thread, domain, and assembly
  DEF_API( thread_attach, void *, ( void * domain ) );
  DEF_API( get_domain, void *, (void) );
  DEF_API( get_assemblies, void **, ( const void * domain, size_t * count ) );

  // images
  DEF_API( get_image, Image *, ( const void * assembly ) );
  DEF_API( get_image_name, const char *, ( const void * image ) );
  DEF_API( get_class_count, size_t, ( const void * image ) );
  DEF_API( get_class_from_image, Class *, ( const void * image, size_t index ) );

  // objects
  DEF_API( object_create, void *, ( void * domain, void * klass ) );
  DEF_API( object_unbox, void *, ( void * obj ) );
  DEF_API( object_box, void *, ( Class * klass, void * data ) );
  DEF_API( object_get_size, uint32_t, ( void * obj ) );
  DEF_API( object_get_class, Class *, ( void * obj ) );
  DEF_API( object_get_virtual_method, Method *, ( void * obj, Method * method ) );

  // arrays
  DEF_API( get_array_element_size, int, ( const void * array_class ) );
  DEF_API( get_array_class, void *, ( const void * element_class, uint32_t rank ) );
  DEF_API( get_array_length, uint32_t, ( const void * array ) );
  DEF_API( array_new, void *, ( void * element_type_infof, size_t length ) );

  // class
  DEF_API( get_class, Class *, ( const void * image, const char * namespaze, const char * name ) );
  DEF_API( get_class_name, const char *, ( const void * klass ) );
  DEF_API( get_class_namespace, const char *, ( const void * klass ) );
  DEF_API( get_class_type, void *, ( const void * klass ) );
  DEF_API( get_class_generic, bool, ( const void * klass ) );
  DEF_API( get_class_inflated, bool, ( const void * klass ) );
  DEF_API( get_class_has_parent, bool, ( const void * klass, const void * klass_c ) );
  DEF_API( get_class_get_parent, void *, ( const void * klass ) );
  DEF_API( get_class_is_subclass_of, bool, ( const void * klass, const void * parent_klass, bool check_interfaces ) );
  DEF_API( get_class_from_type, void *, ( const void * type ) );
  DEF_API( get_class_event_info, void *, ( const void * klass, void * iter ) );
  DEF_API( get_class_interfaces, void *, ( const void * klass, void * iter ) );
  DEF_API( get_class_properties, void *, ( const void * klass, void * iter ) );
  DEF_API( get_class_property_from_name, void *, ( const void * klass, const char * name ) );

  // types
  DEF_API( get_nested_types, Class *, (const void * klass, void *) );
  DEF_API( get_type_attributes, uint32_t, ( const void * type ) );
  DEF_API( get_type_class, Class *, ( const void * type ) );
  DEF_API( get_type_name, const char *, ( const void * type ) );
  DEF_API( get_type_object, void *, ( const void * type ) );
  DEF_API( get_type_type, int, ( void * type ) );
  DEF_API( get_type_equals, bool, ( Type * type_1, Type * type_2 ) );
  DEF_API( get_type_is_byref, bool, ( Type * type ) );

  // methods
  DEF_API( method_call, void *, ( const Method * method, void * obj, void ** params, void ** excption ) );
  DEF_API( get_method, void *, ( const void * klass, const char * name, int params ) );
  DEF_API( get_methods, void *, ( const void * klass, void * iter ) );
  DEF_API( get_method_name, const char *, ( const Method * method ) );
  DEF_API( get_method_is_generic, bool, ( const Method * method ) );
  DEF_API( get_method_is_inflated, bool, ( const Method * method ) );
  DEF_API( get_method_is_instance, bool, ( const Method * method ) );
  DEF_API( get_method_param_count, int, ( const Method * method ) );
  DEF_API( get_method_param_name, const char *, ( const Method * method, int index ) );
  DEF_API( get_method_param_type, const char *, ( const Method * method, int index ) );
  DEF_API( get_method_return_type, void *, ( const Method * method ) );

  // fields
  DEF_API( get_fields, void *, ( const void * klass, void * iter ) );
  DEF_API( get_field, void *, ( const void * klass, const char * name ) );
  DEF_API( get_field_offset, size_t, ( const void * field ) );
  DEF_API( get_field_count, size_t, ( const void * klass ) );
  DEF_API( get_field_name, const char *, ( const void * field ) );
  DEF_API( get_field_object, void *, ( const void * field, const void * obj ) );
  DEF_API( get_field_type, void *, ( const void * field ) );
  DEF_API( get_static_field, void *, ( const void * field, void * output ) );
  DEF_API( set_static_field, void *, ( const void * field, void * value ) );
  DEF_API( get_field_flags, int, ( const void * field ) );

  // strings
  DEF_API( get_string_chars, wchar_t *, ( void * string_obj ) );
  DEF_API( create_string, const char *, ( void * domain, const char * text ) );

  /**
   * Initialize Unity's IL2CPP run-time methods.
   */
  inline void initialize()
  {
    if ( initialized )
      return;

    const auto GameAssemblyHandle = GetHandle(L"GameAssembly.dll");
    if ( GameAssemblyHandle == nullptr )
    {
      throw std::runtime_error( "Failed to get GameAssembly.dll handle." );
    }

    // debugger
    DEF_ADDR( is_debugger_attached, "il2cpp_is_debugger_attached", GameAssemblyHandle );

    // internal calls and runtime
    DEF_ADDR( add_icall, "il2cpp_add_internal_call", GameAssemblyHandle );
    DEF_ADDR( resolve_icall, "il2cpp_resolve_icall", GameAssemblyHandle );
    DEF_ADDR( init_class, "il2cpp_runtime_class_init", GameAssemblyHandle );
    DEF_ADDR( init_object, "il2cpp_runtime_object_init", GameAssemblyHandle );

    // thread, domains, and assemblies
    DEF_ADDR( thread_attach, "il2cpp_thread_attach", GameAssemblyHandle );
    DEF_ADDR( get_domain, "il2cpp_domain_get", GameAssemblyHandle );
    DEF_ADDR( get_assemblies, "il2cpp_domain_get_assemblies", GameAssemblyHandle );

    // images
    DEF_ADDR( get_image, "il2cpp_assembly_get_image", GameAssemblyHandle );
    DEF_ADDR( get_image_name, "il2cpp_image_get_name", GameAssemblyHandle );
    DEF_ADDR( get_class_from_image, "il2cpp_image_get_class", GameAssemblyHandle );
    DEF_ADDR( get_class_count, "il2cpp_image_get_class_count", GameAssemblyHandle );

    // objects
    DEF_ADDR( object_create, "il2cpp_object_new", GameAssemblyHandle );
    DEF_ADDR( object_box, "il2cpp_object_box", GameAssemblyHandle );
    DEF_ADDR( object_unbox, "il2cpp_object_unbox", GameAssemblyHandle );
    DEF_ADDR( object_get_size, "il2cpp_object_get_size", GameAssemblyHandle );
    DEF_ADDR( object_get_class, "il2cpp_object_get_class", GameAssemblyHandle );
    DEF_ADDR( object_get_virtual_method, "il2cpp_object_get_virtual_method", GameAssemblyHandle );

    // arrays
    DEF_ADDR( get_array_element_size, "il2cpp_array_element_size", GameAssemblyHandle );
    DEF_ADDR( get_array_class, "il2cpp_array_class_get", GameAssemblyHandle );
    DEF_ADDR( get_array_length, "il2cpp_array_length", GameAssemblyHandle );
    DEF_ADDR( array_new, "il2cpp_array_new", GameAssemblyHandle );

    // classes
    DEF_ADDR( get_class, "il2cpp_class_from_name", GameAssemblyHandle );
    DEF_ADDR( get_class_name, "il2cpp_class_get_name", GameAssemblyHandle );
    DEF_ADDR( get_class_namespace, "il2cpp_class_get_namespace", GameAssemblyHandle );
    DEF_ADDR( get_class_type, "il2cpp_class_enum_basetype", GameAssemblyHandle );
    DEF_ADDR( get_class_generic, "il2cpp_class_is_generic", GameAssemblyHandle );
    DEF_ADDR( get_class_inflated, "il2cpp_class_is_inflated", GameAssemblyHandle );
    DEF_ADDR( get_class_has_parent, "il2cpp_has_parent", GameAssemblyHandle );
    DEF_ADDR( get_class_get_parent, "il2cpp_class_get_parent", GameAssemblyHandle );
    DEF_ADDR( get_class_is_subclass_of, "il2cpp_class_is_subclass_of", GameAssemblyHandle );
    DEF_ADDR( get_class_from_type, "il2cpp_class_from_il2cpp_type", GameAssemblyHandle );
    DEF_ADDR( get_class_event_info, "il2cpp_class_get_events", GameAssemblyHandle );
    DEF_ADDR( get_class_interfaces, "il2cpp_class_get_interfaces", GameAssemblyHandle );
    DEF_ADDR( get_class_properties, "il2cpp_class_get_properties", GameAssemblyHandle );
    DEF_ADDR( get_class_property_from_name, "il2cpp_class_property_from_name", GameAssemblyHandle );

    // types
    DEF_ADDR( get_nested_types, "il2cpp_class_get_nested_types", GameAssemblyHandle );
    DEF_ADDR( get_type_attributes, "il2cpp_type_get_attrs", GameAssemblyHandle );
    DEF_ADDR( get_type_class, "il2cpp_type_get_class_or_element_class", GameAssemblyHandle );
    DEF_ADDR( get_type_name, "il2cpp_type_get_name", GameAssemblyHandle );
    DEF_ADDR( get_type_object, "il2cpp_type_get_object", GameAssemblyHandle );
    DEF_ADDR( get_type_type, "il2cpp_type_get_type", GameAssemblyHandle );
    DEF_ADDR( get_type_equals, "il2cpp_type_equals", GameAssemblyHandle );
    DEF_ADDR( get_type_is_byref, "il2cpp_type_is_byref", GameAssemblyHandle );

    // methods
    DEF_ADDR( method_call, "il2cpp_runtime_invoke", GameAssemblyHandle );
    DEF_ADDR( get_method, "il2cpp_class_get_method_from_name", GameAssemblyHandle );
    DEF_ADDR( get_methods, "il2cpp_class_get_methods", GameAssemblyHandle );
    DEF_ADDR( get_method_name, "il2cpp_method_get_name", GameAssemblyHandle );
    DEF_ADDR( get_method_is_generic, "il2cpp_method_is_generic", GameAssemblyHandle );
    DEF_ADDR( get_method_is_inflated, "il2cpp_method_is_inflated", GameAssemblyHandle );
    DEF_ADDR( get_method_is_instance, "il2cpp_method_is_instance", GameAssemblyHandle );
    DEF_ADDR( get_method_param_count, "il2cpp_method_get_param_count", GameAssemblyHandle );
    DEF_ADDR( get_method_param_name, "il2cpp_method_get_param_name", GameAssemblyHandle );
    DEF_ADDR( get_method_param_type, "il2cpp_get_param", GameAssemblyHandle );
    DEF_ADDR( get_method_return_type, "il2cpp_method_get_return_type", GameAssemblyHandle );

    // fields
    DEF_ADDR( get_fields, "il2cpp_class_get_fields", GameAssemblyHandle );
    DEF_ADDR( get_field, "il2cpp_class_get_field_from_name", GameAssemblyHandle );
    DEF_ADDR( get_field_offset, "il2cpp_field_get_offset", GameAssemblyHandle );
    DEF_ADDR( get_field_count, "il2cpp_class_num_fields", GameAssemblyHandle );
    DEF_ADDR( get_field_name, "il2cpp_field_get_name", GameAssemblyHandle );
    DEF_ADDR( get_field_object, "il2cpp_field_get_value_object", GameAssemblyHandle );
    DEF_ADDR( get_field_type, "il2cpp_field_get_type", GameAssemblyHandle );
    DEF_ADDR( get_static_field, "il2cpp_field_static_get_value", GameAssemblyHandle );
    DEF_ADDR( set_static_field, "il2cpp_field_static_set_value", GameAssemblyHandle );
    DEF_ADDR( get_field_flags, "il2cpp_field_get_flags", GameAssemblyHandle );

    // strings
    DEF_ADDR( get_string_chars, "il2cpp_string_chars", GameAssemblyHandle );
    DEF_ADDR( create_string, "il2cpp_string_new", GameAssemblyHandle );

    initialized = true;
  };
}

#undef APICALL
#undef DEF_API
#undef DEF_ADDR
#undef GetAddress
#undef GetHandle
