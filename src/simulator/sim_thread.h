#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

//--------------------------------------------------------------------------------

struct AtomicFlags {

  void set(int mask) {
    flags |= mask;
    cond.notify_all();
  }

  void clear(int mask) {
    flags &= ~mask;
    cond.notify_all();
  }

  int check(int mask) const {
    return (flags & mask);
  }

  std::atomic_int flags;
  std::mutex mut;
  std::condition_variable cond;
};

//--------------------------------------------------------------------------------

struct Sim {
  virtual bool busy() const = 0;
  virtual void step() = 0;
};

//--------------------------------------------------------------------------------

struct SimThread {

  SimThread(Sim* prototype);

  void start();
  void stop();
  void pause();
  void resume();
  bool is_paused() const;

  const int REQ_PAUSE = 0b0001;
  const int ACK_PAUSE = 0b0010;
  const int REQ_EXIT  = 0b0100;
  const int ACK_EXIT  = 0b1000;

//private:

  void thread_main();

  Sim* sim = nullptr;
  double sim_time = 0;
  int64_t sim_steps = 0;
  std::thread* main = nullptr;
  AtomicFlags sync;
};

//--------------------------------------------------------------------------------
