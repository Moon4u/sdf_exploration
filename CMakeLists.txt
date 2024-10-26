cmake_minimum_required(VERSION 3.5)

project(SDF_EXPLORATION LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SDF_EXPLORATION
    main.cpp
    ${GLAD_SOURCE}
)


# Add SDL3.
# Still in development!
add_subdirectory(libs/SDL)

target_link_libraries(SDF_EXPLORATION PRIVATE SDL3-shared)
target_link_libraries(SDF_EXPLORATION PRIVATE SDL3::Headers)
target_include_directories(SDF_EXPLORATION PRIVATE SDL3::Headers)
get_target_property(INCLUDE_DIRS SDL3::Headers INTERFACE_INCLUDE_DIRECTORIES)

# SDL3 end.

# add IMGUI.

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

include(FetchContent)
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
)
FetchContent_MakeAvailable(implot)


set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    ${implot_SOURCE_DIR}/implot.h
    ${implot_SOURCE_DIR}/implot_internal.h
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PRIVATE SDL3-shared)
target_link_libraries(imgui PRIVATE SDL3::Headers)
target_include_directories(imgui PRIVATE SDL3::Headers)
get_target_property(INCLUDE_DIRS SDL3::Headers INTERFACE_INCLUDE_DIRECTORIES)

target_link_libraries(SDF_EXPLORATION PRIVATE imgui)
target_include_directories(SDF_EXPLORATION PRIVATE ${implot_SOURCE_DIR})
target_include_directories(SDF_EXPLORATION PRIVATE ${imgui_SOURCE_DIR})
target_include_directories(SDF_EXPLORATION PRIVATE ${imgui_SOURCE_DIR}/backends)

# IMGUI end.

include(GNUInstallDirs)
install(TARGETS SDF_EXPLORATION
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
