# CMake Modules

To use the modules in this directory, add this directory to your CMake module search path by inserting the following code in `CMakeLists.txt` in the main directory:
```cmake
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
```

## BuildZeroMQ

This module downloads, builds, and installs ZeroMQ if the CMake variable `INSTALL_ZMQ` is true, e.g., by passing `-DINSTALL_ZMQ=ON` on the CMake command line.



## FindZeroMQ

The module `FindZeroMQ.cmake` searches for ZeroMQ relying _only_ on pkg-config. 

If ZeroMQ is an optional dependency, add a call `find_package(ZeroMQ)` in `CMakeLists.txt`. If `ZeroMQ` was found, then the variable `ZeroMQ_FOUND` will be true and false otherwise. If ZeroMQ is an obligatory dependency, add `find_package(ZeroMQ REQUIRED)`.

The module can also check the ZeroMQ version. For example, to require ZeroMQ version 1.2.3 _or newer_, add `find_package(ZeroMQ 1.2.3)`. If an exact match is required the keyword `EXACT` must be added: `find_package(ZeroMQ 1.2.3 EXACT)`. This option can be freely mixed with the `REQUIRED` keyword added **after** the version specification: `find_package(ZeroMQ 1.2.3 REQUIRED)`.

If ZeroMQ was found, the following variable are set in addition to `ZeroMQ_FOUND`:
* `ZeroMQ_INCLUDE_DIR`,
* `ZeroMQ_LIBRARY`, and
* `ZeroMQ_VERSION`.

The module uses the standard Linux tool pkg-config to locate the ZeroMQ installation. Non-standard installation paths can be passed to this module either by updating the environment variable `PKG_CONFIG_PATH` or by settings the CMake variable `ZeroMQ_ROOT` (use `-DZeroMQ_ROOT=search-path` on the CMake command line).

The features mentioned above (e.g., version checking, `REQUIRED` flag) are standard features of CMake modules.
