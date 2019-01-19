include(sources.cmake)

add_executable(vr-dvh
    ${sources}
)

target_link_libraries(vr-dvh
    sokol
	imgui
	yama
)

if(PLATFORM_LINUX)
    target_link_libraries(vr-dvh
        GL
        X11
        dl
    )
endif()
