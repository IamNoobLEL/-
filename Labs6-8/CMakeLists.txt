cmake_minimum_required(VERSION 3.24)

add_executable(control_node control_node.cpp)
add_executable(calculation_node calculation_node.cpp)

target_link_libraries(control_node PRIVATE libzmq.so)
target_link_libraries(calculation_node PRIVATE libzmq.so)
