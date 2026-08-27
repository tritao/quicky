// Focused Protected Mode audit of the seven runtime targets selected by the
// changed-bit 01F7:5937 dispatch matrix.  The targets are retained by their
// runtime selector:offset addresses; no semantic callback names are inferred.
//
// For each row, the trace captured the target entry and validated the far
// return to 01F7:059B.  Each body was imported independently into Ghidra
// projects A and B with x86:LE:16:Protected Mode and produced identical
// instruction/decompiler exports.
//
// bit  target       entry AX  BX    CX    DS:SI       static result
// 01   14EF:0218   0022      0100  0001  014F:0252  no calls; RETF 0358
// 02   14F7:03C8   0188      0000  0003  014F:0254  no calls; RETF 0485
// 04   14FF:04F4   0044      0001  0002  014F:0256  no calls; RETF 05C2
// 08   1507:0640   0022      0100  0001  014F:0258  no calls; RETF 0702
// 10   150F:0770   0011      0000  0000  014F:025A  no calls; RETF 081A
// 20   1517:0888   0188      0000  0003  014F:025B  no calls; RETF 090E
// 40   151F:097C   0044      0001  0002  014F:025C  no calls; RETF 0A53
//
// Common static contract:
//   FAR target(AX, BX, CX, DX, SI), DS:SI is an unresolved runtime-table
//   destination, DX is also an I/O port, AX and SI are saved/restored, and no
//   caller-consumed return flags are established.  Each body performs four
//   OUT DX,AL operations with AL rotated between writes and fixed byte/word
//   stores at DS:SI-relative offsets.  The exact constants and offsets remain
//   in the per-target Ghidra listings recorded by the matrix evidence.
//
// Dynamic contract:
//   The seven entry/return samples report no player-record, named callback-
//   global, or dispatched-object differences.  This bounds these sampled
//   targets as external contracts with no observed simulation feedback.  It
//   does not prove the same for any DS:6D8A record not in this matrix.
