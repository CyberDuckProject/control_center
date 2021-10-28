add_executable(control_center entry_point.cpp font_data.cpp "controls.h")
target_link_libraries(control_center PRIVATE Imgui Asio)
target_compile_features(control_center PRIVATE cxx_std_20)
