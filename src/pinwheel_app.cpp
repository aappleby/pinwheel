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

//#include <algorithm>

#include <elf.h>
#include <sys/stat.h>

//------------------------------------------------------------------------------

PinwheelApp::PinwheelApp() {
}

//------------------------------------------------------------------------------

const char* PinwheelApp::app_get_title() {
  return "Pinwheel Test App";
}

//------------------------------------------------------------------------------

void PinwheelApp::app_init(int screen_w, int screen_h) {
  pinwheel_sim = new PinwheelSim();
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

  /*
  uint8_t* dontcare = (uint8_t*)(&p);
  for (int i = 0; i < sizeof(p); i++) {
    dontcare[i] = rand();
  }
  */

  //const char* firmware_filename = "firmware/bin/hello";
  const char* firmware_filename = "microtests/bin/read_regs";

  LOG_G("Loading firmware %s...\n", firmware_filename);
  struct stat sb;
  if (stat(firmware_filename, &sb) == -1) {
    LOG_R("Could not stat firmware %s\n", firmware_filename);
  }
  else {
    LOG_G("Firmware is %d bytes\n", sb.st_size);
    uint8_t* blob = new uint8_t[sb.st_size];
    FILE* f = fopen(firmware_filename, "rb");
    auto result = fread(blob, 1, sb.st_size, f);
    fclose(f);

    Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;
    for (int i = 0; i < header.e_phnum; i++) {
      Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
      if (phdr.p_type & PT_LOAD) {
        if (phdr.p_flags & PF_X) {
          LOG_G("Code @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
          int len = sizeof(p.code_ram.data) < phdr.p_filesz ? sizeof(p.code_ram.data) : phdr.p_filesz;
          memcpy(p.code_ram.data, blob + phdr.p_offset, len);
          //put_cache("rv_tests/firmware.text.vh", blob + phdr.p_offset, phdr.p_filesz);
        }
        else if (phdr.p_flags & PF_W) {
          LOG_G("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
          int len = sizeof(p.data_ram.data) < phdr.p_filesz ? sizeof(p.data_ram.data) : phdr.p_filesz;
          memcpy(p.data_ram.data, blob + phdr.p_offset, len);
          //put_cache("rv_tests/firmware.data.vh", blob + phdr.p_offset, phdr.p_filesz);
        }
      }
    }
  }


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


  while (SDL_PollEvent(&event)) {


    if (event.type == SDL_KEYDOWN)
    switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        view_control.view_target      = Viewport::screenspace(screen_size);
        view_control.view_target_snap = Viewport::screenspace(screen_size);
        break;
      case SDLK_RIGHT:  {
        sim_thread->pause();
        pinwheel_sim->states.push();

        int steps = 1;
        auto keyboard_state = SDL_GetKeyboardState(nullptr);
        if (keyboard_state[SDL_SCANCODE_LSHIFT])  steps *= 100;
        if (keyboard_state[SDL_SCANCODE_LALT])    steps *= 10;
        if (keyboard_state[SDL_SCANCODE_LCTRL])   steps *= 3;
        pinwheel_sim->steps += steps;

        sim_thread->resume();
        break;
      }
      case SDLK_LEFT: {
        sim_thread->pause();
        pinwheel_sim->states.pop();
        sim_thread->resume();
        break;
      }
      case SDLK_SPACE: {
        sim_thread->pause();
        pinwheel_sim->steps = 0;
        sim_thread->resume();
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

  fflush(stdout);
}

//------------------------------------------------------------------------------

void PinwheelApp::app_render_frame(dvec2 screen_size, double delta)  {
  sim_thread->pause();

  auto& view = view_control.view_smooth_snap;
  grid_painter.render(view, screen_size);

  StringDumper d;

  auto& pinwheel = pinwheel_sim->states.top();

  uint32_t insn_a = b24(pinwheel.core.hpc_a) ? uint32_t(pinwheel.code_ram.rdata()) : 0;

  d("hpc_a         0x%08x\n", pinwheel.core.hpc_a);
  d("insn_a        0x%08x ",  insn_a); print_rv(d, insn_a); d("\n");
  d("rs1 a         %d\n", b5(insn_a, 15));
  d("rs2 a         %d\n", b5(insn_a, 20));
  d("\n");

  d("hpc_b         0x%08x\n", pinwheel.core.hpc_b);
  d("insn_b        0x%08x ",  pinwheel.core.insn_b); print_rv(d, pinwheel.core.insn_b); d("\n");
  d("rs1 b         0x%08x\n", pinwheel.core.regs.get_rs1());
  d("rs2 b         0x%08x\n", pinwheel.core.regs.get_rs2());

  const auto imm_b  = pinwheel.core.decode_imm(pinwheel.core.insn_b);
  const auto addr_b = b32(pinwheel.core.regs.get_rs1() + imm_b);
  d("addr b        0x%08x\n", addr_b);

  d("\n");

  d("hpc c         0x%08x\n", pinwheel.core.hpc_c);
  d("insn c        0x%08x ",  pinwheel.core.insn_c); print_rv(d, pinwheel.core.insn_c); d("\n");
  d("addr c        0x%08x\n", pinwheel.core.addr_c);
  d("result c      0x%08x\n", pinwheel.core.result_c);
  d("data c        0x%08x\n", pinwheel.data_ram.rdata());
  d("\n");

  d("hpc d         0x%08x\n", pinwheel.core.hpc_d);
  d("insn d        0x%08x ",  pinwheel.core.insn_d); print_rv(d, pinwheel.core.insn_d); d("\n");
  d("wb addr d     0x%08x\n", pinwheel.core.wb_addr_d);
  d("wb data d     0x%08x\n", pinwheel.core.wb_data_d);
  d("wb wren d     0x%08x\n", pinwheel.core.wb_wren_d);
  d("\n");

  d("debug_reg     0x%08x\n", pinwheel.debug_reg);
  d("ticks         %lld\n",   pinwheel.core.ticks);
  d("speed         %f\n",     double(sim_thread->sim_steps) / sim_thread->sim_time);
  d("states        %d\n",     pinwheel_sim->states.state_count());
  d("state bytes   %d\n",     pinwheel_sim->states.state_size_bytes());

  d("\n");

  for (int hart = 0; hart < 4; hart++) {
    auto r = &pinwheel.core.regs.get_data()[hart << 5];
    d("hart %d", hart);
    d("\n");
    d("r00 %08X  r08 %08X  r16 %08X  r24 %08X\n", r[ 0], r[ 8], r[16], r[24]);
    d("r01 %08X  r09 %08X  r17 %08X  r25 %08X\n", r[ 1], r[ 9], r[17], r[25]);
    d("r02 %08X  r10 %08X  r18 %08X  r26 %08X\n", r[ 2], r[10], r[18], r[26]);
    d("r03 %08X  r11 %08X  r19 %08X  r27 %08X\n", r[ 3], r[11], r[19], r[27]);
    d("r04 %08X  r12 %08X  r20 %08X  r28 %08X\n", r[ 4], r[12], r[20], r[28]);
    d("r05 %08X  r13 %08X  r21 %08X  r29 %08X\n", r[ 5], r[13], r[21], r[29]);
    d("r06 %08X  r14 %08X  r22 %08X  r30 %08X\n", r[ 6], r[14], r[22], r[30]);
    d("r07 %08X  r15 %08X  r23 %08X  r31 %08X\n", r[ 7], r[15], r[23], r[31]);
    d("\n");
  }

  text_painter.render_string(view, screen_size, d.s.c_str(), 32, 32);

  logic<24> hart0_pc = b24(pinwheel.core.hpc_a) ? b24(pinwheel.core.hpc_a) : b24(pinwheel.core.hpc_b);
  {
    d.clear();

    for (int i = -4; i <= 16; i++) {
      int offset = (hart0_pc & 0xFFFF) + (i * 4);
      int op = 0;

      if (offset < 0) op = 0;
      else if (offset > (65536 - 4)) op = 0;
      else op = pinwheel.code_ram.get_data()[offset >> 2];

      d("%c0x%08x ", i == 0 ? '>' : ' ', hart0_pc + (i * 4));
      print_rv(d, op);
      d("\n");
    }

    text_painter.render_string(view, screen_size, d.s.c_str(), 320, 32);
  }

  code_painter.highlight_x = ((b24(pinwheel.core.hpc_b) & 0xFFFF) >> 2) % 16;
  code_painter.highlight_y = ((b24(pinwheel.core.hpc_b) & 0xFFFF) >> 2) / 16;
  code_painter.dump2(view, screen_size, 1024, 1024 - 128 - 32, 0.25, 0.25, 64, 64, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.code_ram.get_data());

  data_painter.dump2(view, screen_size, 1024, 32, 1, 1, 64, 64, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.data_ram.get_data());

  console_painter.dump2(view, screen_size, 32*19,  32, 1, 1, Console::width, Console::height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.console1.buf);
  console_painter.dump2(view, screen_size, 32*19, 256, 1, 1, Console::width, Console::height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.console2.buf);
  console_painter.dump2(view, screen_size, 32*19, 480, 1, 1, Console::width, Console::height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.console3.buf);
  console_painter.dump2(view, screen_size, 32*19, 704, 1, 1, Console::width, Console::height, vec4(0.0, 0.0, 0.0, 0.4), (uint8_t*)pinwheel.console4.buf);

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
