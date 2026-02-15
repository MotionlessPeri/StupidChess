add_executable(StupidChessServer
  src/main.cpp
  src/MatchSession.cpp
)

target_compile_features(StupidChessServer PRIVATE cxx_std_20)

target_include_directories(StupidChessServer
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(StupidChessServer
  PRIVATE
    StupidChess::Core
    StupidChess::Protocol
)
