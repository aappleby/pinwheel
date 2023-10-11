#include "sim_thread.h"

#include "metrolib/src/CoreLib/Check.h"
#include "metrolib/src/CoreLib/Log.h"
#include "metrolib/src/CoreLib/Utils.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

//------------------------------------------------------------------------------

SimThread::SimThread(Sim* prototype) : sim(prototype)
{
}

//----------------------------------------

void SimThread::start() {
  LOG_B("SimThread::start()\n");
  CHECK_P(main == nullptr);
  main = new std::thread([this] { thread_main(); });
  LOG_B("SimThread::start() done\n");
}

//----------------------------------------

void SimThread::stop() {
  LOG_B("SimThread::stop()\n");
  if (main) {
    sync.set(REQ_EXIT);
    main->join();
    delete main;
  }
  LOG_B("SimThread::stop() done\n");
}

//----------------------------------------

void SimThread::pause() {
  //LOG_B("SimThread::pause()\n");
  if (main) {
    std::unique_lock<std::mutex> lock(sync.mut);
    sync.set(REQ_PAUSE);
    sync.cond.wait(lock, [this] {
      return sync.check(ACK_PAUSE);
    });
  }
  //LOG_B("SimThread::pause() done\n");
}

//----------------------------------------

void SimThread::resume() {
  //LOG_B("SimThread::resume()\n");
  if (main) {
    sync.clear(REQ_PAUSE);
  }
  //LOG_B("SimThread::resume() done\n");
}

//----------------------------------------

bool SimThread::is_paused() const {
  return (main == nullptr) || sync.check(ACK_PAUSE);
}

//------------------------------------------------------------------------------

void SimThread::thread_main() {
  LOG_B("SimThread::thread_main()\n");

  while (1) {
    // If the app has requested a pause or we're out of work, halt until
    // we get work, we're unpaused, or we're exiting
    {
      std::unique_lock<std::mutex> lock(sync.mut);
      if (sync.check(REQ_PAUSE) || !sim->busy()) {
        sync.set(ACK_PAUSE);
        //LOG_B("SimThread waiting for work\n");
        sync.cond.wait(lock, [this] {
          return sync.check(REQ_EXIT) || (!sync.check(REQ_PAUSE) && sim->busy());
        });
        sync.clear(ACK_PAUSE);
        if (sim->busy()) {
          //LOG_B("SimThread got work\n");
        }
        else {
          //LOG_B("SimThread got exit request\n");
        }

      }

      if (sync.check(REQ_EXIT)) {
        sync.set(ACK_EXIT);
        break;
      }
    }

    // Run the sim until we get a break signal or until the sim is idle.
    if (sim->busy()) {
      //LOG_B("SimThread doing work\n");
      double time_begin = timestamp();
      while (sim->busy() && !sync.check(REQ_PAUSE | REQ_EXIT)) {
        sim->step();
        sim_steps++;
      }
      double time_end = timestamp();
      sim_time += (time_end - time_begin);

      if (sim->busy()) {
        //LOG_B("SimThread paused work\n");
      }
      else {
        //LOG_B("SimThread finished work\n");
      }
    }
  }

  LOG_B("SimThread::thread_main() done\n");
}

//------------------------------------------------------------------------------
