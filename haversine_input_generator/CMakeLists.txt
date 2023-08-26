add_executable(haversine_input_generator
    main.cc
    ../utils/cli_utils.h ../utils/cli_utils.cc
    ../utils/math_utils.h ../utils/math_utils.cc)

target_include_directories(haversine_input_generator PRIVATE ../utils)
