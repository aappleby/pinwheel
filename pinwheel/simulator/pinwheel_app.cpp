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

inline static const char* tla_op_to_string(int op) {
  switch(op) {
    case TL::PutFullData:    return "PutFullData";
    case TL::PutPartialData: return "PutPartialData";
    case TL::ArithmeticData: return "ArithmeticData";
    case TL::LogicalData:    return "LogicalData";
    case TL::Get:            return "Get";
    case TL::Intent:         return "Intent";
    case TL::Acquire:        return "Acquire";
    case TL::Invalid:        return "<Invalid>";
  }
  return "<bad op>";
}

inline static const char* tld_op_to_string(int op) {
  switch(op) {
    case TL::AccessAck:      return "AccessAck";
    case TL::AccessAckData:  return "AccessAckData";
    case TL::HintAck:        return "HintAck";
    case TL::Grant:          return "Grant";
    case TL::GrantData:      return "GrantData";
    case TL::ReleaseAck:     return "ReleaseAck";
    case TL::Invalid:        return "<Invalid>";
  }
  return "<bad op>";
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
    //"gen/tests/rv_tests/add.code.vh",
    //"gen/tests/rv_tests/add.data.vh",
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

  auto& p = pinwheel_sim->states.top().soc;

  p.tock(true);
  p.tock(true);

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
      view_control.on_mouse_wheel(dvec2(mouse_x, mouse_y), screen_size, double(event.wheel.y) /** 0.25*/);
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

void dump_tla(StringDumper& d, const char* prefix, tilelink_a tla) {
  d("%s.a_opcode  %s\n",       prefix, tla_op_to_string(tla.a_opcode));
  //d("%s.a_param   %d\n",       prefix, tla.a_param);
  d("%s.a_size    %d\n",       prefix, tla.a_size);
  //d("%s.a_source  %d\n",       prefix, tla.a_source);
  d("%s.a_address 0x%08x\n",   prefix, tla.a_address);
  d("%s.a_mask    %c%c%c%c\n", prefix, tla.a_mask & 0b1000 ? '1' : '0', tla.a_mask & 0b0100 ? '1' : '0', tla.a_mask & 0b0010 ? '1' : '0', tla.a_mask & 0b0001 ? '1' : '0');
  d("%s.a_data    0x%08x\n",   prefix, tla.a_data);
  d("%s.a_valid   %d\n",       prefix, tla.a_valid);
  //d("%s.a_ready   %d\n",       prefix, tla.a_ready);
}

void dump_tld(StringDumper& d, const char* prefix, tilelink_d tld) {
  d("%s.d_opcode  %s\n",     prefix, tld_op_to_string(tld.d_opcode));
  //d("%s.d_param   %d\n",     prefix, tld.d_param);
  d("%s.d_size    %d\n",     prefix, tld.d_size);
  //d("%s.d_source  %d\n",     prefix, tld.d_source);
  //d("%s.d_sink    %d\n",     prefix, tld.d_sink);
  d("%s.d_data    0x%08x\n", prefix, tld.d_data);
  d("%s.d_error   %d\n",     prefix, tld.d_error);
  d("%s.d_valid   %d\n",     prefix, tld.d_valid);
  //d("%s.d_ready   %d\n",     prefix, tld.d_ready);
}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  sim_thread->pause();

  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);

  StringDumper d;

  auto& pinwheel = pinwheel_sim->states.top().soc;
  auto& pinwheel_tb = pinwheel_sim->states.top();

  uint32_t insn_a = b24(pinwheel.core.A_pc) ? uint32_t(pinwheel.code_ram.get()) : 0;

  auto thread_a = pinwheel.core.A_hart;
  auto thread_b = pinwheel.core.B_hart;
  auto thread_c = pinwheel.core.C_hart;
  //auto thread_d = pinwheel.core.D_hart;

  uint32_t thread_pcs[16] = {0};
  //if (pinwheel.core.D_pc) thread_pcs[thread_d] = pinwheel.core.D_pc;
  if (pinwheel.core.C_pc) thread_pcs[thread_c] = pinwheel.core.C_pc;
  if (pinwheel.core.B_pc) thread_pcs[thread_b] = pinwheel.core.B_pc;
  if (pinwheel.core.A_pc) thread_pcs[thread_a] = pinwheel.core.A_pc;

  auto thread_a_col = pinwheel.core.A_pc ? thread_a + 2 : 0;
  auto thread_b_col = pinwheel.core.B_pc ? thread_b + 2 : 0;
  auto thread_c_col = pinwheel.core.C_pc ? thread_c + 2 : 0;
  //auto thread_d_col = pinwheel.core.D_pc ? thread_d + 2 : 0;

  int cursor_x = 32;
  int cursor_y = 32;

  {
    d.s.push_back(1);
    d("debug_reg     0x%08x\n", pinwheel.debug_reg.get_tld().d_data);
    d("ticks         %lld\n",   pinwheel.core.ticks);
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
    d.s.push_back(1);
    d("decode\n");
    d.s.push_back(thread_a_col);
    d("pc   0x%08x\n", pinwheel.core.A_pc);
    d("op   0x%08x ",  insn_a); print_rv(d, insn_a); d("\n");
    d("\n");
  }

  {
    d.s.push_back(1);
    d("fetch\n");
    d.s.push_back(thread_b_col);
    d("pc   0x%08x\n", pinwheel.core.B_pc);
    d("op   0x%08x ",  pinwheel.core.B_insn.raw); print_rv(d, pinwheel.core.B_insn.raw); d("\n");
    d("r1   0x%08x\n", pinwheel.regs.get_rs1());
    d("r2   0x%08x\n", pinwheel.regs.get_rs2());
    const auto imm_b  = pinwheel.core.dbg_decode_imm(pinwheel.core.B_insn);
    const auto addr_b = b32(pinwheel.regs.get_rs1() + imm_b);
    d("addr 0x%08x\n", addr_b);
    d("\n");
  }

  {
    d.s.push_back(1);
    d("mem/execute\n");
    d.s.push_back(thread_c_col);
    d("pc   0x%08x\n", pinwheel.core.C_pc);
    d("op   0x%08x ",  pinwheel.core.C_insn.raw); print_rv(d, pinwheel.core.C_insn.raw); d("\n");
    d("addr 0x%08x\n", pinwheel.core.C_addr);
    d("res  0x%08x\n", pinwheel.core.C_result);
    d("data 0x%08x\n", pinwheel.data_ram.get());
    d("\n");
  }

  /*
  {
    d.s.push_back(1);
    d("writeback\n");
    d.s.push_back(thread_d_col);
    d("pc   0x%08x\n", pinwheel.core.D_pc);
    d("op   0x%08x ",  pinwheel.core.D_insn.raw); print_rv(d, pinwheel.core.D_insn.raw); d("\n");
    if (pinwheel.core.reg_if.wren) {
      d("r%02d  0x%08x\n", pinwheel.core.reg_if.waddr, pinwheel.core.reg_if.wdata);
    }
    else {
      d("rXX  0xXXXXXXXX\n");
    }
    d("\n");
  }
  */

  text_painter.render_string(view, screen_size, d.s, cursor_x, cursor_y);


  cursor_x += 256;
  d.clear();
  d.s.push_back(1);
  dump_tla(d, "code_bus", pinwheel.core.code_tla);
  d("\n");
  dump_tld(d, "code_bus", pinwheel.code_ram.get_tld());
  d("\n");

  dump_tla(d, "data_bus", pinwheel.core.data_tla);
  d("\n");
  dump_tld(d, "data_bus", pinwheel.get_data_tld());
  d("\n");

  text_painter.render_string(view, screen_size, d.s, cursor_x, cursor_y);




















  cursor_x = 32;
  cursor_y += 512;
  d.clear();

  for (int thread = 0; thread < 4; thread++) {
    auto& r = pinwheel.regs;
    int c = thread << 5;
    d.s.push_back(1);
    d("thread %d", thread);
    d.s.push_back(thread + 2);
    d("\n");
    d("r00 %08X  r08 %08X  r16 %08X  r24 %08X\n", r.get(c +  0), r.get(c +  8), r.get(c + 16), r.get(c + 24));
    d("r01 %08X  r09 %08X  r17 %08X  r25 %08X\n", r.get(c +  1), r.get(c +  9), r.get(c + 17), r.get(c + 25));
    d("r02 %08X  r10 %08X  r18 %08X  r26 %08X\n", r.get(c +  2), r.get(c + 10), r.get(c + 18), r.get(c + 26));
    d("r03 %08X  r11 %08X  r19 %08X  r27 %08X\n", r.get(c +  3), r.get(c + 11), r.get(c + 19), r.get(c + 27));
    d("r04 %08X  r12 %08X  r20 %08X  r28 %08X\n", r.get(c +  4), r.get(c + 12), r.get(c + 20), r.get(c + 28));
    d("r05 %08X  r13 %08X  r21 %08X  r29 %08X\n", r.get(c +  5), r.get(c + 13), r.get(c + 21), r.get(c + 29));
    d("r06 %08X  r14 %08X  r22 %08X  r30 %08X\n", r.get(c +  6), r.get(c + 14), r.get(c + 22), r.get(c + 30));
    d("r07 %08X  r15 %08X  r23 %08X  r31 %08X\n", r.get(c +  7), r.get(c + 15), r.get(c + 23), r.get(c + 31));

    text_painter.render_string(view, screen_size, d.s, cursor_x, cursor_y);

    logic<24> thread_pc = b24(thread_pcs[thread]);
    d.clear();
    d.s.push_back(1);

    d("disassembly:\n");

    for (int i = -2; i <= 5; i++) {
      int offset = (thread_pc & 0xFFFF) + (i * 4);
      int op = 0;

      if (offset < 0) op = 0;
      else if (offset > (65536 - 4)) op = 0;
      else op = pinwheel.code_ram.get_data()[offset >> 2];

      d("%c0x%08x ", i == 0 ? '>' : ' ', thread_pc + (i * 4));
      print_rv(d, op);
      d("\n");
    }

    text_painter.render_string(view, screen_size, d.s, cursor_x + 256 + 96, cursor_y);

    cursor_y += 128;
    d.clear();
  }

  code_painter.highlight_x = ((b24(pinwheel.core.B_pc) & 0xFFFF) >> 2) % 16;
  code_painter.highlight_y = ((b24(pinwheel.core.B_pc) & 0xFFFF) >> 2) / 16;
  text_painter.render_string(view, screen_size, "First 4K of code RAM", 1024, 1024 - 128 - 32 - 12);
  code_painter.dump(view, screen_size, 1024, 1024 - 128 - 32, 0.25, 0.25, 64, 64, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.code_ram.get_data());

  uint8_t* data_base = (uint8_t*)pinwheel.data_ram.get_data();
  uint8_t* data_end  = data_base + pinwheel.data_ram.get_size();

  text_painter.render_string(view, screen_size, "First 2K of RAM", 1024, 32 - 12);
  data_painter.dump(view, screen_size, 1024, 32,  1, 1, 64, 32, vec4(0.0, 0.0, 0.0, 0.4), data_base);

  text_painter.render_string(view, screen_size, "Last 2K of RAM", 1024, 256 + 128 + 64 - 12);
  data_painter.dump(view, screen_size, 1024, 256 + 128 + 64, 1, 1, 64, 32, vec4(0.0, 0.0, 0.0, 0.4), data_end - (64*32));

  console_painter.dump(view, screen_size, 32*19,  32, 1, 1, pinwheel_tb.console1.width, pinwheel_tb.console1.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_tb.console1.buf);
  console_painter.dump(view, screen_size, 32*19, 256, 1, 1, pinwheel_tb.console2.width, pinwheel_tb.console2.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_tb.console2.buf);
  console_painter.dump(view, screen_size, 32*19, 480, 1, 1, pinwheel_tb.console3.width, pinwheel_tb.console3.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_tb.console3.buf);
  console_painter.dump(view, screen_size, 32*19, 704, 1, 1, pinwheel_tb.console4.width, pinwheel_tb.console4.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_tb.console4.buf);

  text_painter.render_string(view, screen_size, "UART", 1024 + 256, 704 + 128 + 32 - 12);
  console_painter.dump(view, screen_size, 1024 + 256, 704 + 128 + 32, 1, 1, pinwheel_tb.console5.width, pinwheel_tb.console5.height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel_tb.console5.buf);

  sim_thread->resume();
}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_ui(dvec2 screen_size, double delta) {

}

//------------------------------------------------------------------------------
