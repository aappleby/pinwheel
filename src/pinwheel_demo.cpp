#include "pinwheel_demo.h"

#define SDL_MAIN_HANDLED
#ifdef _MSC_VER
#include "SDL/include/SDL.h"
#include <windows.h>
#else
#include <SDL2/SDL.h>
#endif

const char* PinwheelApp::app_get_title() { return "Pinwheel Test App"; }

void PinwheelApp::app_init(int screen_w, int screen_h) {
  dvec2 screen_size(screen_w, screen_h);
  text_painter.init();
  grid_painter.init(65536,65536);
  view_control.init(screen_size);
}

void PinwheelApp::app_close()  {}

bool PinwheelApp::pause_when_idle() { return true; }

void PinwheelApp::app_update(dvec2 screen_size, double delta)  {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_MOUSEMOTION) {
      if (event.motion.state & SDL_BUTTON_LMASK) {
        view_control.pan(dvec2(event.motion.xrel, event.motion.yrel));
      }
    }

    if (event.type == SDL_MOUSEWHEEL) {
      int mouse_x = 0, mouse_y = 0;
      SDL_GetMouseState(&mouse_x, &mouse_y);
      view_control.on_mouse_wheel(dvec2(mouse_x, mouse_y), screen_size, double(event.wheel.y) * 0.25);
    }
  }
  view_control.update(delta);
}

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);
  text_painter.render_string(view, screen_size, "derp", 0, 0);
}

void PinwheelApp::app_render_ui(dvec2 screen_size, double delta)  {}
