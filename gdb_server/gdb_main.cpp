#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <stdint.h>



#define LOG_R(...) set_color(0xAAAAFF); printf(__VA_ARGS__); fflush(stdout); clear_color();
#define LOG_G(...) set_color(0xAAFFAA); printf(__VA_ARGS__); fflush(stdout); clear_color();
#define LOG_B(...) set_color(0xFFAAAA); printf(__VA_ARGS__); fflush(stdout); clear_color();

void set_color(uint32_t color) {
  printf("\u001b[38;2;%d;%d;%dm", (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
}

void clear_color() {
  printf("\u001b[0m");
}



/*
At a minimum, a stub is required to support the ‘?’ command to tell GDB the
reason for halting, ‘g’ and ‘G’ commands for register access, and the ‘m’ and
‘M’ commands for memory access. Stubs that only control single-threaded targets
can implement run control with the ‘c’ (continue) command, and if the target
architecture supports hardware-assisted single-stepping, the ‘s’ (step) command.
Stubs that support multi-threading targets should support the ‘vCont’ command.
All other commands are optional.
*/

const char* gdb_command = "+$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+;memory-tagging+;xmlRegisters=i386#77";

struct GDBServer;

typedef std::function<void(GDBServer&)> handler_func;

struct handler {
  const char* name;
  handler_func handler;
};

extern const handler handler_tab[];
extern const int handler_count;

struct GDBServer {

  int serial_fd = 0;

  enum {
    WAIT_FOR_START,
    RECV_COMMAND,
    RECV_ARGS,
    RECV_CHECKSUM1,
    RECV_CHECKSUM2,
    SEND_ACK,
    SEND_PACKET,
    WAIT_ACK,
  };

  int state = WAIT_FOR_START;
  uint8_t expected_checksum = 0;
  uint8_t actual_checksum = 0;

  char  packet[512];
  int   packet_size = 0;
  char* packet_cursor = 0;

  //----------------------------------------------------------------------------

  GDBServer(const char* pty_path) {
    serial_fd = open(pty_path, O_RDWR | O_NOCTTY);
    printf("File descriptor is %d\n", serial_fd);
  }

  ~GDBServer() {
    close(serial_fd);
    serial_fd = 0;
  }

  //----------------------------------------------------------------------------

  char rbuf[256];
  int rcursor = 0;
  int rsize = 0;

  uint8_t get_byte() {
    if (rcursor == rsize) {
      rcursor = 0;
      rsize = 0;
      pollfd fds = {serial_fd, POLLIN, 0};
      //LOG_G("poll\n");
      auto poll_result = poll(&fds, 1, 10000);
      //LOG_G("read\n");
      rsize = read(serial_fd, rbuf, sizeof(rbuf));
      //LOG_G("done\n");
    }

    // Exit app if we can't read a byte
    if (rsize == 0) {
      LOG_R("Failed to get a byte, exiting\n");
      exit(0);
    }

    auto result = rbuf[rcursor++];
    //printf("%c", result);
    return result;
  }

  void put_byte(uint8_t b) {
    write(serial_fd, &b, 1);
  }

  //----------------------------------------------------------------------------

  void handle_extended() {
  }

  // -> ?
  void handle_questionmark() {
    //  SIGINT = 2
    send_packet("T02");
  }

  void handle_Hc() {
    // -> Hc-1
    // Step thread -1?
    // 'OK' for success
    // ‘E NN’ for an error
    send_packet("E01");
  }

  // -> Hg0 - set thread id?
  void handle_Hg() {
    send_packet("OK");
  }

  void handle_qAttached() {
    // -> qAttached
    // Reply:
    // ‘1’ The remote server attached to an existing process.
    // ‘0’ The remote server created a new process.
    // ‘E NN’ A badly formed request or an error was encountered.
    send_packet("1");
  }

  void handle_qC() {
    // -> qC
    // Return current thread ID
    // Reply: ‘QC thread-id’
    send_packet("QC 1");
  }

  void handle_qL() {
    // FIXME get arg
    // qL1200000000000000000
    // qL 1 20 00000000 00000000
    // not sure if this is correct.
    send_packet("qM 01 1 00000000 00000001");
  }

  void handle_qSupported() {
    send_packet("");
  }

  void handle_qTStatus() {
    send_packet("T0");
  }

  void handle_qTfV() {
    send_packet("");
  }

  void handle_qfThreadInfo() {
    // Only one thread with id 1.
    send_packet("m 1");
  }

  void handle_qsThreadInfo() {
    // Only one thread with id 1.
    send_packet("l");
  }

  void handle_vMustReplyEmpty() {
    send_packet("");
  }

  //----------------------------------------------------------------------------

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

  //----------------------------------------------------------------------------

  void dispatch_command() {
    int cursor = 0;
    int mask = 0x100;
    bool match = false;

    do {
      mask >>= 1;
      cursor |= mask;
      if (cursor >= handler_count) {
        cursor &= ~mask;
        continue;
      }

      int c = cmp(handler_tab[cursor].name, packet);
      if      (c ==  0) { match = true; break; }
      else if (c == -1) { cursor &= ~mask; }
    } while(mask);


    if (match) {
      //printf("handle %s with %s\n", packet, handler_tab[cursor].name);
      handler_tab[cursor].handler(*this);
    }
    else {
      LOG_R("   No handler for command %s\n", packet);
      send_packet("");
    }
  }

  //----------------------------------------------------------------------------

  void send_packet(const char* packet) {
    LOG_B("<<== \"%s\"\n", packet);
    while(1) {
      put_byte('$');
      uint8_t checksum = 0;
      for (auto cursor = packet; *cursor; cursor++) {
        put_byte(*cursor);
        checksum += *cursor;
      }
      put_byte('#');
      put_byte(to_hex((checksum >> 4) & 0xF));
      put_byte(to_hex((checksum >> 0) & 0xF));

      char c = 0;
      do { c = get_byte(); }
      while (c != '+' && c != '-');
      if (c == '+') break;
    }
  }

  void send_ack() {
    //LOG_G("<<== +\n");
    put_byte('+');
  }

  void send_nack() {
    //LOG_R("<<== +\n");
    put_byte('-');
  }

  //----------------------------------------------------------------------------

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

  //----------------------------------------------------------------------------

  void loop() {
    while(1) {
      packet_size = 0;
      packet_cursor = packet;
      expected_checksum = 0;
      actual_checksum = 0;
      char c;

      //printf("Waiting for '$'\n");
      while (1) {
        c = get_byte();
        if (c == '$') {
          //printf("Got '$'\n");
          break;
        }
      }
      //printf("Waiting for '#'\n");
      while (1) {
        c = get_byte();
        if (c == '#') {
          //printf("Got '#'\n");
          packet[packet_size] = 0;
          break;
        }
        else {
          actual_checksum += c;
          packet[packet_size++] = c;
        }
      }
      //printf("Waiting for first hex digit\n");
      while (1) {
        c = get_byte();
        int d = from_hex(c);
        //printf("c %c d %d\n", c, d);
        if (d == -1) {
          printf("bad hex %c\n", c);
          continue;
        }
        else {
          expected_checksum = expected_checksum | (d << 4);
          break;
        }
      }
      //printf("Waiting for second hex digit\n");
      while (1) {
        c = get_byte();
        int d = from_hex(c);
        if (d == -1) {
          printf("bad hex %c\n", c);
          continue;
        }
        else {
          expected_checksum = expected_checksum | (d << 0);
          break;
        }
      }
      if (expected_checksum != actual_checksum) {
        LOG_R("==>> %s\n", packet);
        printf("Packet transmission error\n");
        printf("expected checksum 0x%02x\n", expected_checksum);
        printf("actual checksum   0x%02x\n", actual_checksum);
        send_nack();
        continue;
      } else {
        LOG_G("==>> %s\n", packet);
        send_ack();
        dispatch_command();
      }
    }
  }

  //----------------------------------------------------------------------------
};

// Keep sorted in ASCII order
const handler handler_tab[] = {
  { "?",               &GDBServer::handle_questionmark },
  { "Hc",              &GDBServer::handle_Hg },
  { "Hg",              &GDBServer::handle_Hg },
  { "qAttached",       &GDBServer::handle_qAttached },
  { "qC",              &GDBServer::handle_qC },
  { "qL",              &GDBServer::handle_qL },
  { "qSupported",      &GDBServer::handle_qSupported },
  { "qTStatus",        &GDBServer::handle_qTStatus },
  { "qTfV",            &GDBServer::handle_qTfV },
  { "qfThreadInfo",    &GDBServer::handle_qfThreadInfo },
  { "qsThreadInfo",    &GDBServer::handle_qsThreadInfo },
  { "vMustReplyEmpty", &GDBServer::handle_vMustReplyEmpty },
};

const int handler_count = sizeof(handler_tab) / sizeof(handler_tab[0]);

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("gdb_server <pty path>\n");
    return 1;
  }
  GDBServer s(argv[1]);

  s.loop();
  while (1) {
    LOG_B("%c", s.get_byte());
  }

  return 0;
}

//------------------------------------------------------------------------------
