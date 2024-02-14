#!/usr/bin/python3
# Experimental use of hancho.py, beware

import sys
import glob
import argparse
import pprint

sys.path.append("symlinks/hancho")
import hancho

pp = pprint.PrettyPrinter(depth=6)

################################################################################

parser = argparse.ArgumentParser(
  prog = "Test Build",
  description = "Test Build for Hancho",
  epilog = "Test Build Done!",
)

parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--clean',    default=False, action='store_true', help='Delete intermediate files')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize commands')
parser.add_argument('--dry_run',  default=False, action='store_true', help='Do not run commands')
parser.add_argument('--debug',    default=False, action='store_true', help='Dump debugging information')
parser.add_argument('--dotty',    default=False, action='store_true', help='Dump dependency graph as dotty')
(flags, unrecognized) = parser.parse_known_args()

hancho.config.verbose    = flags.verbose
hancho.config.clean      = flags.clean
hancho.config.serial     = flags.serial
hancho.config.dry_run    = flags.dry_run
hancho.config.debug      = flags.debug
hancho.config.dotty      = flags.dotty

################################################################################

hancho.config.toolchain  = "x86_64-linux-gnu"
hancho.config.build_type = "-g -O0"
hancho.config.warnings   = "-Wunused-variable -Werror"
hancho.config.depfile    = "-MMD -MF {file_out}.d"
hancho.config.defines    = "-DCONFIG_DEBUG"
hancho.config.cpp_std    = "-std=gnu++2a"

compile_cpp = hancho.rule(
  desc      = "Compiling C++ {file_in} => {file_out}",
  command   = "{toolchain}-g++ {opts} {includes} {defines} -c {file_in} -o {file_out}",
  opts      = "{cpp_std} {warnings} {depfile} {build_type}",
  includes  = "-Isymlinks/metrolib -Isymlinks/metron -I. -Isymlinks",
  parallel  = True,
)

compile_c   = hancho.rule(
  desc      = "Compiling C {file_in} => {file_out}",
  command   = "{toolchain}-gcc {opts} {includes} {defines} -c {file_in} -o {file_out}",
  opts      = "{warnings} {depfile} {build_type}",
  includes  = "-Isymlinks/metrolib -Isrc -I. -Isymlinks",
  parallel  = True,
)

link_c_lib = hancho.rule(
  desc      = "Bundling {file_out}",
  command   = "ar rcs {file_out} {join(files_in)}",
)

link_c_bin  = hancho.rule(
  desc      = "Linking {file_out}",
  command   = "{toolchain}-g++ {opts} {join(files_in)} {join(deps)} {libraries} -o {file_out}",
  opts      = "{build_type}",
  deps      = [],
  libraries = "",
)



#compile_cpp("src/foo.cpp", "obj/foo.o")



def obj_name(x):
  return "obj/" + hancho.swap_ext(x, ".o")

def compile_dir(dir):
  files = glob.glob(dir + "/*.cpp") + glob.glob(dir + "/*.c")
  objs  = [obj_name(x) for x in files]
  compile_cpp(files, objs)
  return objs

objs = []
objs += compile_dir("symlinks/imgui")
objs += compile_dir("pinwheel/soc")
objs += compile_dir("pinwheel/simulator")

hancho.hancho_atexit()

"""
objs += compile_dir("pinwheel/tools")
objs += compile_dir("symlinks/glad")

link_c_bin(
  objs,
  "bin/pinwheel_app",
  deps = [
    "symlinks/metrolib/bin/metrolib/libappbase.a",
    "symlinks/metrolib/bin/metrolib/libcore.a",
  ],
  libraries="-lSDL2 -lubsan"
  )
"""
