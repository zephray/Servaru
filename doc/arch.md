# Architecture

TODO: Architecuture diagram

Servaru-I doesn't use the unified shader scheme. Vertex shader and fragment shader, while reuse the same core architecture, are treated differently in the system. This would change in the next revision.

## Vertex shader

The vertex shader is organized as standalone unit, with 4-wide FP32 FPU. The core has ? stage pipeline.

## Fragment shader

The fragment shader uses the same core design as vertex shader. However, as shown in the architecture diagram, 4 shader cores forms an SIMT fragment shader group (FSG). 4 cores shares instruction memory and uniform memory, but each has dedicated register file and dedicated shared memory bank. Each group also has access to 4 texture mapping units shared among 4 shader cores. Each shader core has it's own dedicated ROP unit.