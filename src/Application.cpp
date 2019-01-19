#include "Application.hpp"

#include "Module_DearImGui.hpp"

#include "AppState.hpp"

#include "State_Test.hpp"

#include "sokol_app.h"
#include "SokolGFX.hpp"

#include <iostream>
#include <sstream>
#include <chrono>
#include <cassert>

#if defined(_WIN32)
#   include <Windows.h>
#elif !defined(__EMSCRIPTEN__)
#   include <dlfcn.h>
#endif

using namespace std;

namespace
{

// function used by non-browser apps to get the name of the executable
// by querying the system based on a function address
void getAddr() {}

// time stuff
uint32_t m_currentFrameTime = 0; // start of current frame (ms)
uint32_t m_timeSinceLastFrame = 0; // used as delta for updates
uint32_t m_totalFrames = 0; // number of frames since the app has started
uint32_t m_lastFrameEnd = 0; // end time of the last frame
uint32_t m_lastFPSStatusUpdateTime = 0; // last time the fps status was updated
uint32_t m_lastFPSStatusUpdateFrame = 0; // frame number of the last time the status was updated

// results
uint32_t m_meanFrameTime;
uint32_t m_fps;

// modules
Module_DearImGui m_imgui;

// other
std::string m_fileName;

// rendering
sg_pass_action m_passAction = { };

// states
AppStatePtr m_currentState = State_Noop();
AppStatePtr m_nextState;

// obtain filename for current module
void getFileName()
{
#if defined(_WIN32)
    HMODULE engine;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)getAddr,
        &engine);

    char path[_MAX_PATH + _MAX_FNAME + 1];
    GetModuleFileNameA(engine, path, _MAX_PATH + _MAX_FNAME);

    // normalize path
    char* p = path;
    while (*p)
    {
        if (*p == '\\')
        {
            *p = '/';
        }
        ++p;
    }

    m_fileName = path;

#elif !defined(__EMSCRIPTEN__)
    void* p = reinterpret_cast<void*>(getAddr);

    Dl_info info;
    dladdr(p, &info);

    m_fileName = info.dli_fname;
#else
    // nothing smart to do yet
#endif
}

bool doInit()
{
    m_currentFrameTime = Application::getTicks();

    sg_desc desc = { };
    desc.mtl_device = sapp_metal_get_device();
    desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
    desc.mtl_drawable_cb = sapp_metal_get_drawable;
    desc.d3d11_device = sapp_d3d11_get_device();
    desc.d3d11_device_context = sapp_d3d11_get_device_context();
    desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
    desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
    desc.gl_force_gles2 = sapp_gles2();
    sg_setup(&desc);


    getFileName();

    if (!m_imgui.init())
    {
        cerr << "Couldn't initialize ImGui\n";
        return false;
    }

    // initial clear color
    m_passAction.colors[0].action = SG_ACTION_CLEAR;
    m_passAction.colors[0].val[0] = 0.0f;
    m_passAction.colors[0].val[1] = 0.0f;
    m_passAction.colors[0].val[2] = 0.0f;
    m_passAction.colors[0].val[3] = 1.0f;

    // state
    Application::setState(State_Test());

    return true;
}

void init()
{
    if (!doInit())
    {
        cerr << "Couldn't initialize app\n";
        exit(1);
    }
}

void cleanup()
{
    if (m_nextState)
    {
        m_nextState->cleanup();
        m_nextState.reset();
    }
    m_currentState->cleanup();
    m_currentState.reset();

    m_imgui.cleanup();
    sg_shutdown();
}

void update()
{
    m_imgui.update(m_timeSinceLastFrame);

    m_currentState->update(m_timeSinceLastFrame);
}

void render()
{
    const int width = sapp_width();
    const int height = sapp_height();
    sg_begin_default_pass(&m_passAction, width, height);

    m_currentState->render();

    m_imgui.render();

    sg_end_pass();
    sg_commit();
}

void updateFPSData()
{
    const uint32_t now = Application::getTicks();
    const uint32_t fpsStatusUpdateTimeDelta = now - m_lastFPSStatusUpdateTime;

    if (fpsStatusUpdateTimeDelta > 1000) // update once per second
    {
        const uint32_t framesDelta = m_totalFrames - m_lastFPSStatusUpdateFrame;
        m_meanFrameTime = fpsStatusUpdateTimeDelta / framesDelta;
        m_fps = (framesDelta * 1000) / fpsStatusUpdateTimeDelta;

        m_lastFPSStatusUpdateTime = now;
        m_lastFPSStatusUpdateFrame = m_totalFrames;

        //ostringstream sout;
        //sout << "ft: " << m_meanFrameTime << " ms | fps: " << m_fps;
        //glfwSetWindowTitle(m_window, sout.str().c_str());
    }
}

void onEvent(const sapp_event* e)
{
    if (m_imgui.processEvent(*e)) return;
    if (m_currentState->processEvent(*e)) return;
}

void checkForStateChange()
{
    if (!m_nextState) return;

    auto next = m_nextState.release(); // in case a failed initialize sets another next state
    if (!next->initialize())
    {
        cerr << "Error initializing " << next->name() << ". Aborting\n";
        delete next;
    }
    else
    {
        cout << "Switching to state " << next->name() << "\n";
        m_currentState->cleanup();
        m_currentState.reset(next);
    }
}

void mainLoop()
{
    checkForStateChange();

    uint32_t now = Application::getTicks();
    m_timeSinceLastFrame = now - m_currentFrameTime;
    m_currentFrameTime = now;

    update();
    render();

    ++m_totalFrames;
    updateFPSData();
}

} // anonymous namespace


uint32_t Application::getTicks()
{
    // actually the time returned is since the first time this function is called
    // but this is very early in the execution time, so it's fine

    static auto start = std::chrono::steady_clock::now();
    auto time = std::chrono::steady_clock::now() - start;
    return uint32_t(chrono::duration_cast<chrono::milliseconds>(time).count()) + 1;
}

void Application::setState(AppStatePtr state)
{
    if (m_nextState)
    {
        // we have a pending but never realized next state
        // abort and forget

        cout << "Ignoring state " << m_nextState->name() << " when switching to " << state->name() << "\n";
        m_nextState.reset();
    }

    m_nextState.reset(state.release());
}

const sg_pass_action& Application::defaultPassAction()
{
    return m_passAction;
}

void Application::setDefaultPassAction(const sg_pass_action& action)
{
    m_passAction = action;
}

sapp_desc Application::main(int argc, char* argv[])
{
    sapp_desc desc = {};
    desc.width = 1024;
    desc.height = 768;
    desc.init_cb = init;
    desc.frame_cb = mainLoop;
    desc.cleanup_cb = cleanup;
    desc.event_cb = onEvent;

    return desc;
}
