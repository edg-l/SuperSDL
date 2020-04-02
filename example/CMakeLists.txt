cmake_minimum_required(VERSION 3.13)

project(SuperSDLExample)


set(SOURCE_FILES
	example.cpp
)

include_directories(src)

add_executable(SuperSDLExample ${SOURCE_FILES})

target_link_libraries(SuperSDLExample SuperSDL)

