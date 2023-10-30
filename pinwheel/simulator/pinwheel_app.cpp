#include "pinwheel/simulator/pinwheel_app.h"

#include "pinwheel/tools/rvdisasm.h"
#include "pinwheel/tools/tilelink.h"

#define SDL_MAIN_HANDLED
#ifdef _MSC_VER
#include "SDL/include/SDL.h"
#include <windows.h>
#else
#include <SDL2/SDL.h>
#endif

#include "metrolib/core/Dumper.h"
#include "metrolib/core/Log.h"

#include <elf.h>
#include <sys/stat.h>

inline static const char* tilelink_op_to_string(int op) {
  switch(op) {
    case TL::PutFullData: return "PutFullData";
    case TL::PutPartialData: return "PutPartialData";
    case TL::ArithmeticData: return "ArithmeticData";
    case TL::LogicalData: return "LogicalData";
    case TL::Get: return "Get";
    case TL::Intent: return "Intent";
    case TL::Acquire: return "Acquire";
  }
  return "<invalid>";
}

//------------------------------------------------------------------------------

PinwheelApp::PinwheelApp() {
}

//------------------------------------------------------------------------------

const char* PinwheelApp::app_get_title() {
  return "Pinwheel Test App";
}

//------------------------------------------------------------------------------

void PinwheelApp::app_init(int screen_w, int screen_h) {

  //const char* firmware_filename = "bin/tests/firmware/write_code";
  //const char* firmware_filename = "bin/tests/firmware/basic";
  //const char* firmware_filename = "bin/tests/firmware/hello";
  //const char* firmware_filename = "tests/rv_tests/add.elf";

  pinwheel_sim = new PinwheelSim(
    "gen/tests/firmware/hello.code.vh",
    "gen/tests/firmware/hello.data.vh",
    "pinwheel/uart/message.hex"
  );

  sim_thread = new SimThread(pinwheel_sim);

  dvec2 screen_size(screen_w, screen_h);

  view_control.init(screen_size);
  text_painter.init();
  grid_painter.init(65536,65536);
  code_painter.init_hex_u32();
  //data_painter.init_hex_u8();
  data_painter.init_hex_u32();
  console_painter.init_ascii();
  box_painter.init();

  auto& p = pinwheel_sim->states.top();

  p.tock(true);
  p.tick(true);

  sim_thread->start();
}

//------------------------------------------------------------------------------

void PinwheelApp::app_close()  {
  LOG_G("PinwheelApp::app_close()\n");
  sim_thread->stop();
  delete sim_thread;
  delete pinwheel_sim;
  LOG_G("PinwheelApp::app_close() done\n");
}

//------------------------------------------------------------------------------

bool PinwheelApp::pause_when_idle() { return false; }

//------------------------------------------------------------------------------

void PinwheelApp::app_update(dvec2 screen_size, double delta)  {
  SDL_Event event;
  sim_thread->pause();

  auto keyboard_state = SDL_GetKeyboardState(nullptr);

  int manual_steps = 0;

  while (SDL_PollEvent(&event)) {

    if (event.type == SDL_KEYDOWN)
    switch (event.key.keysym.sym) {

      case SDLK_ESCAPE:
        view_control.view_target      = Viewport::screenspace(screen_size);
        view_control.view_target_snap = Viewport::screenspace(screen_size);
        break;

      case SDLK_RIGHT:  {
        if (!running) {
          pinwheel_sim->states.push();
          manual_steps = 1;
          if (keyboard_state[SDL_SCANCODE_LSHIFT])  manual_steps *= 100;
          if (keyboard_state[SDL_SCANCODE_LALT])    manual_steps *= 10;
          if (keyboard_state[SDL_SCANCODE_LCTRL])   manual_steps *= 3;
        }
        break;
      }

      case SDLK_r: {
        running = !running;
        if (!running) {
          pinwheel_sim->steps = 0;
        }
        break;
      }

      case SDLK_f: {
        fastmode = !fastmode;
        break;
      }

      case SDLK_LEFT: {
        if (!running) {
          pinwheel_sim->states.pop();
        }
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

  if (running) {
    pinwheel_sim->steps = fastmode ? 0x10000000 : 100;
  }
  else {
    pinwheel_sim->steps = manual_steps;
  }

  sim_thread->resume();
  fflush(stdout);
}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  sim_thread->pause();

  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);

  StringDumper d;

  auto& pinwheel = pinwheel_sim->states.top();

  uint32_t insn_a = b24(pinwheel.core.reg_hpc_a) ? uint32_t(pinwheel.code_ram.get()) : 0;

  auto hart_a = ((pinwheel.core.reg_hpc_a >> 24) & 0xF);
  auto hart_b = ((pinwheel.core.reg_hpc_b >> 24) & 0xF);
  auto hart_c = ((pinwheel.core.reg_hpc_c >> 24) & 0xF);
  auto hart_d = ((pinwheel.core.reg_hpc_d >> 24) & 0xF);

  uint32_t hart_pcs[16] = {0};
  if (pinwheel.core.reg_hpc_d) hart_pcs[hart_d] = pinwheel.core.reg_hpc_d;
  if (pinwheel.core.reg_hpc_c) hart_pcs[hart_c] = pinwheel.core.reg_hpc_c;
  if (pinwheel.core.reg_hpc_b) hart_pcs[hart_b] = pinwheel.core.reg_hpc_b;
  if (pinwheel.core.reg_hpc_a) hart_pcs[hart_a] = pinwheel.core.reg_hpc_a;

  auto hart_a_col = pinwheel.core.reg_hpc_a ? hart_a + 2 : 0;
  auto hart_b_col = pinwheel.core.reg_hpc_b ? hart_b + 2 : 0;
  auto hart_c_col = pinwheel.core.reg_hpc_c ? hart_c + 2 : 0;
  auto hart_d_col = pinwheel.core.reg_hpc_d ? hart_d + 2 : 0;

  int cursor_y = 32;

  {
    d.s.push_back(1);
    d("debug_reg     0x%08x\n", pinwheel.debug_reg.get());
    d("ticks         %lld\n",   pinwheel.core.reg_ticks);
    d("tick rate     %f\n",     double(sim_thread->sim_steps) / sim_thread->sim_time);
    d("states        %d\n",     pinwheel_sim->states.state_count());
    d("state bytes   %d\n",     pinwheel_sim->states.state_size_bytes());
    d("debug reg     0x%llx\n", pinwheel.get_debug());
    d("\n");

    if (running) {
      sim_thread->sim_steps = 0;
      sim_thread->sim_time = 0;
    }
  }

  {
    //logic<3>  a_opcode;
    //logic<3>  a_param;
    //logic<3>  a_size;
    //logic<1>  a_source;
    //logic<32> a_address;
    //logic<4>  a_mask;
    //logic<32> a_data;
    //logic<1>  a_valid;
    //logic<1>  a_ready;

    d("bus op   %s\n",     tilelink_op_to_string(pinwheel.core.bus_tla.a_opcode));
    d("bus addr 0x%08x\n", pinwheel.core.bus_tla.a_address);
    d("bus data 0x%08x\n", pinwheel.core.bus_tla.a_data);
    d("bus mask 0x%08x\n", pinwheel.core.bus_tla.a_mask);
    d("\n");
  }

  {
    d.s.push_back(1);
    d("decode\n");
    d.s.push_back(hart_a_col);
    d("pc   0x%08x\n", pinwheel.core.reg_hpc_a);
    d("op   0x%08x ",  insn_a); print_rv(d, insn_a); d("\n");
    d("\n");
  }

  {
    d.s.push_back(1);
    d("fetch\n");
    d.s.push_back(hart_b_col);
    d("pc   0x%08x\n", pinwheel.core.reg_hpc_b);
    d("op   0x%08x ",  pinwheel.core.reg_insn_b); print_rv(d, pinwheel.core.reg_insn_b); d("\n");
    d("r1   0x%08x\n", pinwheel.regs.get_rs1());
    d("r2   0x%08x\n", pinwheel.regs.get_rs2());
    const auto imm_b  = pinwheel.core.decode_imm(pinwheel.core.reg_insn_b);
    const auto addr_b = b32(pinwheel.regs.get_rs1() + imm_b);
    d("addr 0x%08x\n", addr_b);
    d("\n");
  }

  {
    d.s.push_back(1);
    d("mem/execute\n");
    d.s.push_back(hart_c_col);
    d("pc   0x%08x\n", pinwheel.core.reg_hpc_c);
    d("op   0x%08x ",  pinwheel.core.reg_insn_c); print_rv(d, pinwheel.core.reg_insn_c); d("\n");
    d("addr 0x%08x\n", pinwheel.core.reg_addr_c);
    d("res  0x%08x\n", pinwheel.core.reg_result_c);
    d("data 0x%08x\n", pinwheel.data_ram.get());
    d("\n");
  }

  {
    d.s.push_back(1);
    d("writeback\n");
    d.s.push_back(hart_d_col);
    d("pc   0x%08x\n", pinwheel.core.reg_hpc_d);
    d("op   0x%08x ",  pinwheel.core.reg_insn_d); print_rv(d, pinwheel.core.reg_insn_d); d("\n");
    if (pinwheel.core.sig_rf_wren) {
      d("r%02d  0x%08x\n", pinwheel.core.sig_rf_waddr, pinwheel.core.sig_rf_wdata);
    }
    else {
      d("rXX  0xXXXXXXXX\n");
    }
    d("\n");
  }

  text_painter.render_string(view, screen_size, d.s, 32, cursor_y);
  cursor_y += 480;
  d.clear();

  for (int hart = 0; hart < 4; hart++) {
    auto& r = pinwheel.regs;
    int c = hart << 5;
    d.s.push_back(1);
    d("hart %d", hart);
    d.s.push_back(hart + 2);
    d("\n");
    d("r00 %08X  r08 %08X  r16 %08X  r24 %08X\n", r.get(c +  0), r.get(c +  8), r.get(c + 16), r.get(c + 24));
    d("r01 %08X  r09 %08X  r17 %08X  r25 %08X\n", r.get(c +  1), r.get(c +  9), r.get(c + 17), r.get(c + 25));
    d("r02 %08X  r10 %08X  r18 %08X  r26 %08X\n", r.get(c +  2), r.get(c + 10), r.get(c + 18), r.get(c + 26));
    d("r03 %08X  r11 %08X  r19 %08X  r27 %08X\n", r.get(c +  3), r.get(c + 11), r.get(c + 19), r.get(c + 27));
    d("r04 %08X  r12 %08X  r20 %08X  r28 %08X\n", r.get(c +  4), r.get(c + 12), r.get(c + 20), r.get(c + 28));
    d("r05 %08X  r13 %08X  r21 %08X  r29 %08X\n", r.get(c +  5), r.get(c + 13), r.get(c + 21), r.get(c + 29));
    d("r06 %08X  r14 %08X  r22 %08X  r30 %08X\n", r.get(c +  6), r.get(c + 14), r.get(c + 22), r.get(c + 30));
    d("r07 %08X  r15 %08X  r23 %08X  r31 %08X\n", r.get(c +  7), r.get(c + 15), r.get(c + 23), r.get(c + 31));

    text_painter.render_string(view, screen_size, d.s, 32, cursor_y);

    logic<24> hart_pc = b24(hart_pcs[hart]);
    d.clear();
    d.s.push_back(1);

    d("disassembly:\n");

    for (int i = -2; i <= 5; i++) {
      int offset = (hart_pc & 0xFFFF) + (i * 4);
      int op = 0;

      if (offset < 0) op = 0;
      else if (offset > (65536 - 4)) op = 0;
      else op = pinwheel.code_ram.get_data()[offset >> 2];

      d("%c0x%08x ", i == 0 ? '>' : ' ', hart_pc + (i * 4));
      print_rv(d, op);
      d("\n");
    }

    text_painter.render_string(view, screen_size, d.s, 32 + 256 + 96, cursor_y);

    cursor_y += 128;
    d.clear();
  }

  //pinwheel.data_ram.get_data()[16383] = 0xFFFFFFFF;
  //pinwheel.code_ram.get_data()[12000] = 0xFFFFFFFF;

  code_painter.highlight_x = ((b24(pinwheel.core.reg_hpc_b) & 0xFFFF) >> 2) % 16;
  code_painter.highlight_y = ((b24(pinwheel.core.reg_hpc_b) & 0xFFFF) >> 2) / 16;
  code_painter.dump(view, screen_size, 1024, 1024 - 128 - 32, 0.25, 0.25, 64, 64, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.code_ram.get_data());

  uint8_t* data_base = (uint8_t*)pinwheel.data_ram.get_data();

  text_painter.render_string(view, screen_size, "First 2K of RAM", 1024, 32 - 12);
  data_painter.dump(view, screen_size, 1024, 32,  1, 1, 64, 32, vec4(0.0, 0.0, 0.0, 0.4), data_base);

  text_painter.render_string(view, screen_size, "Lat 2K of RAM", 1024, 256 + 128 + 64 - 12);
  data_painter.dump(view, screen_size, 1024, 256 + 128 + 64, 1, 1, 64, 32, vec4(0.0, 0.0, 0.0, 0.4), data_base + 65536 - (64*32));

  console_painter.dump(view, screen_size, 32*19,  32, 1, 1, pinwheel_sim->console1.width, pinwheel_sim->console1.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_sim->console1.buf);
  console_painter.dump(view, screen_size, 32*19, 256, 1, 1, pinwheel_sim->console2.width, pinwheel_sim->console2.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_sim->console2.buf);
  console_painter.dump(view, screen_size, 32*19, 480, 1, 1, pinwheel_sim->console3.width, pinwheel_sim->console3.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_sim->console3.buf);
  console_painter.dump(view, screen_size, 32*19, 704, 1, 1, pinwheel_sim->console4.width, pinwheel_sim->console4.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_sim->console4.buf);

  //box_painter.push_corner_size(1024 + (harts[0]->pc % 64) * 14 - 1, 512 + (harts[0]->pc / 64) * 12, 12*4+2*3+2, 12, 0x8000FFFF);
  //box_painter.push_corner_size(1024 + (harts[1]->pc % 64) * 14 - 1, 512 + (harts[1]->pc / 64) * 12, 12*4+2*3+2, 12, 0x80FFFF00);
  //box_painter.push_corner_size(1024 + (harts[2]->pc % 64) * 14 - 1, 512 + (harts[2]->pc / 64) * 12, 12*4+2*3+2, 12, 0x80FF00FF);
  //box_painter.render(view, screen_size, 0, 0);

  sim_thread->resume();
}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_ui(dvec2 screen_size, double delta) {

}

//------------------------------------------------------------------------------
