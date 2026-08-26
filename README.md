# Latch

A desktop logic-gate sandbox for building and simulating digital circuits on an
infinite grid. Place gates, wire them together with orthogonal (right-angle)
routing, drive them with toggle switches, and watch signals propagate in real
time. Built in C++20 with [raylib](https://www.raylib.com/) for rendering and
[Dear ImGui](https://github.com/ocornut/imgui) for the UI panels.

## Features

- **Nine components** — `AND`, `NAND`, `OR`, `NOR`, `XOR`, `XNOR`, `NOT`, `LED`
  (output indicator), and `TOGGLE` (input switch).
- **Three-valued logic** — every signal is `LOW`, `HIGH`, or `UNDEFINED`, so
  floating inputs are shown explicitly instead of guessed. Wires are color-coded
  by their current value.
- **Orthogonal wiring** — wires route as L-shaped bends; the elbow direction is
  chosen from how you drag and can be re-armed by returning to the start.
- **Flexible wire editing** — start wires from output pins, free input pins, or
  by tapping into an existing wire node or segment; branch a single output to
  many inputs; select and delete individual nodes and segments.
- **Auto-rerouting** — wires attached to a gate re-route automatically when the
  gate (or a selection of nodes) is moved.
- **Simulation control** — step one tick at a time, or run continuously with an
  adjustable ticks-per-second rate.
- **Editing tools** — rubber-band selection, copy/paste, delete, and full
  undo/redo backed by circuit snapshots.
- **Infinite canvas** — pan and zoom over a grid that components snap to.

## Controls

### Mouse

| Action | Control |
| --- | --- |
| Place selected component | Right-click on the canvas |
| Move a gate / selection | Left-drag a gate |
| Draw a wire | Left-drag from an output pin, a free input pin, a wire node, or a wire segment |
| Toggle a switch | Left-click a `TOGGLE` |
| Rubber-band select | Hold **Shift** + left-drag |
| Pan | Left-drag on empty canvas |
| Zoom | Mouse wheel |

Pick which component right-click places from the **Components** panel.

### Keyboard

| Action | Key |
| --- | --- |
| Evaluate circuit once | **Space** |
| Delete selection | **Delete** |
| Copy selection | **Ctrl + C** |
| Paste at cursor | **Ctrl + V** |
| Undo | **Ctrl + Z** |
| Redo | **Ctrl + Y** |

### UI panels

- **Components** — choose the component type for right-click placement.
- **Simulation** — `Step`, `Run`/`Stop`, and a ticks-per-second slider; shows the
  tick count.
- **Selected Component Info** — details of a single selected gate or wire node.
- **Snapshots** — the undo/redo history, with the current state highlighted.

## Building

The project builds with **CMake** (3.21+) and is cross-platform. All
dependencies are fetched automatically at configure time (see
[Dependencies](#dependencies)), so you only need **CMake**, a **C++20 compiler**,
and **Git** installed. Any CMake-supported toolchain works — MSVC / Visual
Studio 2022, GCC, or Clang.

### Using CMake presets (recommended)

The repo ships a `CMakePresets.json` with ready-made configurations. Configure,
then build:

```bash
cmake --preset vs
cmake --build --preset vs
```

Available presets:

- **`vs`** — Visual Studio 2022 generator, x64 (Windows); builds into `build/vs/`.
- **`ninja`** — single-config Ninja Debug build (cross-platform; works with GCC,
  Clang, or MSVC); builds into `build/ninja/`.

You can also open the project folder directly in Visual Studio 2022 — it reads
`CMakePresets.json` and exposes the presets in the configuration dropdown.

### Without presets

```bash
cmake -S . -B build
cmake --build build --config Debug
```

The `Latch` executable is written under the build tree — for the `vs` preset,
`build/vs/src/Debug/Latch.exe`. The required libraries are linked statically, so
there is no DLL to copy alongside it.

### Tests

Tests use [Catch2](https://github.com/catchorg/Catch2) and build by default when
Latch is the top-level project. Build the suite, then run it with CTest — each
`TEST_CASE` is registered as an individual CTest test:

```bash
cmake --build --preset vs
ctest --preset vs --output-on-failure
```

Use the matching `ninja` preset (`ctest --preset ninja --output-on-failure`) for
Ninja builds. To run the test binary directly — handy for
[Catch2 filters and tags](https://github.com/catchorg/Catch2/blob/devel/docs/command-line.md),
e.g. only the gate tests:

```bash
./build/vs/tests/Debug/latch_tests.exe "[gate]"
```

The tests exercise `latch_core` headlessly (no window is opened) and cover the
gate truth tables, circuit evaluation and signal propagation, wire routing
geometry, and the id/index bookkeeping. They assert steady-state behavior rather
than per-tick timing, so they remain valid if the evaluation engine changes.

Pass `-DLATCH_BUILD_TESTS=OFF` at configure time to skip building them entirely
(also skips fetching Catch2).


## Dependencies

All dependencies are fetched automatically by CMake via `FetchContent` at
configure time (which is why Git is required to build) — no manual install or
vendoring needed:

- **raylib** `6.0` — windowing, input, and 2D rendering.
- **Dear ImGui** `v1.92.1` + **rlImGui** — immediate-mode UI panels and the
  raylib backend that draws them.
- **Catch2** `v3.7.1` — unit-test framework (only when tests are enabled).

## Project layout

```
src/
  main.cpp          Entry point
  App.{h,cpp}       Main loop, input handling, mouse-state machine, rendering
  Circuit.{h,cpp}   Component/wire storage, evaluation, snapshots
  Gate.{h,cpp}      Gate base class, per-gate logic and lookup tables
  Wire.{h,cpp}      Orthogonal wire geometry: nodes, segments, routing
  ComponentInfo.h   Serializable component description
  Pin.h             Logic levels and pin references
  Action.h          Undo/redo snapshot manager
  IdManager.h       Stable id ↔ index mapping
  CMakeLists.txt    latch_core library + Latch executable targets
tests/              Catch2 unit tests
CMakeLists.txt      Top-level build: dependency fetching, options
CMakePresets.json   Configure/build/test presets
```

The core simulation logic (`Circuit`, `Gate`, `Wire`, and their headers) is
compiled into a `latch_core` static library, which both the `Latch` executable
and the test suite link against.

## How it works

The `App` runs a mouse-state machine (`Idle`, `Panning`, `Dragging`,
`Connecting`, `Selecting`) each frame. `Circuit` owns the components and wires
and evaluates the network on each tick using per-gate three-valued lookup
tables. Editing operations capture a `CircuitSnapshot` into the `ActionManager`,
which powers undo/redo and the Snapshots panel.

## License

Latch is released under the [MIT License](LICENSE). It bundles third-party
dependencies under their own terms: [raylib](https://www.raylib.com/) (zlib/libpng)
and [Dear ImGui](https://github.com/ocornut/imgui) (MIT).
