#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "./class.hpp"

class Image
{
private:
  const char * m_name {};

public:
  using classes_t = std::vector<void *>;

  /**
   * Returns a pointer to the class given a class name.
   */
  auto get_class( const char * name, const char * namespaze = "" ) const -> Class *
  {
    return Il2cpp::get_class( this, namespaze, name );
  }

  /**
   * Returns all classes of the image as a vector of void pointers.
   */
  auto get_classes() const -> classes_t
  {
    static classes_t m_classes {};
    if ( m_classes.size() )
      return m_classes;

    size_t count = Il2cpp::get_class_count( this );
    if ( count )
      m_classes.resize( count );

    size_t valid_classes = 0;

    for ( size_t index = 0U; index < count; ++index )
    {
      auto address = Il2cpp::get_class_from_image( this, index );
      if ( !address )
        continue;

      auto name = Il2cpp::get_class_name( address );
      if ( !name )
        continue;

      if ( !strcmp( name, "<Module>" ) )
        continue;

      m_classes[valid_classes++] = address;
    }

    return m_classes;
  }

  /**
   * Returns the name of the image.
   */
  auto get_name() const { return m_name; }
};
