#pragma once

//------------------------------------------------------------------------------

struct GDBServer {
public:

  using putter = void (*)(char);
  using getter = char (*)();

  GDBServer(getter _get, putter _put) : _get_byte(_get), _put_byte(_put)  {}

  void put_byte(char b);
  char get_byte();

  void packet_start();
  void packet_data(const char* buf, int len);
  void packet_string(const char* buf);
  void packet_u32(int x);
  void packet_end();
  bool wait_packet_ack();

  void send_packet(const char* packet, int len);
  void send_packet(const char* packet);
  void send_ack();
  void send_nack();

  void handle_extended();
  void handle_questionmark();
  void handle_D();
  void handle_H();
  void handle_g();
  void handle_k();
  void handle_p();
  void handle_qAttached();
  void handle_qC();
  void handle_qL();
  void handle_qOffsets();
  void handle_qSupported();
  void handle_qSymbol();
  void handle_qTStatus();
  void handle_qTfP();
  void handle_qTfV();
  void handle_qfThreadInfo();
  void handle_qsThreadInfo();
  void handle_vKill();
  void handle_vMustReplyEmpty();

  void dispatch_command();
  void loop();

private:

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

  putter  _put_byte = nullptr;
  getter  _get_byte = nullptr;

  int     state = WAIT_FOR_START;
  int     serial_fd = 0;
  char    packet[512];
  int     packet_size = 0;
  char*   packet_cursor = 0;
  char send_checksum = 0;
};

//------------------------------------------------------------------------------
