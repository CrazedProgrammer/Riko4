#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#define _CRT_SECURE_NO_WARNINGS

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) \
 || defined(__TOS_WIN__) || defined(__WINDOWS__)
/* Compiling for Windows */
#  ifndef __WINDOWS__
#    define __WINDOWS__
#  endif
#  include <windows.h>
#endif/* Predefined Windows macros */

#ifndef CALLBACK
#  if defined(_ARM_)
#    define CALLBACK
#  else
#    define CALLBACK __stdcall
#  endif
#endif

#ifdef __EMSCRIPTEN__
#  include "emscripten.h"
#endif

#ifndef __WINDOWS__
#  include <ftw.h>
#endif

#include <cstring>

#include "events.h"
#include "fs.h"
#include "luaIncludes.h"
#include "luaMachine.h"
#include "process.h"

#include "riko.h"

namespace riko {
    bool running = true;
    int exitCode = 0;

    bool useBundle = false;

    lua_State *mainThread;
    SDL_Window *window;
}


int main(int argc, char * argv[]) {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(riko::events::loop, 0, 0);
#endif

    riko::process::parseCommands(argc, argv);

    int libStatus = riko::process::initLibs();
    if (libStatus != 0) return libStatus;

    riko::process::parseConfig();

    int scriptStatus = riko::process::openScripts();
    if (scriptStatus != 0) return scriptStatus;

    int windowStatus = riko::process::setupWindow();
    if (windowStatus != 0) return windowStatus;

    char bootLoc[strlen(riko::fs::scriptsPath) + 10];
    sprintf(bootLoc, "%s/boot.lua", riko::fs::scriptsPath);
    riko::mainThread = riko::lua::createLuaInstance(bootLoc, "@boot.lua");

    if (riko::mainThread == nullptr) {
        return 7;
    }

    riko::events::ready = true;
#ifndef __EMSCRIPTEN__
    while (riko::running) {
        int start = SDL_GetTicks();

        riko::events::loop();

        int time = SDL_GetTicks() - start;
        if (time < 0) continue;

        int sleepTime = 100/6 - time;
        if (sleepTime > 0) {
            SDL_Delay(sleepTime);
        }
    }

    riko::process::cleanup();

    return riko::exitCode;
#else
    return 0;
#endif
}

#pragma clang diagnostic pop
