# Reconfigure one build tree so the expensive JUCE module compilation is shared.
cmake_minimum_required(VERSION 3.22)
if(NOT config)
    set(config RelWithDebInfo)
endif()
if(NOT CMAKE_HOST_SYSTEM_PROCESSOR)
    cmake_host_system_information(RESULT CMAKE_HOST_SYSTEM_PROCESSOR QUERY OS_PLATFORM)
endif()
set(configure_args "-DCMAKE_BUILD_TYPE=${config}"
    "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(generator)
    list(PREPEND configure_args -G "${generator}")
endif()
if(osx_sysroot)
    list(APPEND configure_args "-DCMAKE_OSX_SYSROOT=${osx_sysroot}")
endif()

set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} 4)
foreach(mode manual-copy manual-local auto-copy auto-local)
    set(auto OFF)
    set(copy OFF)
    if(mode MATCHES "^auto")
        set(auto ON)
    endif()
    if(mode MATCHES "copy$")
        set(copy ON)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${build_dir}"
        ${configure_args}
        "-DAUTO_MANIFEST=${auto}" "-DCOPY_PLUGIN=${copy}"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: configure failed (${result})")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build_dir}"
        --config "${config}" -j 4 RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: build failed (${result})")
    endif()
    execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${build_dir}"
        --build-config "${config}"
        --output-on-failure --stop-on-failure RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: bundle verification failed (${result})")
    endif()
endforeach()
