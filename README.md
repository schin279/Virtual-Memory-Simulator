# Virtual Memory Simulator

The simulator emulates the behavior of a virtual memory system using various page replacement algorithms and real-world memory traces.

## Overview

The aim of this project is to:
- Simulate virtual memory management with a single-level page table.
- Implement and compare page replacement algorithms: Random (`rand`), Least Recently Used (`lru`), and Enhanced Second Chance (`esc`, also known as Clock).
- Analyze real memory access patterns using SPEC benchmark traces.

## ⚙Features

- Reads and parses real memory trace files.
- Supports 3 page replacement policies: `rand`, `lru`, `esc`.
- Accepts different frame counts to simulate various memory sizes.
- Two modes: `debug` (detailed event output) and `quiet` (performance summary only).
- Tracks disk reads, disk writes, and page fault rate.
