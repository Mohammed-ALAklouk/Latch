# Gate Simulator

A desktop logic-gate sandbox for building and simulating digital circuits on an
infinite grid. Place gates, wire them together with orthogonal (right-angle)
routing, drive them with toggle switches, and watch signals propagate in real
time. Built in C++17 with [raylib](https://www.raylib.com/) for rendering and
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

The project targets **Windows** with **Visual Studio 2022** and uses
[premake5](https://premake.github.io/) to generate the solution. `premake5.exe`
is bundled in the repo.

1. Generate the Visual Studio solution:

   ```bash
   make.bat
   ```

   (This runs `premake5 vs2022`.)

2. Open `GateSimulator.sln` in Visual Studio 2022 and build (Debug or Release,
   x64).

3. The executable is written to `bin/<Config>/`. `raylib.dll` from
   `vendor/Raylib/lib` must sit next to it at runtime.


## Dependencies

Both dependencies are vendored under `vendor/`, so no separate install is needed:

- **raylib** — windowing, input, and 2D rendering (`vendor/Raylib`).
- **Dear ImGui** + **rlImGui** — immediate-mode UI panels (`vendor/ImGui`).

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
vendor/             raylib and Dear ImGui
```

## How it works

The `App` runs a mouse-state machine (`Idle`, `Panning`, `Dragging`,
`Connecting`, `Selecting`) each frame. `Circuit` owns the components and wires
and evaluates the network on each tick using per-gate three-valued lookup
tables. Editing operations capture a `CircuitSnapshot` into the `ActionManager`,
which powers undo/redo and the Snapshots panel.
