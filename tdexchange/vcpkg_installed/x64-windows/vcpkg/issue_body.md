Package: boost-regex:x64-windows -> 1.82.0#2

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.37.32822.0
-    vcpkg-tool version: 2023-08-09-9990a4c9026811a312cb2af78bf77f3d9d288416
    vcpkg-scripts version: 56c03b53f 2023-09-07 (3 weeks ago)

**To Reproduce**

`vcpkg install `
**Failure logs**

```
-- Using cached boostorg-regex-boost-1.82.0.tar.gz.
-- Cleaning sources at C:/Users/forti/Tools/vcpkg/buildtrees/boost-regex/src/ost-1.82.0-aceb223f87.clean. Use --editable to skip cleaning for the packages you specify.
-- Extracting source C:/Users/forti/Tools/vcpkg/downloads/boostorg-regex-boost-1.82.0.tar.gz
-- Using source at C:/Users/forti/Tools/vcpkg/buildtrees/boost-regex/src/ost-1.82.0-aceb223f87.clean
-- Including C:/Users/forti/Tools/vcpkg/ports/boost-regex/b2-options.cmake
CMake Error at C:/Users/forti/Projects/2023/Cpp/tdexchange/tdexchange/vcpkg_installed/x64-windows/x64-windows/share/boost-build/boost-modular-build.cmake:39 (message):
  Could not find b2 in
  C:/Users/forti/Projects/2023/Cpp/tdexchange/tdexchange/vcpkg_installed/x64-windows/x64-windows/tools/boost-build
Call Stack (most recent call first):
  ports/boost-regex/portfile.cmake:12 (boost_modular_build)
  scripts/ports.cmake:147 (include)



```
**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "boost-asio"
  ]
}

```
</details>
