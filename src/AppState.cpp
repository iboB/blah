#include "AppState.hpp"

AppState::~AppState() = default;

namespace
{

// default implementations

class NoopState : public AppState
{
    const char* name() const override { return "noop"; }
    bool initialize() override { return true; }
    void cleanup() override {}
    bool processEvent(const sapp_event& e) override { return false; }
    void update(uint32_t dt) override {}
    void render() override {}
};

}

AppStatePtr State_Noop()
{
    return std::make_unique<NoopState>();
}
