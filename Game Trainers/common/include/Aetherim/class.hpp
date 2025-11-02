#pragma once

#if defined( _WIN64 ) || defined( _WIN32 )
#  include <Windows.h>
#else
#  include <string.h>
#endif

#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <vector>

#include "./api.hpp"
#include "./method.hpp"
#include "./field.hpp"

class Class
{
private:
  const char * name {};

public:
  Class() = default;

  using fields_t = std::vector<Field *>;

  /**
   * Returns a pointer to the specified method.
   */
  auto get_method( const char * name, int argc = 0 ) const -> Method *
  {
    if ( Il2cpp::get_method == nullptr )
      return nullptr;

    auto method = Il2cpp::get_method( this, name, argc );
    if ( !method )
      return nullptr;

    return reinterpret_cast<Method *>( method );
  }

  /**
   * Returns a field given a name.
   */
  auto get_field( const char * name ) const -> Field *
  {
    if ( Il2cpp::get_field == nullptr )
      return nullptr;

    auto field = Il2cpp::get_field( this, name );
    if ( !field )
      return nullptr;

    return reinterpret_cast<Field *>( field );
  }

  /**
   * Returns all fields of the class as a vector of void pointers.
   */
  auto get_fields() const -> fields_t
  {
    fields_t m_fields {};
    if ( m_fields.size() )
      return m_fields;

    if ( Il2cpp::get_field_count == nullptr || Il2cpp::get_fields == nullptr )
      return m_fields;

    const size_t count = Il2cpp::get_field_count( this );
    if ( count )
      m_fields.resize( count );

    void * iter = NULL;
    void * field = nullptr;

    size_t index = 0;

    while ( field = Il2cpp::get_fields( this, &iter ) )
    {
      if ( !field || field == NULL )
        continue;

      m_fields[index++] = reinterpret_cast<Field *>( field );
    }

    return m_fields;
  }

  /**
   * Given a name, returns a class that's nested inside of the current class.
   */
  auto get_nested_class( const char * name ) const -> Class *
  {
    if ( Il2cpp::get_nested_types == nullptr || Il2cpp::get_class_name == nullptr )
      return nullptr;

    void * iter = NULL;

    while ( auto type = Il2cpp::get_nested_types( this, &iter ) )
    {
      const auto class_name = Il2cpp::get_class_name( type );

      if ( !strcmp( class_name, name ) )
        return type;
    }

    return nullptr;
  }

  /**
   * Return the name of the current class.
   */
  auto get_name() const -> const char *
  {
    if ( Il2cpp::get_class_name == nullptr )
      return "Class->get_name() is nullptr";

    return Il2cpp::get_class_name( this );
  }
};
