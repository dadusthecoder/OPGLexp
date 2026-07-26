# Title: Zero-Cost Debugging Stack

## Status
Accepted

## Context
The engine needed a way to log and display statistics (like draw calls and meshlet counts) dynamically in the Editor UI without introducing overhead to the final shipped game. Traditional `std::cout` statements are insufficient for Editor tools and too costly for release builds.

## Decision
- We introduced `LGT_DIST` as a compile definition in `CMakeLists.txt` for the Dist (Release) configuration.
- We built a `DebugStats` utility that records key-value strings safely across threads using a mutex.
- We built an `ImGuiConsoleSink` that hooks into `spdlog` to intercept all engine logs for rendering in the Editor.
- When `LGT_DIST` is defined, `DebugStats::Report()` calls evaluate to empty stubs, and logging macros (`CORE_INFO`, `CORE_TRACE`, etc.) compile out entirely, ensuring zero runtime cost.

## Consequences
- **Positive**: Clean logging inside the Editor, robust stat tracking, and zero impact on distribution builds.
- **Negative**: Extra compilation configuration to maintain; slight macro obfuscation around logging methods.
