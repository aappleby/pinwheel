#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <elf.h>
#include <sys/stat.h>
//#include <arch/riscv/include/asm/elf.h>

#include "metrolib/core/Tests.h"
#include "metrolib/appbase/AppHost.h"
#include "pinwheel/soc/pinwheel_soc.h"
#include "pinwheel/simulator/pinwheel_app.h"

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  PinwheelApp app;
  AppHost host(&app);

  auto app_result = host.app_main(argc, argv);

  printf("main() done\n");
  return app_result;
}

//------------------------------------------------------------------------------
