#pragma once

#if defined( _WIN64 ) || defined( _WIN32 )
#  include <Windows.h>
#endif

#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <vector>

#include "./api.hpp"

class Method
{
public:
  Method() = default;

  /**
   * @brief Invoke the method as if it were static.
   */
  auto invoke_static( void ** params = nullptr ) const -> void *
  {
    if ( Il2cpp::method_call == nullptr || this == nullptr )
      return nullptr;

    void * excption = nullptr;

    const auto result = Il2cpp::method_call( this, nullptr, params, &excption );

    if ( excption != nullptr )
    {
      // log the exception? do we even care?
    }

    return result;
  }

  /**
   * @brief Invoke the method as if it weren't static.
   */
  auto invoke( void * obj, void ** params = nullptr ) -> const void *
  {
    if ( Il2cpp::method_call == nullptr || this == nullptr )
      return nullptr;

    void * excption = nullptr;

    const auto result = Il2cpp::method_call( this, obj, params, &excption );

    if ( excption != nullptr )
    {
      // log the exception? do we even care?
    }

    return result;
  }

  /**
   * @brief Returns the number of parameters the method has.
   */
  auto get_param_count() const -> uint32_t
  {
    if ( Il2cpp::get_method_param_count == nullptr || this == nullptr )
      return -1;

    return Il2cpp::get_method_param_count( this );
  }

  /**
   * @brief Get the name of a parameter at a specific index.
   */
  auto get_param_name( int index ) const -> const char *
  {
    if ( Il2cpp::get_method_param_name == nullptr || this == nullptr )
      return "Method->get_param_name() is nullptr";

    return Il2cpp::get_method_param_name( this, index );
  }

  /**
   * @brief Get the data type of a parameter.
   */
  auto get_param_type( int index ) const -> const char *
  {
    if ( Il2cpp::get_method_param_type == nullptr || this == nullptr )
      return "Method->get_param_type() is nullptr";

    return Il2cpp::get_method_param_type( this, index );
  }

  auto is_generic() const -> bool
  {
    if ( Il2cpp::get_method_is_generic == nullptr || this == nullptr )
      return false;

    return Il2cpp::get_method_is_generic( this );
  }

  auto is_inflated() const -> bool
  {
    if ( Il2cpp::get_method_is_inflated == nullptr || this == nullptr )
      return false;

    return Il2cpp::get_method_is_inflated( this );
  }

  auto is_instance() const -> bool
  {
    if ( Il2cpp::get_method_is_instance == nullptr || this == nullptr )
      return false;

    return Il2cpp::get_method_is_instance( this );
  }

  /**
   * @brief Get the method name.
   */
  auto get_name() const -> const char *
  {
    if ( Il2cpp::get_method_name == nullptr )
      return "Method->get_name() is nullptr";

    return Il2cpp::get_method_name( this );
  }
};
