#pragma once

#include <cstdint>

#include "AppStatePtr.hpp"

struct sapp_event;

class AppState : public std::enable_shared_from_this<AppState>
{
public:
    virtual ~AppState();

    virtual const char* name() const = 0;

    // return false if not successful
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;

    // called when there are events, before update
    // return true if the event was "swallowed" by the state
    // TODO: redesign
    virtual bool processEvent(const sapp_event& e) = 0;

    // dt is time delta in milliseconds since last frame
    virtual void update(uint32_t dt) = 0;

    // called when rendering a frame
    virtual void render() = 0;
};

AppStatePtr State_Noop();
