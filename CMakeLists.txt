cmake_minimum_required(VERSION 3.16)

project(CyberDuck2 CXX)

add_subdirectory(third_party)
add_subdirectory(src)

# for clangd in vs code
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CACHEFILE_DIR}/compile_commands.json" "${CMAKE_CURRENT_SOURCE_DIR}/build/compile_commands.json")