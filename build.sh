set -o errexit
make  -C rv_tests
ninja -C submodules
ninja -C submodules/MetroLib
ninja -C firmware
ninja -C microtests
ninja -C submodules/Metron bin/metron
ninja
#ninja check_sv
