#include "gdb_server.h"

//------------------------------------------------------------------------------

#ifdef __riscv

#define LOG_R(...)
#define LOG_G(...)
#define LOG_B(...)

#else

#include <stdio.h>

#define LOG_R(...) set_color(0xAAAAFF); printf(__VA_ARGS__); fflush(stdout); clear_color();
#define LOG_G(...) set_color(0xAAFFAA); printf(__VA_ARGS__); fflush(stdout); clear_color();
#define LOG_B(...) set_color(0xFFAAAA); printf(__VA_ARGS__); fflush(stdout); clear_color();

void set_color(unsigned int color) {
  printf("\u001b[38;2;%d;%d;%dm", (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
}

void clear_color() {
  printf("\u001b[0m");
}

#endif

//------------------------------------------------------------------------------

char to_hex(int x) {
  if (x >= 0  && x <= 9)  return '0' + x;
  if (x >= 10 && x <= 15) return 'A' - 10 + x;
  return '?';
}

int from_hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  return -1;
}

//------------------------------------------------------------------------------

int cmp(const char* prefix, const char* text) {
  while(1) {
    // Note we return when we run out of matching prefix, not when both
    // strings match.
    if (*prefix == 0)    return 0;
    if (*prefix > *text) return -1;
    if (*prefix < *text) return 1;
    prefix++;
    text++;
  }
}

//------------------------------------------------------------------------------

int _atoi(const char* cursor) {
  int sign = 1;
  if (*cursor == '-') {
    sign = -1;
    cursor++;
  }

  int accum = 0;
  while(*cursor) {
    if (*cursor < '0' || *cursor > '9') break;
    accum *= 10;
    accum += *cursor++ - '0';
  }
  return sign * accum;
}

//------------------------------------------------------------------------------
/*
At a minimum, a stub is required to support the ‘?’ command to tell GDB the
reason for halting, ‘g’ and ‘G’ commands for register access, and the ‘m’ and
‘M’ commands for memory access. Stubs that only control single-threaded targets
can implement run control with the ‘c’ (continue) command, and if the target
architecture supports hardware-assisted single-stepping, the ‘s’ (step) command.
Stubs that support multi-threading targets should support the ‘vCont’ command.
All other commands are optional.
*/


// Keep sorted in ASCII order
const GDBServer::handler GDBServer::handler_tab[] = {
  { "?",               &GDBServer::handle_questionmark },
  { "D",               &GDBServer::send_ok },
  { "H",               &GDBServer::handle_H },
  { "g",               &GDBServer::handle_g },
  // 'k' always kills the target and explicitly does _not_ have a reply.
  { "k",               &GDBServer::send_nothing },
  { "p",               &GDBServer::handle_p },
  { "qAttached",       &GDBServer::handle_qAttached },
  { "qC",              &GDBServer::handle_qC },
  { "qL",              &GDBServer::handle_qL },
  { "qOffsets",        &GDBServer::send_empty },
  { "qSupported",      &GDBServer::send_empty },
  { "qSymbol",         &GDBServer::send_ok },
  { "qTStatus",        &GDBServer::handle_qTStatus },
  { "qTfP",            &GDBServer::send_empty },
  { "qTfV",            &GDBServer::send_empty },
  { "qfThreadInfo",    &GDBServer::handle_qfThreadInfo },
  { "qsThreadInfo",    &GDBServer::handle_qsThreadInfo },
  { "vKill",           &GDBServer::send_ok },
  { "vMustReplyEmpty", &GDBServer::send_empty },
};

const int GDBServer::handler_count = sizeof(GDBServer::handler_tab) / sizeof(GDBServer::handler_tab[0]);

//------------------------------------------------------------------------------

void GDBServer::put_byte(char b) {
  send_checksum += b;
  _put_byte(b);
}

char GDBServer::get_byte() {
  return _get_byte();
}

//------------------------------------------------------------------------------

void GDBServer::packet_start() {
  put_byte('$');
  send_checksum = 0;
}

void GDBServer::packet_data(const char* buf, int len) {
  for (int i = 0; i < len; i++) {
    put_byte(packet[i]);
  }
}

void GDBServer::packet_string(const char* buf) {
  while(*buf) {
    put_byte(*buf++);
  }
}

void GDBServer::packet_u8(char x) {
  put_byte(to_hex((x >> 4) & 0xF));
  put_byte(to_hex((x >> 0) & 0xF));
}

void GDBServer::packet_u32(int x) {
  for(int i = 0; i < 4; i++) {
    put_byte(x);
    x = x >> 8;
  }
}

void GDBServer::packet_end() {
  char checksum = send_checksum;
  put_byte('#');
  packet_u8(checksum);
}

bool GDBServer::wait_packet_ack() {
  char c = 0;
  do { c = get_byte(); }
  while (c != '+' && c != '-');
  if (c == '-') {
    LOG_R("==============================\n");
    LOG_R("===========  NACK  ===========\n");
    LOG_R("==============================\n");
  }
  return c == '+';
}

//------------------------------------------------------------------------------

void GDBServer::send_packet(const char* packet) {
  LOG_B("<<== \"%s\"\n", packet);
  while(1) {
    packet_start();
    packet_string(packet);
    packet_end();
    if (wait_packet_ack()) break;
  }
}

void GDBServer::send_ack()     { put_byte('+'); }
void GDBServer::send_nack()    { put_byte('-'); }
void GDBServer::send_ok()      { send_packet("OK"); }
void GDBServer::send_empty()   { send_packet(""); }
void GDBServer::send_nothing() { }

//------------------------------------------------------------------------------

void GDBServer::handle_questionmark() {
  //  SIGINT = 2
  send_packet("T02");
}

void GDBServer::handle_H() {
  packet_cursor++;
  int thread_id = _atoi(packet_cursor);
  if (thread_id == 0 || thread_id == 1) {
    send_ok();
  }
  else {
    send_packet("E01");
  }
}

void GDBServer::handle_g() {
  packet_start();
  for (int i = 0; i < 32; i++) {
    packet_u32(0xF00D0000 + i);
  }
  packet_end();
}

void GDBServer::handle_p() {
  packet_cursor++;
  // FIXME are we sure this is hex? I think it is...
  int hi = from_hex(*packet_cursor++);
  int lo = from_hex(*packet_cursor++);
  packet_start();
  packet_u32(0xF00D0000 + ((hi << 4) | lo));
  packet_end();
}

void GDBServer::handle_qAttached() {
  // -> qAttached
  // Reply:
  // ‘1’ The remote server attached to an existing process.
  // ‘0’ The remote server created a new process.
  // ‘E NN’ A badly formed request or an error was encountered.
  send_packet("1");
}

void GDBServer::handle_qC() {
  // -> qC
  // Return current thread ID
  // Reply: ‘QC thread-id’
  send_packet("QC 1");
}

void GDBServer::handle_qL() {
  // FIXME get arg
  // qL1200000000000000000
  // qL 1 20 00000000 00000000
  // not sure if this is correct.
  send_packet("qM 01 1 00000000 00000001");
}

void GDBServer::handle_qTStatus() {
  send_packet("T0");
}

void GDBServer::handle_qfThreadInfo() {
  // Only one thread with id 1.
  send_packet("m 1");
}

void GDBServer::handle_qsThreadInfo() {
  // Only one thread with id 1.
  send_packet("l");
}

//------------------------------------------------------------------------------

void GDBServer::dispatch_command() {
  for (int i = 0; i < handler_count; i++) {
    if (cmp(handler_tab[i].name, packet) == 0) {
      (*this.*handler_tab[i].handler)();
      return;
    }
  }

  LOG_R("   No handler for command %s\n", packet);
  send_empty();
}

//------------------------------------------------------------------------------

void GDBServer::loop() {
  while(1) {
    packet_size = 0;
    packet_cursor = packet;
    char c;

    char expected_checksum = 0;
    char actual_checksum = 0;

    while (1) {
      c = get_byte();
      if (c == '$') {
        break;
      }
    }

    while (1) {
      c = get_byte();
      if (c == '#') {
        packet[packet_size] = 0;
        break;
      }
      else {
        actual_checksum += c;
        packet[packet_size++] = c;
      }
    }

    for (int digits = 0; digits < 2; digits++) {
      while (1) {
        c = get_byte();
        int d = from_hex(c);
        if (d == -1) {
          LOG_R("bad hex %c\n", c);
          continue;
        }
        else {
          expected_checksum = (expected_checksum << 4) | d;
          break;
        }
      }
    }

    if (actual_checksum != expected_checksum) {
      LOG_R("==>> %s\n", packet);
      LOG_R("Packet transmission error\n");
      LOG_R("expected checksum 0x%02x\n", expected_checksum);
      LOG_R("actual checksum   0x%02x\n", actual_checksum);
      send_nack();
      continue;
    } else {
      LOG_G("==>> %s\n", packet);
      send_ack();
      dispatch_command();
    }
  }
}

//------------------------------------------------------------------------------
