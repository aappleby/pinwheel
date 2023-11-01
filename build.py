#!/usr/bin/python3
# Experimental use of metrolib/tinybuild.py, beware

import sys
import glob
import argparse

sys.path.append("symlinks/metrolib")
import tinybuild

parser = argparse.ArgumentParser()
parser.add_argument('--verbose',  default=False, action='store_true', help='Print verbose build info')
parser.add_argument('--clean',    default=False, action='store_true', help='Delete intermediate files')
parser.add_argument('--serial',   default=False, action='store_true', help='Do not parallelize actions')
parser.add_argument('--dry_run',  default=False, action='store_true', help='Do not run actions')
options = parser.parse_args()

tinybuild.global_config["verbose"] = options.verbose
tinybuild.global_config["clean"  ] = options.clean
tinybuild.global_config["serial" ] = options.serial
tinybuild.global_config["dry_run"] = options.dry_run

tinybuild.global_config["toolchain"]  = "x86_64-linux-gnu"
tinybuild.global_config["build_type"] = "-g -O0"
tinybuild.global_config["warnings"]   = "-Wunused-variable -Werror"
tinybuild.global_config["depfile"]    = "-MMD -MF {file_out}.d"
tinybuild.global_config["defines"]    = "-DCONFIG_DEBUG"
tinybuild.global_config["cpp_std"]    = "-std=gnu++2a"

compile_cpp = tinybuild.map(
  desc      = "Compiling C++ {file_in} => {file_out}",
  command   = "{toolchain}-g++ {opts} {includes} {defines} -c {file_in} -o {file_out}",
  opts      = "{cpp_std} {warnings} {depfile} {build_type}",
  includes  = "-Isymlinks/metrolib -Isymlinks/metron -I. -Isymlinks",
)

compile_c   = tinybuild.map(
  desc      = "Compiling C {file_in} => {file_out}",
  command   = "{toolchain}-gcc {opts} {includes} {defines} -c {file_in} -o {file_out}",
  opts      = "{warnings} {depfile} {build_type}",
  includes  = "-Isymlinks/metrolib -Isrc -I. -Isymlinks",
)

link_c_lib = tinybuild.reduce(
  desc      = "Bundling {file_out}",
  command   = "ar rcs {file_out} {join(files_in)}",
)

link_c_bin  = tinybuild.reduce(
  desc      = "Linking {file_out}",
  command   = "{toolchain}-g++ {opts} {join(files_in)} {join(deps)} {libraries} -o {file_out}",
  opts      = "{build_type}",
  deps      = [],
  libraries = "",
)

def obj_name(x):
  return "obj/" + tinybuild.swap_ext(x, ".o")

def compile_dir(dir):
  files = glob.glob(dir + "/*.cpp") + glob.glob(dir + "/*.c")
  objs  = [obj_name(x) for x in files]
  compile_cpp(files, objs)
  return objs


objs = []
objs = objs + compile_dir("symlinks/imgui")
objs = objs + compile_dir("pinwheel/soc")
objs = objs + compile_dir("pinwheel/simulator")
objs = objs + compile_dir("pinwheel/tools")
objs = objs + compile_dir("symlinks/glad");

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

build obj/pinwheel/tools/rvdisasm.o : compile_cpp pinwheel/tools/rvdisasm.cpp
build obj/symlinks/glad/glad.o      : compile_cpp symlinks/glad/glad.c

build bin/pinwheel_app: c_binary $
  obj/symlinks/glad/glad.o $
  bin/imgui/libimgui.a $
  obj/pinwheel/simulator/pinwheel_app.o $
  obj/pinwheel/simulator/pinwheel_main.o $
  obj/pinwheel/simulator/pinwheel_sim.o $
  obj/pinwheel/simulator/sim_thread.o $
  obj/pinwheel/tools/rvdisasm.o $
  symlinks/metrolib/bin/metrolib/libappbase.a $
  symlinks/metrolib/bin/metrolib/libcore.a $
  bin/imgui/libimgui.a
  libraries=-lSDL2 -lubsan
"""
