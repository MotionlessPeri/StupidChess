add_library(StupidChessProtocol STATIC
  src/ProtocolTypes.cpp
)

add_library(StupidChess::Protocol ALIAS StupidChessProtocol)

target_compile_features(StupidChessProtocol PUBLIC cxx_std_20)

target_include_directories(StupidChessProtocol
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
