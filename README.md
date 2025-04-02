# Concurrent Channel Communication Library in C

## Overview

This project implements a thread-safe, message-passing channel abstraction in C, inspired by Go's concurrency model. Channels enable safe communication between multiple threads through a fixed-capacity buffer. Each thread can act as a sender or receiver, and all operations are synchronized using POSIX threads and semaphores to ensure race-free execution.

## Features

- Thread-safe communication with a bounded buffer
- Supports both blocking and non-blocking:
  - `send` and `receive` operations
- Allows multiple producers and consumers
- Channel multiplexing via a `select`-style operation for waiting on multiple channels
- Memory-safe and concurrency-safe (validated with Valgrind and ThreadSanitizer)

## Tech Stack

- **Language:** C (C11)
- **Concurrency:** POSIX Threads (`pthread_mutex_t`, `pthread_cond_t`)
- **Synchronization:** POSIX Semaphores
- **Debugging Tools:** Valgrind, ThreadSanitizer, GDB
- **Build Tools:** `make`, `gcc`

## Design Highlights

- **Buffer Management:** FIFO queue internally used to store messages. Thread safety is enforced externally.
- **Blocking vs Non-blocking:** Blocking operations use condition variables to wait efficiently; non-blocking operations return immediately if conditions are not met.
- **Channel Select:** Waits on multiple channels and returns the index of a ready channel for reading.
- **Synchronization:** All critical sections are protected using `pthread` locks and condition variables to avoid race conditions and busy-waiting.

## Testing

The implementation is validated using multi-threaded tests covering:

- High-contention blocking/non-blocking reads and writes
- Graceful closing and destroying of channels in use
- Select-based channel readiness

### Running Tests

```bash
make test                         # Builds and runs all tests
./channel                         # Run all test cases
./channel test_name iters         # Run specific test case
./channel_sanitize test_name      # Detect race conditions
valgrind ./channel test_name      # Detect memory issues
```

## Real-World Application

This project models real-world concurrency mechanisms like Go's channels and is applicable in multi-threaded systems such as:

- Web servers handling asynchronous I/O
- Operating system-level task scheduling
- Real-time event-driven applications

## Outcome

Successfully built and validated a concurrent messaging system in C with support for channel multiplexing, blocking/non-blocking synchronization, and correctness under high concurrency. Demonstrated advanced proficiency in low-level systems programming, concurrency control, and debugging multithreaded applications.
# CSP-Style-Concurrent-Messaging-in-C
