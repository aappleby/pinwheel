#include "AppLib/App.h"

#include "MetroLib/src/AppLib/GridPainter.h"
#include "MetroLib/src/AppLib/BoxPainter.h"
#include "MetroLib/src/AppLib/TextPainter.h"
#include "MetroLib/src/AppLib/DumpPainter.h"
#include "MetroLib/src/AppLib/Viewport.h"
#include "MetroLib/src/CoreLib/StateStack.h"

#include "pinwheel.h"
#include "pinwheel_sim.h"
#include "sim_thread.h"

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
};
