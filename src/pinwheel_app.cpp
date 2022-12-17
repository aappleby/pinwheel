#include "pinwheel_app.h"

#define SDL_MAIN_HANDLED
#ifdef _MSC_VER
#include "SDL/include/SDL.h"
#include <windows.h>
#else
#include <SDL2/SDL.h>
#endif

#include "CoreLib/Dumper.h"
#include "CoreLib/Log.h"

const char* PinwheelApp::app_get_title() { return "Pinwheel Test App"; }

void PinwheelApp::app_init(int screen_w, int screen_h) {
  dvec2 screen_size(screen_w, screen_h);
  text_painter.init();
  grid_painter.init(65536,65536);
  view_control.init(screen_size);

  pinwheel.tock(true);
}

void PinwheelApp::app_close()  {
  LOG_G("PinwheelApp::app_close()\n");
}

bool PinwheelApp::pause_when_idle() { return true; }

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

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);

  StringDumper d;

  d("reg_p0.hart   %d\n",     pinwheel.reg_p0.hart);
  d("reg_p0.pc     0x%08x\n", pinwheel.reg_p0.pc);
  d("reg_p0.insn   0x%08x\n", pinwheel.reg_p0.active ? int(pinwheel.pbus_data) : 0);
  d("reg_p0.enable %d\n",     pinwheel.reg_p0.active);
  d("reg_p0.active %d\n",     pinwheel.reg_p0.enable);
  d("\n");

  d("reg_p1.hart   %d\n",     pinwheel.reg_p1.hart);
  d("reg_p1.pc     0x%08x\n", pinwheel.reg_p1.pc);
  d("reg_p1.insn   0x%08x\n", pinwheel.reg_p1.insn);
  d("reg_p1.enable %d\n",     pinwheel.reg_p1.active);
  d("reg_p1.active %d\n",     pinwheel.reg_p1.enable);
  d("\n");

  d("reg_p2.hart   %d\n",     pinwheel.reg_p2.hart);
  d("reg_p2.pc     0x%08x\n", pinwheel.reg_p2.pc);
  d("reg_p2.insn   0x%08x\n", pinwheel.reg_p2.insn);
  d("reg_p2.align  %d\n",     pinwheel.reg_p2.align);
  d("reg_p2.alu    0x%08x\n", pinwheel.reg_p2.alu_out);
  d("reg_p2.enable %d\n",     pinwheel.reg_p2.active);
  d("reg_p2.active %d\n",     pinwheel.reg_p2.enable);
  d("\n");

  d("ra            0x%08x\n", pinwheel.ra);
  d("rb            0x%08x\n", pinwheel.rb);
  d("dbus_data     0x%08x\n", pinwheel.dbus_data);
  d("pbus_data     0x%08x\n", pinwheel.pbus_data);
  d("\n");

  int phases[4] = {-1, -1, -1, -1};

  phases[pinwheel.reg_p0.hart] = 0;
  phases[pinwheel.reg_p1.hart] = 1;
  phases[pinwheel.reg_p2.hart] = 2;

  for (int hart = 0; hart < 4; hart++) {
    auto r = &pinwheel.regfile[hart << 5];
    d("hart %d phase %d\n", hart, phases[hart]);
    switch(phases[hart]) {
      case 0:
        d("pc     0x%08x\n", pinwheel.reg_p0.pc);
        d("insn   0x%08x\n", pinwheel.reg_p0.active ? int(pinwheel.pbus_data) : 0);
        d("enable %d\n",     pinwheel.reg_p0.active);
        d("active %d\n",     pinwheel.reg_p0.enable);
        d("alu    ----------\n");
        break;
      case 1:
        d("pc     0x%08x\n", pinwheel.reg_p1.pc);
        d("insn   0x%08x\n", pinwheel.reg_p1.insn);
        d("enable %d\n",     pinwheel.reg_p1.active);
        d("active %d\n",     pinwheel.reg_p1.enable);
        d("alu    ----------\n");
        break;
      case 2:
        d("pc     0x%08x\n", pinwheel.reg_p2.pc);
        d("insn   0x%08x\n", pinwheel.reg_p2.insn);
        d("enable %d\n",     pinwheel.reg_p2.active);
        d("active %d\n",     pinwheel.reg_p2.enable);
        d("alu    0x%08x\n", pinwheel.reg_p2.alu_out);
        break;
      case -1:
        d("idle\n");
    }
    d("regs\n");
    d("  r00 0x%08x r01 0x%08x r02 0x%08x r03 0x%08x\n", r[ 0], r[ 1], r[ 2], r[ 3]);
    d("  r04 0x%08x r05 0x%08x r06 0x%08x r07 0x%08x\n", r[ 4], r[ 5], r[ 6], r[ 7]);
    d("  r08 0x%08x r09 0x%08x r10 0x%08x r11 0x%08x\n", r[ 8], r[ 9], r[10], r[11]);
    d("  r12 0x%08x r13 0x%08x r14 0x%08x r15 0x%08x\n", r[12], r[13], r[14], r[15]);
    d("  r16 0x%08x r17 0x%08x r18 0x%08x r19 0x%08x\n", r[16], r[17], r[18], r[19]);
    d("  r20 0x%08x r21 0x%08x r22 0x%08x r23 0x%08x\n", r[20], r[21], r[22], r[23]);
    d("  r24 0x%08x r25 0x%08x r26 0x%08x r27 0x%08x\n", r[24], r[25], r[26], r[27]);
    d("  r28 0x%08x r29 0x%08x r30 0x%08x r31 0x%08x\n", r[28], r[29], r[30], r[31]);
    d("\n");
  }

  text_painter.render_string(view, screen_size, d.s.c_str(), 32, 32);
}

void PinwheelApp::app_render_ui(dvec2 screen_size, double delta)  {}
