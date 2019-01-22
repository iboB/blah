#include "State_Test.hpp"

#include "AppState.hpp"
#include "Application.hpp"

#include "DearImGui.hpp"

#include "SokolGFX.hpp"

#include <yama/yama.hpp>

using namespace yama;

namespace
{

class TestState : public AppState
{
    const char* name() const override { return "test"; }

    bool initialize() override
    {
        sg_pass_action action = {};
        action.colors[0].action = SG_ACTION_CLEAR;
        action.colors[0].val[0] = 0.8f;
        action.colors[0].val[1] = 0.9f;
        action.colors[0].val[2] = 0.9f;
        action.colors[0].val[3] = 1.0f;
        action.depth.action = SG_ACTION_CLEAR;
        action.depth.val = 1;
        action.stencil.action = SG_ACTION_DONTCARE;
        Application::setDefaultPassAction(action);

        return true;
    }

    void cleanup() override
    {
    }

    bool processEvent(const sapp_event& e) override
    {
        return false;
    }

    static float dataGetter(const void* vdata, int idx)
    {
        auto data = reinterpret_cast<const float*>(vdata);
        return data[idx];
    }

    void update(uint32_t dt) override
    {
        ImVec2 screen = {float(Application::screenWidth()), float(Application::screenHeight())};
        ImGui::SetNextWindowPos(ImVec2{0, 0});
        ImGui::SetNextWindowSize(screen);
        ImGui::Begin("DVH", nullptr, ImGuiWindowFlags_NoDecoration);

        static const float data[][7] = 
        {
            { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f },
            { 0.8f, 0.12f, 0.14f, 0.3f, 0.62f, 0.41f, 0.52f },
        };
        static const void* vdata[2]
        {
            data[0],
            data[1],
        };
        static const char* names[] =
        {
            "data a",
            "other data"
        };
        static const ImColor colors[] = 
        {
            {255, 0, 0, 255},
            {0, 0, 255, 255},
        };
        ImGui::PlotMultiLines("Cumulative", 2, names, colors, dataGetter, vdata, 7, FLT_MAX, FLT_MAX, screen);

        ImGui::End();
        //ImGui::ShowDemoWindow();
    }

    void render() override
    {
    }

};

}

AppStatePtr State_Test()
{
    return std::make_unique<TestState>();
}
