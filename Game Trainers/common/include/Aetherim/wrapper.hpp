#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "./image.hpp"

class Wrapper
{
public:
  using images_t = std::vector<Image *>;

private:
  images_t m_images {};

public:
  /**
   * Dumps all images.
   */
  Wrapper()
  {
    size_t count = 0U;

    const auto domain = Il2cpp::get_domain();
    Il2cpp::thread_attach( domain );

    const auto assemblies = Il2cpp::get_assemblies( domain, &count );

    for ( size_t index = 0U; index < count; ++index )
    {
      auto image = Il2cpp::get_image( assemblies[index] );
      if ( image )
        m_images.emplace_back( static_cast<Image *>( image ) );
    }
  }

  /**
   * Returns a pointer to the desired image if found.
   */
  auto get_image( const char * name ) const -> Image *
  {
    for ( const auto image : m_images )
    {
      if ( !strcmp( name, image->get_name() ) )
        return image;
    }

    return nullptr;
  }

  /**
   * Returns all images.
   */
  auto get_images() const -> images_t { return m_images; }

  /**
   * Checks if there's an active debugger attached to the thread.
   */
  auto is_debugger_attached() const -> bool { return Il2cpp::is_debugger_attached(); }
};
