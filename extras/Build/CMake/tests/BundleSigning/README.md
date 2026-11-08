# macOS bundle-signing regression

This standalone toy plug-in uses the CMake audio plug-in example sources. It exercises
`VST3_AUTO_MANIFEST FALSE`, `juce_enable_vst3_manifest_step(T)`, custom post-build
resources, then `juce_enable_copy_plugin_step(T)`. The runner also covers automatic
manifest generation and copying disabled. Both built and copied VST3/AU bundles must
pass `codesign --verify --deep --strict`. Copies stay inside the test build tree.

From the JUCE root on an Intel or Apple Silicon Mac:

```sh
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk \
  cmake -Dbuild_dir="$PWD/build-signing" -Dgenerator=Ninja -Dconfig=RelWithDebInfo \
  -P extras/Build/CMake/tests/BundleSigning/run.cmake
```

The four configurations share compiled objects and build at `-j 4`. A failure stops
the script immediately. There are 12 signature checks: two local checks per mode,
plus two installed-copy checks in each copy-enabled mode.

The nested build uses the host architecture so its manifest helper can run.
The parent passes its generator, active configuration and SDK (`generator`,
`config`, `osx_sysroot`). Standalone runs default to CMake's default generator
and RelWithDebInfo; Xcode and Ninja Multi-Config select the same configuration
for both build and CTest. No Ninja dependency is added to an Xcode parent.

`test_runner.cmake` captures all four configure/build/test command sequences for
simulated Intel/Ninja, Intel/Xcode and Apple Silicon/Ninja Multi-Config parents,
including paths containing spaces. Run it with `cmake -Dtest_dir=<scratch-dir>
-P extras/Build/CMake/tests/BundleSigning/test_runner.cmake`. These command checks
complement the 12 real bundle checks above.

Before the fix (fork `1dbb25e9`), the initial manual/copy build failed
`bundle.VST3` and, when selected separately, `installed.AU`, both with
"a sealed resource is missing or invalid". Wrapper setup had registered the seal
before the manual manifest and custom resources; copying skipped verification.
After the fix all 12 checks pass. The changing custom resource also forces each
mode to relink, so an earlier build's valid signature cannot mask bad ordering.
