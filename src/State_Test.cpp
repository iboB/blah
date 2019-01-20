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

    void update(uint32_t dt) override
    {
        ImGui::SetNextWindowPos(ImVec2{0, 0});
        ImGui::SetNextWindowSize(ImVec2{float(Application::screenWidth()), float(Application::screenHeight())});
        ImGui::Begin("DVH", nullptr, ImGuiWindowFlags_NoDecoration);



        ImGui::End();
        ImGui::ShowDemoWindow();
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
