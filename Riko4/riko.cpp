#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) \
 || defined(__TOS_WIN__) || defined(__WINDOWS__)
/* Compiling for Windows */
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#  include <windows.h>
#endif/* Predefined Windows macros */

#ifndef CALLBACK
#if defined(_ARM_)
#define CALLBACK
#else
#define CALLBACK __stdcall
#endif
#endif

#ifndef MAXINT
#define MAXUINT     ((UINT)~((UINT)0))
#define MAXINT      ((INT)(MAXUINT >> 1))
#define MININT      ((INT)~MAXINT)
#endif

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <LuaJIT/lua.hpp>

#include <LFS/lfs.h>

#include <rikoGPU.h>
#include <rikoAudio.h>
#include <rikoImage.h>

SDL_Window *window;
SDL_Renderer *renderer;

lua_State *mainThread;

double pixelSize = 5;
double afPixscale = 5;// 7.2 / 2;

void printLuaError(int result) {
	if (result != 0) {
		switch (result) {
			case LUA_ERRRUN:
				SDL_Log("Lua Runtime error");
				break;
			case LUA_ERRSYNTAX:
				SDL_Log("Lua syntax error");
				break;
			case LUA_ERRMEM:
				SDL_Log("Lua was unable to allocate the required memory");
				break;
			case LUA_ERRFILE:
				SDL_Log("Lua was unable to find boot file");
				break;
			default:
				SDL_Log("Unknown lua error: %d", result);
		}
	}
}

void createLuaInstance(const char* filename) {
	lua_State *state = luaL_newstate();

	// Make standard libraries available in the Lua object
	luaL_openlibs(state);

	luaopen_gpu(state);
	luaopen_aud(state);
	luaopen_image(state);

	mainThread = lua_newthread(state);

	int result;

	// Load the program; this supports both source code and bytecode files.
	result = luaL_loadfile(mainThread, filename);

	if (result != 0) {
		printLuaError(result);
		return;
	}
}

void SDL_SetRendererViewportRatio_17_10(SDL_Window *window,
	SDL_Renderer *renderer, SDL_Rect *viewport) {

	printf("Begin size: %f", pixelSize);

	Uint8 r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	/*w = w / 2;
	h = h / 2;*/
	printf("%d, %d", w, h);
	if (w * 10 > h * 17) {
		viewport->w = h * 17 / 10;
		viewport->h = h;
		printf("End W: %d %d", h * 17 / 10, h);
		//pixelSize = h / 200;
		//SDL_RenderSetLogicalSize(renderer, h * 17/10, h);
	} else {
		viewport->w = w;
		viewport->h = w * 10 / 17;
		//pixelSize = w / 340;
		//SDL_RenderSetLogicalSize(renderer, w, w * 10/17);
	}
	printf("\n\nViewoffset: %d \n\n", (w - viewport->w) / 2);
	viewport->x = (w - viewport->w) / 2;
	viewport->y = (h - viewport->h) / 2;

	//SDL_RenderSetViewport(renderer, viewport);

	//SDL_RenderSetScale(renderer, 7.2, 7.2);
}

int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_DisplayMode current;
	int lw = MAXINT;
	int lh = MAXINT;

	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {

		int should_be_zero = SDL_GetCurrentDisplayMode(i, &current);

		if (should_be_zero != 0) {
			SDL_Log("Could not get display mode for video display #%d: %s", i, SDL_GetError());
		} else {
			if (current.w < lw && current.h < lh) {
				lw = current.w;
				lh = current.h;
			}
		}
	}

	window = SDL_CreateWindow(
		"Riko4",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		340*pixelSize,//lw / 2,
		200*pixelSize,//lh / 2,
		SDL_WINDOW_OPENGL //| SDL_WINDOW_FULLSCREEN
	);

	SDL_ShowCursor(SDL_DISABLE);

	if (window == NULL) {
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1, 
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	SDL_Rect *viewport = new SDL_Rect();
	SDL_SetRendererViewportRatio_17_10(window, renderer, viewport);

	SDL_Surface *surface;
	surface = SDL_LoadBMP("icon.ico");

	SDL_SetWindowIcon(window, surface);

	SDL_Event event;

	createLuaInstance("scripts/boot.lua");

	bool canRun = true;
	bool running = true;
	int pushedArgs = 0;

	int lastMoveX = 0;
	int lastMoveY = 0;

	int cx;
	int cy;
	int mult;

	int exitCode = 0;

	while (running) {
		if (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					break;
				case SDL_TEXTINPUT:
					lua_pushstring(mainThread, "char");
					lua_pushstring(mainThread, event.text.text);
					pushedArgs = 2;
					break;
				case SDL_KEYDOWN:
					lua_pushstring(mainThread, "key");
					lua_pushstring(mainThread, SDL_GetKeyName(event.key.keysym.sym));
					pushedArgs = 2;
					break;
				case SDL_KEYUP:
					lua_pushstring(mainThread, "keyUp");
					lua_pushstring(mainThread, SDL_GetKeyName(event.key.keysym.sym));
					pushedArgs = 2;
					break;
				case SDL_MOUSEWHEEL:
					lua_pushstring(mainThread, "mouseWheel");
					mult = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -1 : 1;
					
					lua_pushnumber(mainThread, event.wheel.y * mult);
					lua_pushnumber(mainThread, event.wheel.x * mult);
					pushedArgs = 3;
					break;
				case SDL_MOUSEMOTION:
					cx = event.motion.x / afPixscale;
					cy = event.motion.y / afPixscale;
					if (cx != lastMoveX || cy != lastMoveY) {
						lua_pushstring(mainThread, "mouseMoved");
						lua_pushnumber(mainThread, cx);
						lua_pushnumber(mainThread, cy);
						lua_pushnumber(mainThread, cx - lastMoveX);
						lua_pushnumber(mainThread, cy - lastMoveY);
						lastMoveX = cx;
						lastMoveY = cy;
						pushedArgs = 5;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					lua_pushstring(mainThread, "mousePressed");
					lua_pushnumber(mainThread, (int) event.button.x / afPixscale);
					lua_pushnumber(mainThread, (int) event.button.y / afPixscale);
					lua_pushnumber(mainThread, event.button.button);
					pushedArgs = 4;
					break;
				case SDL_MOUSEBUTTONUP:
					lua_pushstring(mainThread, "mouseReleased");
					lua_pushnumber(mainThread, (int)event.button.x / afPixscale);
					lua_pushnumber(mainThread, (int)event.button.y / afPixscale);
					lua_pushnumber(mainThread, event.button.button);
					pushedArgs = 4;
					break;
			}
		}

		if (event.type == SDL_QUIT) {
			break;
		}

		if (canRun) {
			int result = lua_resume(mainThread, pushedArgs);

			if (result == 0) {
				printf("Script finished!\n");
				canRun = false;
			} else if (result != LUA_YIELD) {
				printLuaError(result);
				canRun = false;
				exitCode = 1;
			}
		}	

		pushedArgs = 0;
		SDL_Delay(1);
	}

	//closeAudio();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	free(viewport);

	SDL_Quit();

	return exitCode;
}