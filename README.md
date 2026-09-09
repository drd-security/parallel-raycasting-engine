# Parallel Raycasting Engine

A C++ raycasting application progressively parallelized with **OpenMP, `std::thread`, synchronization primitives, condition variables, and UDP networking** to separate compute-heavy rendering from I/O and multiplayer communication.

## Project progression

The repository preserves the four incremental stages from the assignment:

| Stage | Goal |
|---|---|
| `01-openmp` | Parallelize floor, ceiling, wall, and sprite casting with OpenMP. |
| `02-display-thread` | Move display/input handling to a separate thread. |
| `03-udp-receiver-thread` | Move blocking UDP receive work to its own thread. |
| `04-udp-sender-thread` | Move position sending to a thread and wake it through a condition variable only when the player moves. |

The root `src/` and `include/` directories contain the **final stage** for convenient building.

## Concepts demonstrated

- Data-parallel rendering with OpenMP.
- Thread lifecycle management and clean shutdown.
- Double buffering between rendering and display work.
- Synchronization of shared game/window state.
- Blocking network receive without busy-waiting.
- Condition variables for event-driven network sending.
- UDP exchange of remote-player positions.
- Measuring architectural speedups in a real interactive application rather than a synthetic loop.

## Build

Requires Linux, X11 development libraries, a C++ compiler with OpenMP support, and POSIX sockets.

```bash
make
```

## Run

```bash
./raycasting 800 600 examples/ips1.txt
```

A second local instance can use `examples/ips2.txt` to exchange positions over loopback.

## Controls

- Up/down: move forward/backward.
- Left/right: rotate.
- Escape: terminate.

## Academic context

This submission was stored under a single student ID and is treated here as an **individual project**. The goal was to understand where parallelism helps, where it merely improves architectural separation, and how to make shared code thread-safe.

## Portfolio note

The original starter code was based on an educational raycasting implementation. Before public release, verify which portions are student-authored versus provided by the course and whether redistribution is permitted.

## Publication status

See [NOTICE.md](NOTICE.md).
