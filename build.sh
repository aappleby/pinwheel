set -o errexit
make  -C rv_tests
ninja -C submodules
ninja -C submodules/MetroLib
ninja -C submodules/Metron bin/metron
ninja
#ninja check_sv
