# Parallelization strategy

## Stage 1 — OpenMP

The expensive per-frame casting loops are parallelized across independent work units. This is the stage expected to produce the clearest FPS improvement at higher resolutions because it directly distributes compute-heavy rendering work over multiple cores.

## Stage 2 — display/input thread

The window-system interaction is moved away from the main game/render logic. Even if throughput does not increase dramatically, the architecture becomes clearer by separating computation from X11 event/display work.

## Stage 3 — UDP receiver thread

Position reception becomes blocking rather than continuously polling. A dedicated receiver thread can wait on the socket without stalling the render/game loop or consuming CPU through busy waiting.

## Stage 4 — UDP sender thread

The sender waits on a condition variable and transmits only after movement. This turns sending into event-driven work instead of repeatedly transmitting unchanged state.

## Concurrency concerns

The design must coordinate shutdown, avoid races on shared position/frame data, and ensure all worker threads terminate before program exit. Tools such as Helgrind are appropriate for checking data-race assumptions.
