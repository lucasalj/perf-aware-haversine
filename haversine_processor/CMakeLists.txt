add_executable(haversine_processor
    main.cc
    json_parser.h json_parser.cc
    ../utils/math_utils.h ../utils/math_utils.cc
    ../utils/cli_utils.h ../utils/cli_utils.cc)

target_include_directories(haversine_processor PRIVATE ../utils)
