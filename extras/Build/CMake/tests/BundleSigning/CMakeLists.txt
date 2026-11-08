cmake_minimum_required(VERSION 3.22)
project(BundleSigningRegression VERSION 1.0.0 LANGUAGES C CXX)
enable_testing()

get_filename_component(juce_root "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
add_subdirectory("${juce_root}" juce)
option(AUTO_MANIFEST "Generate the manifest in juce_add_plugin" OFF)
option(COPY_PLUGIN "Copy after deferred bundle assembly" ON)

juce_add_plugin(SealRegression
    PRODUCT_NAME SealRegression
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Seal
    FORMATS VST3 AU
    VST3_AUTO_MANIFEST ${AUTO_MANIFEST}
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_COPY_DIR "${CMAKE_BINARY_DIR}/installed/VST3"
    AU_COPY_DIR "${CMAKE_BINARY_DIR}/installed/AU")
target_sources(SealRegression PRIVATE
    "${juce_root}/examples/CMake/AudioPlugin/PluginProcessor.cpp"
    "${juce_root}/examples/CMake/AudioPlugin/PluginEditor.cpp")
target_link_libraries(SealRegression PRIVATE juce::juce_audio_utils)
target_compile_definitions(SealRegression PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_compile_features(SealRegression PRIVATE cxx_std_17)

if(NOT AUTO_MANIFEST)
    juce_enable_vst3_manifest_step(SealRegression)
endif()

# Documented deferred assembly: callers may add resources before enabling copy.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/custom.txt" "${AUTO_MANIFEST};${COPY_PLUGIN}\n")
foreach(format VST3 AU)
    set_property(TARGET SealRegression_${format} APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/custom.txt")
    add_custom_command(TARGET SealRegression_${format} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:SealRegression_${format}>/Contents/Resources"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/custom.txt"
            "$<TARGET_BUNDLE_DIR:SealRegression_${format}>/Contents/Resources/custom.txt"
        VERBATIM)
    add_test(NAME bundle.${format} COMMAND "${CMAKE_COMMAND}"
        "-Dbundle=$<TARGET_BUNDLE_DIR:SealRegression_${format}>" "-Dformat=${format}"
        -P "${CMAKE_CURRENT_LIST_DIR}/verify.cmake")
endforeach()

if(COPY_PLUGIN)
    juce_enable_copy_plugin_step(SealRegression)
    add_test(NAME installed.VST3 COMMAND "${CMAKE_COMMAND}"
        "-Dbundle=${CMAKE_BINARY_DIR}/installed/VST3/SealRegression.vst3" -Dformat=VST3
        -P "${CMAKE_CURRENT_LIST_DIR}/verify.cmake")
    add_test(NAME installed.AU COMMAND "${CMAKE_COMMAND}"
        "-Dbundle=${CMAKE_BINARY_DIR}/installed/AU/SealRegression.component" -Dformat=AU
        -P "${CMAKE_CURRENT_LIST_DIR}/verify.cmake")
endif()
