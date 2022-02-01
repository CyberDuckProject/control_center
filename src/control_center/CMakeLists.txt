add_executable(control_center
    control_center.cpp
    font_data.cpp 
    motor_data.h
    transmitter.h 
    gui_context.h
    controller.h
    address.h
    timer_loop.h
    receiving_loop.h
    texture_update_data.h
    tripplebuffer.h
)
target_link_libraries(control_center PRIVATE Imgui Implot Asio JPEG)
target_compile_features(control_center PRIVATE cxx_std_20)
