# ISA

This document describe the ISA Servaru-I shader. Servaru-I loosely targets DX 8.1 SM 1.4 / OpenGL 1.3 with ARB shader. This means there is no integer operations, no flow control, and no memory operations. Future versions would likely target higher version of API support.

## Opcode Overview
 
The following are all the supported operations:

- EXC: 0  Fail and report error
- ABS: 1  Absolute value (vec4)
- ADD: 2  Add (vec4)
- CMP: 3  Conditional (ternary) assign (vec4)
- DP3: 4  3-component dot product (vec4 -> scalar)
- DP4: 5  4-component dot product (vec4 -> scalar)
- DPH: 6  Homogeneous dot product (vec4 -> scalar)
- DST: 7  Distance vector (vec4)
- EX2: 8  Exponential base 2 (scalar)
- FLR: 9  Floor (vec4)
- FRC: 10 Fraction (vec4)
- KIL: 11 Kill fragment
- LG2: 12 Logarithm base 2 (scalar)
- LIT: 13 Compute lighting coefficients (vec4)
- LRP: 14 Linear interpolation (vec4)
- MAD: 15 Multiply and add (vec4)
- MAX: 16 Maximum (vec4)
- MIN: 17 Minimum (vec4)
- MOV: 18 Move (vec4)
- MUL: 19 Multiply (vec4)
- POW: 20 Exponentiate (scalar)
- RCP: 21 Reciprocal (scalar)
- RSQ: 22 Reciprocal square root (scalar)
- SGE: 23 Set on greater or equal (vec4)
- SLT: 24 Set on less than (vec4)
- SUB: 25 Subtract (vec4)
- TEX: 26 Texture sample (vec4)
- TXB: 27 Texture sample with bias (vec4)
- TXF: 28 Texture fetch (vec4)
- XPD: 29 Cross product (vec4)
- ZTS: 30 Perform Z-test (TBD!)

## Register File

Register file are divided into 3 parts:

- Un: Uniform register file, read only, limited to 1 read per cycle.
- Rn: Local temp register file, RW, up to 3 read and 1 write per cycle.
- Sn: Shared register file, RW, up to 1 read and write per cycle.

P0 to P8 are used as input registers, and P9 to P15 are used as output reigsters, as part of the calling convention.

Register uses 9-bit encoding X0 to X511, where:

- X0 to X31 are mapped to R0 to R31
- X127 to X159 are mapped to P0 to P31
- X256 to X383 are mapped to U0 to U127

Remaining locations are unused.

## Instruction Encoding

The instruction is always 64 bits, with the base formatting:

{rsvd, opcode, dst, src1, src2, src3}

Each source/ destination field is 14 bits, being either immediate number or register specifier:

- {1'b0, sign, exp5, mantissa7} Immediate FP value with 13-bit precision, boardcast to all components.
- {1'b1, reg_addr, swizzle} Reg with 9 bit register address, and 4 bit swizzle/ masking bit, address on higher bits.

Bit interpretation is based on instruction.

Opcode is 5-bit.

Rsvd is 3-bit. They are not used for anything as of now, but might be used later.
