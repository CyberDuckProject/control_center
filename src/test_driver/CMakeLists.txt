add_executable(test_driver
    test_driver.cpp 
)
target_link_libraries(test_driver PRIVATE Imgui Asio JPEG nfd)
target_compile_features(test_driver PRIVATE cxx_std_20)
