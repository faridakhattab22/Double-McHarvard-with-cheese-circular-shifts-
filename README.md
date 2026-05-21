# Pipelined Processor Simulator

A computer architecture simulator implementing a 3-stage pipelined CPU with hazard detection, forwarding, branch flushing, ALU operations, and real-time GUI visualization using C and Python.

---

## Overview

This project simulates a custom 16-bit pipelined processor architecture.  
The processor executes instructions through a 3-stage pipeline:

- Instruction Fetch (IF)
- Instruction Decode (ID)
- Execute (EX)

The simulator supports:
- Arithmetic and logical operations
- Memory operations
- Branching and jumps
- Hazard detection and stalling
- Data forwarding
- Pipeline flushing
- Real-time processor visualization through a Python GUI

---

## Features

- 3-stage pipelined CPU architecture
- 16-bit instruction format
- 64 general-purpose registers
- Instruction and data memory simulation
- Hazard detection and pipeline stalling
- Data forwarding support
- Branch and jump flushing
- ALU operations with status register updates
- Cycle-by-cycle execution visualization
- Real-time GUI using Python Tkinter
- Register and memory monitoring
- Assembly parser and instruction encoder

---

## Technologies Used

### Languages
- C
- Python

### Libraries
- Tkinter (GUI)
- Regex
- Subprocess & Threading

---

## Processor Architecture

### Pipeline Stages

### 1. Fetch (IF)
- Reads instructions from instruction memory
- Updates the Program Counter (PC)
- Passes instructions to the IF/ID register

### 2. Decode (ID)
- Decodes instruction fields
- Reads register values
- Determines instruction format
- Passes decoded data to the ID/EX register

### 3. Execute (EX)
- Performs ALU operations
- Handles branching and jumps
- Executes memory operations
- Detects hazards
- Updates the status register (SREG)

---

## Supported Instructions

### Arithmetic & Logic
- ADD
- SUB
- MUL
- AND
- OR
- SAL
- SAR

### Memory
- LB
- SB

### Control Flow
- BEQZ
- JR

### Immediate Operations
- LDI

---

## Hazard Handling

The processor implements:
- Data hazard detection
- Pipeline stalling
- Data forwarding
- Branch flushing

This ensures correct execution during dependent instructions and control-flow changes.

---

## GUI Visualization

The simulator includes a Python GUI that provides:

- Real-time pipeline stage visualization
- Register file monitoring
- Instruction memory display
- Data memory visualization
- Processor state tracking
- Cycle-by-cycle execution updates

The GUI communicates with the C simulator using subprocess communication and regex parsing.

---

## Project Structure

```bash
├── main.c
├── parser.c
├── pipeline_if_id.c
├── alu_hazards.c
├── flush.c
├── registers.c
├── sreg.c
├── processor_gui.py
├── structures.h
├── processor.h
└── README.md
