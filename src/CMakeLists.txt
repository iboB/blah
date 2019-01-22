include(sources.cmake)

add_executable(sokol-test
    ${sources}
)

target_link_libraries(sokol-test
    sokol
	imgui
	yama
)

if(PLATFORM_LINUX)
    target_link_libraries(sokol-test
        GL
        X11
        dl
    )
endif()
