set -o errexit
make  -C rv_tests
ninja
#ninja check_sv
