#pragma once

#include <cstdint>

struct sapp_event;

class Module_DearImGui
{
public:
    static bool init();
    static void cleanup();

    // return true if the event was "swallowed" by ImGui
    static bool processEvent(const sapp_event& e);
    static void update(uint32_t deltaTimeMs);
    static void render();
};
