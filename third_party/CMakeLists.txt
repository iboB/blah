add_library(sokol INTERFACE)
target_sources(sokol INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/sokol/sokol_app.h
	${CMAKE_CURRENT_SOURCE_DIR}/sokol/sokol_gfx.h
)
target_include_directories(sokol INTERFACE sokol)

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
)
target_include_directories(imgui INTERFACE imgui)
set_target_properties(imgui PROPERTIES FOLDER ThirdParty)

add_subdirectory(yama)
