add_executable(StupidChessCoreSmokeTests
  CoreSmokeTests.cpp
)

target_compile_features(StupidChessCoreSmokeTests PRIVATE cxx_std_20)

target_link_libraries(StupidChessCoreSmokeTests
  PRIVATE
    StupidChess::Core
)

add_test(
  NAME CoreSmokeTests
  COMMAND StupidChessCoreSmokeTests
)
