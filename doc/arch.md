# Architecture

TODO: Architecuture diagram

This documentation describes the Servaru-I architecture.

## Shader Core

Servaru-I uses a unified shader design. The shader is based on the RV32IMAF ISA, with binary compatibility. However, it has some additional instructions and CSRs to control the extended features.

### Instruction Set

It implements the RV32IMF ISA, with the following exceptions:

- TBD

An implict 4-wide SIMT mode may be enabled to accelerate shading operations. The SIMT mode is backed by 4-wide FPU and FPR. In this mode, all floating point operations operate on the 4-wide register, while integer operations reamain scalar.

### CSRs

It has the following CSRs,

- SIMTEN: Enable SIMT lanes
- SIMTAIM: SIMT load/ store address injection mask
- QUEUELOCK: Lock input queue

### Memory Layout

Shared memory bank is how different cores transmit data. Say the rasterizer has generated 4 fragments, and it would like to kick-off a workgroup. It would query the scheduler to get a target FS queue address, then write into that part of the memory. FS would continously try to process the queue.

I memory and D memory does not share the same address space. Different cores/ threads also doesn't share the same memory view.

Each core has its own local instruction memory, allowing up to 1 64bit access per cycle. The instruction memory could also be written by the host CPU. Write always have priority so the core should be put into reset before writing to the instruction memory. Threads on the same core share the same instruction memory, however they do not be in lock-step.

Each core has its own local data memory, allowing up to 1 vec4 access per cycle. These memory could also be written by other cores, at a potentially higher latency. Remote access has priority over local access.

For a core configured for fragment shading, the following is the memory layout:

0x0000 - 0x01FF Input job queue
0x0200 - 0x04FF Local storage
0x0500 - 0x05FF v0 varyings (16 Vec4 FP32)
0x0600 - 0x06FF v1 varyings (16 Vec4 FP32)
0x0700 - 0x07FF v2 varyings (16 Vec4 FP32)

Each input queue entry is composed of:
0x00 - w0 (vec4 int32_t)
0x10 - w1 (vec4 int32_t)
0x20 - w2 (vec4 int32_t)
0x30 - screen position x
0x34 - screen position y
0x38 - pixel valid mask

The input job queue could hold up to 8 entries.

The memory layout is repeated 4 times, for 4 hardware threads, occupying 0x0000 - 0x1FFF (8KB)

### Microarch

The core is built around a 6-stage pipeline:

- IF: Instruction fetch, the address is sent to the SRAM
- ID: Instruction decode, the instruction is decoded, and register file is accessed
- E1: Execute stage 1, integer operations finishes this stage. Memory access starts this stage
- E2: Execute stage 2, memory cache tag compare.
- E3: Execute stage 3, floating point operations and memory access finish at this stage.
- WB: Write-back.

The only forwarding path is from WB to ID, and there is no other type of stalling other than memory access (L1$ access miss or local memory contention). Instruction memory access never miss (always run from local memory). The pipeline uses FGMT to cycle through 4 hardware threads to hide FPU and memory latency.
