#include "metrolib/appbase/App.h"
#include "metrolib/appbase/BoxPainter.h"
#include "metrolib/appbase/DumpPainter.h"
#include "metrolib/appbase/GridPainter.h"
#include "metrolib/appbase/TextPainter.h"
#include "metrolib/appbase/Viewport.h"
#include "metrolib/core/StateStack.h"

#include "pinwheel/soc/pinwheel_soc.h"
#include "pinwheel/simulator/pinwheel_sim.h"
#include "pinwheel/simulator/sim_thread.h"

struct PinwheelApp : public App {

  PinwheelApp();

  const char* app_get_title() override;
  void app_init(int screen_w, int screen_h) override;
  void app_close() override;
  bool pause_when_idle() override;

  void app_update(dvec2 screen_size, double delta) override;
  void app_render_frame(dvec2 screen_size, double delta) override;
  void app_render_ui(dvec2 screen_size, double delta) override;

  ViewController view_control;
  TextPainter    text_painter;
  GridPainter    grid_painter;
  DumpPainter    code_painter;
  DumpPainter    data_painter;

  DumpPainter    console_painter;
  BoxPainter     box_painter;

  PinwheelSim*   pinwheel_sim = nullptr;
  SimThread*     sim_thread = nullptr;
  bool running = false;
  bool fastmode = false;

  int64_t ticks_old = 0;
  int64_t ticks_new = 0;
  double  time_old = 0;
  double  time_new = 0;
};
