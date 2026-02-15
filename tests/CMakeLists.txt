if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
  message(FATAL_ERROR "Tests require vcpkg toolchain. Configure with -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake")
endif()

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not vcpkg. Use vcpkg toolchain for deterministic gtest binaries.")
endif()

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(StupidChessCoreTests
  CoreSmokeTests.cpp
)

target_compile_features(StupidChessCoreTests PRIVATE cxx_std_20)

target_link_libraries(StupidChessCoreTests
  PRIVATE
    StupidChess::Core
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(StupidChessCoreTests
  DISCOVERY_MODE PRE_TEST
)
