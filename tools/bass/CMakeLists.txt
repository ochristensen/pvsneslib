cmake_minimum_required(VERSION 3.10.1)
add_executable(bass
	bass/bass.cpp
)

target_include_directories(bass
	PRIVATE
	.
)

target_link_libraries(bass
	ws2_32
	ole32
)
