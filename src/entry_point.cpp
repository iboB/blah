#include "Application.hpp"

#define SOKOL_IMPL
#define SOKOL_GLCORE33
//~ #define SOKOL_D3D11
//~ #define SOKOL_D3D11_SHADER_COMPILER
#define SOKOL_WIN32_FORCE_MAIN
#if !defined(NDEBUG)
#   define SOKOL_DEBUG 1
#endif
#include "sokol_app.h"
#include "SokolGFX.hpp"

sapp_desc sokol_main(int argc, char* argv[]) {
    auto desc = Application::main(argc, argv);

    return desc;
}
