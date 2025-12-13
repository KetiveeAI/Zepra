# CMake generated Testfile for 
# Source directory: /home/swana/Documents/zeprabrowser
# Build directory: /home/swana/Documents/zeprabrowser/build_nxrender
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ZepraBasicTest "/home/swana/Documents/zeprabrowser/build_nxrender/bin/zepra_test")
set_tests_properties(ZepraBasicTest PROPERTIES  _BACKTRACE_TRIPLES "/home/swana/Documents/zeprabrowser/CMakeLists.txt;355;add_test;/home/swana/Documents/zeprabrowser/CMakeLists.txt;0;")
subdirs("source/zepraScript")
subdirs("source/webCore")
subdirs("source/networking")
subdirs("source/zepraEngine")
subdirs("source/integration")
