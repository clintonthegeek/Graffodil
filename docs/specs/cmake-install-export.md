# Spec: CMake Install / Export / Versioning

**Status:** spec'd, needs implementation plan
**Depends on:** Phase 5 (spatial module) should land first so the module set is final
**Addresses:** evaluation finding A9
**Priority note:** land before the Corbomite/PlanStan migrations begin.

## Problem

Graffodil is consumed by several programs, but no `install()` rules, no
`GraffodilConfig.cmake`, and no version artifact exist. Consumers can only
vendor or `add_subdirectory` a sibling checkout, with no way to pin or check a
version. Formalizing the package also forces an explicit statement of what is
public API.

## Goals

- `find_package(Graffodil 0.1 REQUIRED COMPONENTS Core Sugiyama Force Circular Batch Spatial)`
  works from an install tree.
- `add_subdirectory` consumption keeps working unchanged (sibling-repo layout
  stays supported).
- A version header so consumers can compile-time check
  (`GRAFFODIL_VERSION_MAJOR/MINOR/PATCH`, `GRAFFODIL_VERSION` string).
- Correct transitive deps: an installed `Graffodil::Force` must pull in
  `Graffodil::Spatial` (PRIVATE link of a static lib still needs the archive
  at final link).

## Non-goals

- Shared-library builds, export macros, ABI/d-pointer work (static-only stays).
- pkg-config files.
- Packaging (deb/rpm/conan/vcpkg).

## Design

1. **Targets.** Each module's CMakeLists adds an `INSTALL_INTERFACE` include
   path alongside the existing `BUILD_INTERFACE`:
   ```cmake
   target_include_directories(graffodil-core PUBLIC
       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
   ```
   and sets `EXPORT_NAME` (`Core`, `Sugiyama`, …) so the exported names are
   `Graffodil::Core` etc., matching the in-tree ALIAS targets.
2. **Install rules** (root CMakeLists or a `cmake/install.cmake` include):
   `include(GNUInstallDirs)`; per module
   `install(TARGETS graffodil-<m> EXPORT GraffodilTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})`
   and `install(DIRECTORY src/<m>/include/graffodil DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})`.
   Note: static libs with `PRIVATE` deps (Force → Spatial) export the dep into
   the link interface automatically via `$<LINK_ONLY:...>` — both targets must
   be in the same export set.
3. **Package config.** `cmake/GraffodilConfig.cmake.in`:
   ```cmake
   @PACKAGE_INIT@
   include(CMakeFindDependencyMacro)
   find_dependency(Qt6 6.8 COMPONENTS Core Widgets)
   include("${CMAKE_CURRENT_LIST_DIR}/GraffodilTargets.cmake")
   # COMPONENTS validation: every requested component must be an exported target
   foreach(_c ${Graffodil_FIND_COMPONENTS})
     if(NOT TARGET Graffodil::${_c})
       set(Graffodil_FOUND FALSE)
       set(Graffodil_NOT_FOUND_MESSAGE "Unknown Graffodil component: ${_c}")
     endif()
   endforeach()
   ```
   plus `write_basic_package_version_file(... COMPATIBILITY SameMinorVersion)`
   — pre-1.0, minor bumps may break API, so SameMinorVersion (not SameMajorVersion).
   Install both to `${CMAKE_INSTALL_LIBDIR}/cmake/Graffodil`.
4. **Version header.** `configure_file` from
   `src/core/include/graffodil/Version.h.in` using `PROJECT_VERSION_*`;
   generated into the build tree, installed with the other headers, and added
   to `graffodil-core`'s build interface include path.
5. **Demo and tests excluded** from install; gate them behind
   `option(GRAFFODIL_BUILD_DEMO ON)` / `option(GRAFFODIL_BUILD_TESTS ON)` so a
   consumer using FetchContent doesn't build them
   (`if(PROJECT_IS_TOP_LEVEL)` defaults are fine).

## Acceptance test

Scripted in CI/ctest or run manually:

```bash
cmake -S . -B build-inst -DCMAKE_INSTALL_PREFIX=$PWD/_inst
cmake --build build-inst && cmake --install build-inst
# Minimal consumer:
#   find_package(Graffodil REQUIRED COMPONENTS Core Force)
#   target_link_libraries(consumer PRIVATE Graffodil::Core Graffodil::Force)
cmake -S consumer-test -B consumer-build -DCMAKE_PREFIX_PATH=$PWD/_inst
cmake --build consumer-build   # must link, including the Spatial transitive
```

The plan for this spec should include that minimal consumer project as a
committed fixture under `tests/package/` with a ctest entry that exercises
configure+build of it.
