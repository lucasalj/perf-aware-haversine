add_executable(haversine_generator_release
    main.cc
    cli_utils.h cli_utils.cc)
target_include_directories(haversine_generator_release PRIVATE haversine_generator_release)
