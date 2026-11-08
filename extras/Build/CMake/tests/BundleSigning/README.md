# macOS bundle-signing regression

This standalone toy plug-in uses the CMake audio plug-in example sources. It exercises
`VST3_AUTO_MANIFEST FALSE`, `juce_enable_vst3_manifest_step(T)`, custom post-build
resources, then `juce_enable_copy_plugin_step(T)`. The runner also covers automatic
manifest generation and copying disabled. Both built and copied VST3/AU bundles must
pass `codesign --verify --deep --strict`. Copies stay inside the test build tree.

From the JUCE root on an arm64 Mac with Ninja:

```sh
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk \
  cmake -Dbuild_dir="$PWD/build-signing" \
  -P extras/Build/CMake/tests/BundleSigning/run.cmake
```

The four configurations share compiled objects and build at `-j 4`. A failure stops
the script immediately. There are 12 signature checks: two local checks per mode,
plus two installed-copy checks in each copy-enabled mode.

Before the fix (fork `1dbb25e9`), the initial manual/copy build failed
`bundle.VST3` and, when selected separately, `installed.AU`, both with
"a sealed resource is missing or invalid". Wrapper setup had registered the seal
before the manual manifest and custom resources; copying skipped verification.
After the fix all 12 checks pass. The changing custom resource also forces each
mode to relink, so an earlier build's valid signature cannot mask bad ordering.
