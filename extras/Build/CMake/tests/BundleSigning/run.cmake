# Reconfigure one build tree so the expensive JUCE module compilation is shared.
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
        -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_OSX_ARCHITECTURES=arm64
        "-DAUTO_MANIFEST=${auto}" "-DCOPY_PLUGIN=${copy}"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: configure failed (${result})")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build_dir}" -j 4 RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: build failed (${result})")
    endif()
    execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${build_dir}"
        --output-on-failure --stop-on-failure RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${mode}: bundle verification failed (${result})")
    endif()
endforeach()
