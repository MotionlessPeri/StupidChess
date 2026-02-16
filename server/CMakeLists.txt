add_library(StupidChessServerSession STATIC
  src/MatchService.cpp
  src/MatchSession.cpp
)

add_library(StupidChess::ServerSession ALIAS StupidChessServerSession)

target_compile_features(StupidChessServerSession PUBLIC cxx_std_20)

target_include_directories(StupidChessServerSession
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(StupidChessServerSession
  PUBLIC
    StupidChess::Core
    StupidChess::Protocol
)

add_executable(StupidChessServer
  src/main.cpp
)

target_compile_features(StupidChessServer PRIVATE cxx_std_20)

target_include_directories(StupidChessServer
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(StupidChessServer
  PRIVATE
    StupidChess::ServerSession
)
