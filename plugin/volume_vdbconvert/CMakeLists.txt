#
#   Lightmetrica - Copyright (c) 2020 Felix Heim
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# Find vdbloader library
find_package(vdbloader)
if (vdbloader_FOUND)
    # Create plugin
    lm_add_plugin(
        NAME volume_vdbconvert
        LIBRARIES
            vdbloader::vdbloader
        SOURCES
            "volume_vdbconvert.cpp"
            "renderer_vdbconvert.cpp")
endif()
