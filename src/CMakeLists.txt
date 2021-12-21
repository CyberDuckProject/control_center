add_executable(control_center
    entry_point_new.cpp 
    font_data.cpp 
    motor_data.h
    transmitter.h 
    gui_context.h
    controller.h
    address.h
    transmission_loop.h
    receiving_loop.h
)
target_link_libraries(control_center PRIVATE Imgui Asio)
target_compile_features(control_center PRIVATE cxx_std_20)
