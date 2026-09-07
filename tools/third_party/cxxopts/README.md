# cxxopts

`cxxopts.hpp` is the vendored single-header command-line parser used privately
by `rosbags-info`, `rosbags-read`, and `rosbags-gen`. It was moved unchanged
from `include/rosbags/cxxopts.hpp`; its MIT license is included in the header.

This directory is a private include path for CLI targets only. The header is
not part of the public rosbags API and is not installed with the library.
