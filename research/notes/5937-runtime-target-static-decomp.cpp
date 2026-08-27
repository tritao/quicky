// Focused Protected Mode reconstruction of one runtime-loaded callback reached
// through 01F7:0598.  This is deliberately address-named: the callback body
// is proven for the captured target 14EF:0218, but the runtime record table can
// select other targets in other states.
//
// Static source:
//   research/build/runtime-target-static-v1/disasm-a/runtime-target-padded.bin.asm
//   research/build/runtime-target-static-v1/decomp-a/runtime-target-padded.bin.c
//   Both independent Ghidra exports are byte-for-byte equal.
// Dynamic source:
//   research/build/traces/player-w1l1-0598-target-code-v3.json
//   The target entered at 14EF:0218 and returned to 01F7:059B.

// Calling convention, confirmed by the listing:
//   FAR callback, entered at 14EF:0218.
//   AX, BX, CX, DX, and SI are live inputs.  DS:SI is the write base and DX
//   is also used as an I/O port.  SI and AX are preserved by PUSH/POP.
//   RETF at 14EF:0358.  The restored AX is the entry AX.  No return flags are
//   consumed by the 01F7:0598 caller; the body does not establish a semantic
//   C-style boolean result.
//
// The byte stores below are exact.  `base` is DS:SI at entry.  The three
// additions to SI use the low byte selected from AH, BL, and BH while the
// original high byte of CX remains in the 16-bit addition.  The port writes
// use the rotated AL value after each block.
void address_named_loaded_callback_14EF_0218(void) {
    uint8_t al = (uint8_t)AX;
    uint8_t rotated = rol8(al, 1);
    uint16_t base = SI;

    out8(DX, al);
    DS8[base + 0x0058] = 0x65;
    DS8[base + 0x005A] = 0x65;
    DS8[base + 0x00B0] = 0x63;
    DS8[base + 0x00B2] = 0x63;
    DS8[base + 0x0108] = 0x63;
    DS8[base + 0x010A] = 0x63;
    DS16[base + 0x0160] = 0x6563;
    DS8[base + 0x0162] = 0x63;
    DS16[base + 0x01B8] = 0x6363;
    DS8[base + 0x01BA] = 0x63;
    DS16[base + 0x0210] = 0x6363;
    DS8[base + 0x0212] = 0x63;
    DS16[base + 0x0268] = 0x6563;
    DS8[base + 0x026A] = 0x63;
    DS8[base + 0x02C0] = 0x63;
    DS8[base + 0x02C2] = 0x63;
    DS8[base + 0x0318] = 0x63;
    DS8[base + 0x031A] = 0x63;
    DS8[base + 0x0370] = 0x63;
    DS8[base + 0x0372] = 0x63;
    DS8[base + 0x03C8] = 0x65;
    DS8[base + 0x03CA] = 0x65;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    uint16_t row = (uint16_t)(base + ((uint16_t)CH << 8 | AH));
    DS8[row + 0x0058] = 0x65;
    DS8[row + 0x00B0] = 0x63;
    DS8[row + 0x0108] = 0x63;
    DS8[row + 0x0160] = 0x63;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS16[row + 0x0268] = 0x6363;
    DS16[row + 0x02C0] = 0x6563;
    DS8[row + 0x0318] = 0x63;
    DS8[row + 0x0370] = 0x63;
    DS8[row + 0x03C8] = 0x63;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    row = (uint16_t)(row + ((uint16_t)CH << 8 | BL));
    DS8[row + 0x0059] = 0x65;
    DS16[row + 0x00B0] = 0x6365;
    DS16[row + 0x0108] = 0x6363;
    DS16[row + 0x0160] = 0x6363;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS16[row + 0x0268] = 0x6363;
    DS16[row + 0x02C0] = 0x6363;
    DS16[row + 0x0318] = 0x6363;
    DS16[row + 0x0370] = 0x6363;
    DS16[row + 0x03C8] = 0x6565;

    out8(DX, rotated);
    row = (uint16_t)(row + ((uint16_t)CH << 8 | BH));
    DS8[row + 0x0059] = 0x65;
    DS8[row + 0x00B1] = 0x63;
    DS16[row + 0x0108] = 0x6365;
    DS16[row + 0x0160] = 0x6363;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS8[row + 0x0269] = 0x63;
    DS8[row + 0x02C1] = 0x63;
    DS8[row + 0x0319] = 0x63;
    DS8[row + 0x0371] = 0x63;
    DS8[row + 0x03C9] = 0x63;
}

// Contract classification:
//   - player reads/writes: none observed; the target executes with DS=014F,
//     not the callback's recovered simulation DS, and the trace has no player
//     or callback-global differences across entry/return.
//   - direct globals: none named.  It writes an unresolved DS:SI-relative
//     runtime table and emits four port writes, so this is a contract rather
//     than a semantic renderer name.
//   - callees/object creation/probes: none.
//   - simulation feedback: not observed for this target and invocation;
//     other DS:6D8A targets remain address-named and unresolved.

// ---------------------------------------------------------------------------
// Five-level runtime-target expansion (changed DS:60D8 bit 0x01)
// ---------------------------------------------------------------------------
//
// The same focused Ghidra import was repeated for one loaded target selected
// at ordinary startup in each of W1L1 through W5L1 after DS:60D8 was patched
// to 0x0001.  The target entry offsets differ, but the captured code prefix
// through RETF is identical byte-for-byte in all five traces:
//
//   W1L1  14EF:0218   RETF 14EF:0358   selector 0x14EF
//   W2L1  128F:0424   RETF 128F:0564   selector 0x128F
//   W3L1  13D7:1E94   RETF 13D7:1FD4   selector 0x13D7
//   W4L1  13C7:13FC   RETF 13C7:153C   selector 0x13C7
//   W5L1  167F:1C10   RETF 167F:1D50   selector 0x167F
//
// The captured 0x400-byte windows include the complete 0x141-byte body and
// trailing bytes.  Each was padded at its recorded entry offset and imported
// into independent x86:LE:16:Protected Mode Ghidra projects A and B.  Both
// decompiler and instruction-list exports matched for every target.  The
// body is represented once below because the code prefix hash is common; the
// address-qualified names remain distinct in the evidence ledger.
//
// Static contract, exact through the first RETF:
//   FAR address_named_loaded_callback_* (AX, BX, CX, DX, SI)
//   - PUSH SI; PUSH AX
//   - OUT DX, AL; ROL AL, 1
//   - fixed byte/word stores relative to DS:SI at offsets 0x0058..0x03CA
//   - OUT DX, AL; ROL AL, 1
//   - SI += (CH:AH); fixed byte/word stores at offsets 0x0058..0x03C8
//   - OUT DX, AL; ROL AL, 1
//   - SI += (CH:BL); fixed byte/word stores at offsets 0x0059..0x03C8
//   - OUT DX, AL
//   - SI += (CH:BH); fixed byte/word stores at offsets 0x0059..0x03C9
//   - POP AX; POP SI; RETF
//
// The instruction listing shows no CALL and no conditional branch.  The
// target preserves AX and SI.  It establishes no caller-consumed return
// value or flags.  DS:SI is deliberately unresolved: its identity could be
// a resource/presentation table, but the static body does not establish that
// semantic name.
void address_named_loaded_callback_5937_bit01_common(
    uint16_t AX, uint16_t BX, uint16_t CX, uint16_t DX, uint16_t SI) {
    uint8_t al = low8(AX);
    uint8_t rotated = rol8(al, 1);
    uint16_t base = SI;

    // The first block is identical in all five target bodies.
    out8(DX, al);
    DS8[base + 0x0058] = 0x65;
    DS8[base + 0x005A] = 0x65;
    DS8[base + 0x00B0] = 0x63;
    DS8[base + 0x00B2] = 0x63;
    DS8[base + 0x0108] = 0x63;
    DS8[base + 0x010A] = 0x63;
    DS16[base + 0x0160] = 0x6563;
    DS8[base + 0x0162] = 0x63;
    DS16[base + 0x01B8] = 0x6363;
    DS8[base + 0x01BA] = 0x63;
    DS16[base + 0x0210] = 0x6363;
    DS8[base + 0x0212] = 0x63;
    DS16[base + 0x0268] = 0x6563;
    DS8[base + 0x026A] = 0x63;
    DS8[base + 0x02C0] = 0x63;
    DS8[base + 0x02C2] = 0x63;
    DS8[base + 0x0318] = 0x63;
    DS8[base + 0x031A] = 0x63;
    DS8[base + 0x0370] = 0x63;
    DS8[base + 0x0372] = 0x63;
    DS8[base + 0x03C8] = 0x65;
    DS8[base + 0x03CA] = 0x65;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    uint16_t row = (uint16_t)(base + ((uint16_t)high8(CX) << 8 | high8(AX)));
    DS8[row + 0x0058] = 0x65;
    DS8[row + 0x00B0] = 0x63;
    DS8[row + 0x0108] = 0x63;
    DS8[row + 0x0160] = 0x63;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS16[row + 0x0268] = 0x6363;
    DS16[row + 0x02C0] = 0x6563;
    DS8[row + 0x0318] = 0x63;
    DS8[row + 0x0370] = 0x63;
    DS8[row + 0x03C8] = 0x63;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    row = (uint16_t)(row + ((uint16_t)high8(CX) << 8 | low8(BX)));
    DS8[row + 0x0059] = 0x65;
    DS16[row + 0x00B0] = 0x6365;
    DS16[row + 0x0108] = 0x6363;
    DS16[row + 0x0160] = 0x6363;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS16[row + 0x0268] = 0x6363;
    DS16[row + 0x02C0] = 0x6363;
    DS16[row + 0x0318] = 0x6363;
    DS16[row + 0x0370] = 0x6363;
    DS16[row + 0x03C8] = 0x6565;

    out8(DX, rotated);
    row = (uint16_t)(row + ((uint16_t)high8(CX) << 8 | high8(BX)));
    DS8[row + 0x0059] = 0x65;
    DS8[row + 0x00B1] = 0x63;
    DS16[row + 0x0108] = 0x6365;
    DS16[row + 0x0160] = 0x6363;
    DS16[row + 0x01B8] = 0x6363;
    DS16[row + 0x0210] = 0x6363;
    DS8[row + 0x0269] = 0x63;
    DS8[row + 0x02C1] = 0x63;
    DS8[row + 0x0319] = 0x63;
    DS8[row + 0x0371] = 0x63;
    DS8[row + 0x03C9] = 0x63;
}

// Dynamic entry/return evidence for all five invocations is in
// `5937-runtime-target-level-static-v1.json`.  Each trace captured the full
// 0x400-byte target window and found no player-record, named callback-global,
// or dispatched-object difference across target entry and return.  This
// narrows the sampled 0x01 dispatch edge to a non-simulation contract.  It
// does not generalize to runtime records selected by other DS:60D8 bits,
// later states, or other levels; those targets remain address-named.

// ---------------------------------------------------------------------------
// Five-level ordinary-startup target set
// ---------------------------------------------------------------------------
//
// A second unpatched startup trace captured the target selected by the normal
// level-start record in each of W1L1 through W5L1.  The target addresses are:
//
//   W1L1  14A7:1258   RETF 14A7:138C
//   W2L1  1247:1470   RETF 1247:15A4
//   W3L1  1397:1040   RETF 1397:1174
//   W4L1  1387:03FC   RETF 1387:0530
//   W5L1  163F:0C10   RETF 163F:0D44
//
// The complete captured body through RETF is 309 bytes in every case and
// has common SHA-256
//   6408cc8d623b432b51ad8993b275d54cb13be12a0f3b9a88f0ef4f9f594da627.
// Each body was imported into independent x86:LE:16:Protected Mode Ghidra
// projects A and B; the decompiler and instruction-list exports matched.
// The five raw 0x400-byte windows differ only outside the common body.
//
// Exact contract through the common RETF.  The `word` stores are listed in
// instruction order.  They are not a semantic interpretation of the table.
// `base` is the entry DS:SI value; each row adjustment is a 16-bit ADD whose
// low byte is selected from AH, BL, or BH and whose high byte is CH.
void address_named_loaded_callback_5937_ordinary_common(
    uint16_t AX, uint16_t BX, uint16_t CX, uint16_t DX, uint16_t SI) {
    uint16_t base = SI;
    uint8_t rotated = rol8(low8(AX), 1);

    out8(DX, low8(AX));
    DS16[base + 0x0000] = 0x796b;
    DS16[base + 0x0058] = 0x7a6c;
    DS16[base + 0x00b0] = 0x796b;
    DS16[base + 0x0108] = 0x636b;
    DS16[base + 0x0160] = 0x656b;
    DS16[base + 0x01b8] = 0x706b;
    DS16[base + 0x0210] = 0x676c;
    DS16[base + 0x0268] = 0x796b;
    DS16[base + 0x02c0] = 0x796c;
    DS16[base + 0x0318] = 0x676c;
    DS16[base + 0x0370] = 0x6b6c;
    DS16[base + 0x03c8] = 0x696c;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    uint16_t row = (uint16_t)(base + ((uint16_t)high8(CX) << 8 | high8(AX)));
    DS16[row + 0x0000] = 0x656a;
    DS16[row + 0x0058] = 0x706b;
    DS16[row + 0x00b0] = 0x7069;
    DS16[row + 0x0108] = 0x6369;
    DS16[row + 0x0160] = 0x7a69;
    DS16[row + 0x01b8] = 0x7069;
    DS16[row + 0x0210] = 0x696c;
    DS16[row + 0x0268] = 0x676b;
    DS16[row + 0x02c0] = 0x796b;
    DS16[row + 0x0318] = 0x6b6b;
    DS16[row + 0x0370] = 0x6c6b;
    DS16[row + 0x03c8] = 0x676b;

    out8(DX, rotated);
    rotated = rol8(rotated, 1);
    row = (uint16_t)(row + ((uint16_t)high8(CX) << 8 | low8(BX)));
    DS16[row + 0x0000] = 0x6365;
    DS16[row + 0x0058] = 0x7a78;
    DS16[row + 0x00b0] = 0x657a;
    DS16[row + 0x0108] = 0x6570;
    DS16[row + 0x0160] = 0x7065;
    DS16[row + 0x01b8] = 0x6579;
    DS16[row + 0x0210] = 0x6770;
    DS16[row + 0x0268] = 0x6969;
    DS16[row + 0x02c0] = 0x6567;
    DS16[row + 0x0318] = 0x7965;
    DS16[row + 0x0370] = 0x6969;
    DS16[row + 0x03c8] = 0x656c;

    out8(DX, rotated);
    row = (uint16_t)(row + ((uint16_t)high8(CX) << 8 | high8(BX)));
    DS16[row + 0x0000] = 0x6565;
    DS16[row + 0x0058] = 0x7978;
    DS16[row + 0x00b0] = 0x637a;
    DS16[row + 0x0108] = 0x7965;
    DS16[row + 0x0160] = 0x6765;
    DS16[row + 0x01b8] = 0x797a;
    DS16[row + 0x0210] = 0x7970;
    DS16[row + 0x0268] = 0x6569;
    DS16[row + 0x02c0] = 0x6579;
    DS16[row + 0x0318] = 0x696b;
    DS16[row + 0x0370] = 0x6965;
    DS16[row + 0x03c8] = 0x696b;
}

// The five ordinary-target traces are recorded in
// `5937-runtime-target-level-ordinary-static-v1.json`.  All five complete
// simulation snapshots and dispatched target-object records compare equal
// across target entry and return.  This closes the normal startup target set
// only.  Runtime records selected in later states remain address-named.
