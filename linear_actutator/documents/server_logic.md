# Arduino Controller Server

## Purpose

Non-blocking read/write to serial port.

## Method and Rules

1. The read/write thread must be the same thread.
2. An in-memory model must maintain the data state.
3. The in-memory model will have multi-threaded read and writes.

## Runtime Flow

Thread 1: (in-memory model) main read/write ----- read-serial ---- read-model ---- write-serial ---- read ---- read ---- write ---- read ---- 
Thread 2: (read/write interface) secondary thread read ---- read ---- write-to-model ---- read-model ---- write-to-model
