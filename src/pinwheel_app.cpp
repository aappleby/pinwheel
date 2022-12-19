#include "pinwheel_app.h"

#include "rvdisasm.h"

#define SDL_MAIN_HANDLED
#ifdef _MSC_VER
#include "SDL/include/SDL.h"
#include <windows.h>
#else
#include <SDL2/SDL.h>
#endif

#include "CoreLib/Dumper.h"
#include "CoreLib/Log.h"

//------------------------------------------------------------------------------

const char* PinwheelApp::app_get_title() {
  return "Pinwheel Test App";
}

//------------------------------------------------------------------------------

void PinwheelApp::app_init(int screen_w, int screen_h) {
  dvec2 screen_size(screen_w, screen_h);
  text_painter.init();
  grid_painter.init(65536,65536);
  view_control.init(screen_size);

  pinwheel.tock(true);
}

//------------------------------------------------------------------------------

void PinwheelApp::app_close()  {
  LOG_G("PinwheelApp::app_close()\n");
}

//------------------------------------------------------------------------------

bool PinwheelApp::pause_when_idle() { return true; }

//------------------------------------------------------------------------------

void PinwheelApp::app_update(dvec2 screen_size, double delta)  {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {


    if (event.type == SDL_KEYDOWN)
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        view_control.view_target      = Viewport::screenspace(screen_size);
        view_control.view_target_snap = Viewport::screenspace(screen_size);
        break;
      case SDLK_RIGHT:  {
        pinwheel.tick(0);
        break;
      }
    }

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

//------------------------------------------------------------------------------

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);

  StringDumper d;

  d("vane0.hart   %d\n",     pinwheel.vane0.hart);
  d("vane0.pc     0x%08x\n", pinwheel.vane0.pc);
  d("vane0.insn   0x%08x ", pinwheel.vane0.active ? int(pinwheel.pbus_data) : 0);
  print_rv(d, pinwheel.vane0.active ? uint32_t(pinwheel.pbus_data) : 0);
  d("\n");
  d("vane0.enable %d\n",     pinwheel.vane0.active);
  d("vane0.active %d\n",     pinwheel.vane0.enable);
  d("\n");

  d("vane1.hart   %d\n",     pinwheel.vane1.hart);
  d("vane1.pc     0x%08x\n", pinwheel.vane1.pc);
  d("vane1.insn   0x%08x ",  pinwheel.vane1.insn);
  print_rv(d, pinwheel.vane1.active ? uint32_t(pinwheel.vane1.insn) : 0);
  d("\n");
  d("vane1.enable %d\n",     pinwheel.vane1.active);
  d("vane1.active %d\n",     pinwheel.vane1.enable);
  d("\n");

  d("vane2.hart   %d\n",     pinwheel.vane2.hart);
  d("vane2.pc     0x%08x\n", pinwheel.vane2.pc);
  d("vane2.insn   0x%08x ", pinwheel.vane2.insn);
  print_rv(d, pinwheel.vane2.active ? uint32_t(pinwheel.vane2.insn) : 0);
  d("\n");
  d("vane2.enable %d\n",     pinwheel.vane2.active);
  d("vane2.active %d\n",     pinwheel.vane2.enable);
  d("\n");

  d("vane2.align  %d\n",     pinwheel.align);
  d("vane2.alu    0x%08x\n", pinwheel.alu_out);
  d("ra           0x%08x\n", pinwheel.ra);
  d("rb           0x%08x\n", pinwheel.rb);
  d("dbus_data    0x%08x\n", pinwheel.dbus_data);
  d("pbus_data    0x%08x\n", pinwheel.pbus_data);
  d("\n");

  registers_base* harts[3];

  harts[pinwheel.vane0.hart] = &pinwheel.vane0;
  harts[pinwheel.vane1.hart] = &pinwheel.vane1;
  harts[pinwheel.vane2.hart] = &pinwheel.vane2;

  for (int hart = 0; hart < 3; hart++) {
    auto r = &pinwheel.regfile[hart << 5];
    d("hart %d vane %d", hart, harts[hart]->vane);
    d("\n");
    d("r00 %08x  r01 %08x  r02 %08x  r03 %08x\n", r[ 0], r[ 1], r[ 2], r[ 3]);
    d("r04 %08x  r05 %08x  r06 %08x  r07 %08x\n", r[ 4], r[ 5], r[ 6], r[ 7]);
    d("r08 %08x  r09 %08x  r10 %08x  r11 %08x\n", r[ 8], r[ 9], r[10], r[11]);
    d("r12 %08x  r13 %08x  r14 %08x  r15 %08x\n", r[12], r[13], r[14], r[15]);
    d("r16 %08x  r17 %08x  r18 %08x  r19 %08x\n", r[16], r[17], r[18], r[19]);
    d("r20 %08x  r21 %08x  r22 %08x  r23 %08x\n", r[20], r[21], r[22], r[23]);
    d("r24 %08x  r25 %08x  r26 %08x  r27 %08x\n", r[24], r[25], r[26], r[27]);
    d("r28 %08x  r29 %08x  r30 %08x  r31 %08x\n", r[28], r[29], r[30], r[31]);
    d("\n");
  }
  text_painter.render_string(view, screen_size, d.s.c_str(), 32, 32);


  {
    d.clear();

    for (int i = 0; i < 16; i++) {
      auto op = pinwheel.code_mem[(harts[0]->pc >> 2) + i];
      d("0x%08x ", harts[0]->pc + i * 4);
      print_rv(d, op);
      d("\n");
    }

    text_painter.render_string(view, screen_size, d.s.c_str(), 32 + 384, 32);
  }

}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_ui(dvec2 screen_size, double delta) {

}

//------------------------------------------------------------------------------
