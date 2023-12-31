#!/usr/bin/python3
# Experimental use of metrolib/tinybuild.py, beware

import sys
import glob

sys.path.append("symlinks/metrolib")
import tinybuild
import pprint

################################################################################

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
objs += compile_dir("symlinks/imgui")
objs += compile_dir("pinwheel/soc")
objs += compile_dir("pinwheel/simulator")
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
