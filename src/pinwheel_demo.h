#include "AppLib/App.h"

#include "AppLib/GridPainter.h"
#include "AppLib/TextPainter.h"
#include "AppLib/Viewport.h"

struct PinwheelApp : public App {

  const char* app_get_title() override;
  void app_init(int screen_w, int screen_h) override;
  void app_close() override;
  bool pause_when_idle() override;

  void app_update(dvec2 screen_size, double delta) override;
  void app_render_frame(dvec2 screen_size, double delta) override;
  void app_render_ui(dvec2 screen_size, double delta) override;

  ViewController view_control;
  TextPainter text_painter;
  GridPainter grid_painter;
};
