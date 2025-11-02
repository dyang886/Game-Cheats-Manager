#pragma once

#if defined( _WIN64 ) || defined( _WIN32 )
#  include <Windows.h>
#endif

#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <vector>

#include "./api.hpp"

class Class;

class Type
{
public:
  Type() = default;

  /**
   * Get a class of this type.
   */
  auto get_class() const -> Class *
  {
    if ( Il2cpp::get_type_class == nullptr )
      return nullptr;

    return Il2cpp::get_type_class( this );
  }

  /**
   * Get the name of the type.
   */
  auto get_name() const -> const char *
  {
    if ( Il2cpp::get_type_name == nullptr )
      return "Il2cpp::get_type_name is not supported.";

    return Il2cpp::get_type_name( this );
  }

  /**
   * Get an object of this type.
   */
  auto get_object() const -> void *
  {
    if ( Il2cpp::get_type_object == nullptr )
      return nullptr;

    return Il2cpp::get_type_object( this );
  }

  /**
   * Get the type's type as an integer.
   */
  auto get_type() -> int
  {
    if ( Il2cpp::get_type_type == nullptr )
      return -1;

    return Il2cpp::get_type_type( this );
  }

  /**
   * Get attributes of this type.
   */
  auto get_attributes() const -> uint32_t
  {
    if ( Il2cpp::get_type_attributes == nullptr )
      return 0;

    return Il2cpp::get_type_attributes( this );
  }

  /**
   * Check if this type is the same as a different type.
   */
  auto compare_type( Type * type ) -> bool
  {
    if ( Il2cpp::get_type_equals == nullptr )
      return false;

    return Il2cpp::get_type_equals( this, type );
  }

  /**
   * Check if this type is byref.
   */
  auto is_by_ref() -> bool
  {
    if ( Il2cpp::get_type_is_byref == nullptr )
      return false;

    return Il2cpp::get_type_is_byref( this );
  }
};
