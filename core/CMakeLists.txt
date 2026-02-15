add_library(StupidChessCore STATIC
  src/MatchReferee.cpp
)

add_library(StupidChess::Core ALIAS StupidChessCore)

target_compile_features(StupidChessCore PUBLIC cxx_std_20)

target_include_directories(StupidChessCore
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
