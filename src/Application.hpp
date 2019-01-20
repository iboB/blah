#pragma once

#include <cstdint>
#include <string>

#include "AppStatePtr.hpp"

struct sapp_desc;
struct sg_pass_action;

class Application
{
public:
    // name of binary which contains application code on desktop platforms
    static const std::string& fileName();

    // time in ms since the application has started
    static uint32_t getTicks();

    // total number of frames the aplication has run
    static uint32_t totalFrames();

    // sets current state.
    // the change will happen at the beginning of the next frame
    static void setState(AppStatePtr state);

    // rendering
    static int screenWidth();
    static int screenHeight();
    static const sg_pass_action& defaultPassAction();
    static void setDefaultPassAction(const sg_pass_action&);

    // main
    static sapp_desc main(int argc, char* argv[]);
};
