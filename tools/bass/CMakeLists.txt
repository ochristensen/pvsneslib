cmake_minimum_required(VERSION 3.9.2)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bass/data/architectures/spc700.arch
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/architectures/spc700.arch COPYONLY
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bass/data/architectures/wdc65816.arch
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/architectures/wdc65816.arch COPYONLY
)

add_executable(bass
    bass/bass.cpp
)

target_include_directories(bass
    PRIVATE
    .
)

if(WIN32)
    target_link_libraries(bass
        ws2_32
        ole32
    )
elseif(UNIX AND NOT APPLE)
  target_link_libraries(bass dl)
endif()
