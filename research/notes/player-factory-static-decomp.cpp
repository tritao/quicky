// Focused Ghidra reconstruction of the 01F7:0E06 object-factory callers
// that can be reached from the player callback or its player lifecycle.
//
// Source: the repository's protected-mode Ghidra pipeline, raw NE segment
// import, no-analysis annotation, decompiler/listing export, and NE
// relocation records.  This is deliberately not a census of every 0E06
// caller in the executable: unrelated enemy, boss, menu, and renderer
// families stop at their own closure boundaries.

#include <cstdint>

struct PooledObject {
    std::uint8_t bytes[0x78];
    std::uint8_t &u8(std::uint16_t offset);
    const std::uint8_t &u8(std::uint16_t offset) const;
    std::uint16_t &u16(std::uint16_t offset);
    const std::uint16_t &u16(std::uint16_t offset) const;
    std::int32_t &i32(std::uint16_t offset);
};

struct FactoryResult {
    PooledObject *object;       // ES:DI on return
    bool allocation_succeeded;  // false means the allocator's VGA error loop
};

// 01F7:0E06.  The allocator consumes AX as the callback offset and returns
// ES:DI for the first free 0x78-byte object.  DX is preserved by the body but
// has no semantic use in the allocator itself; callers may use it as part of
// their own register convention.  It initializes only the common header:
// +0x18=AX, +0x1C=0x1997, +0x28=1, +0x17=1, +0x12=FFFF, +0x1A=FFFF,
// +0x14=0, then calls 1036 to register the phase-1 object.
extern FactoryResult object_pool_factory_0E06(
    std::uint16_t callback_ax, std::uint16_t preserved_dx);

struct FactoryCallsite {
    const char *call_address;
    const char *caller;
    std::uint16_t callback_ax;
    std::uint16_t callback_dx;
    const char *post_allocation_writes;
    const char *simulation_feedback;
};

// These are the six 0E06 relocations on the player/lifecycle closure.  The
// other executable-wide 0E06 relocations are intentionally outside this
// focused matrix and are not silently treated as player behavior.
constexpr FactoryCallsite kPlayerReachableFactoryCalls[] = {
    {
        "01F7:0B6F", "player_object_reinitialize_0B56",
        0x3f27, 0x0000,
        "DS:881A=DI; returned +0x17=2",
        "publishes the persistent player callback and phase-2 scheduler entry",
    },
    {
        "01F7:1717", "map_effect_tile_rewrite_16CE",
        0x10b5, 0x11b2,
        "returned +0x2C=3, +0x12=FFFF, +0x02=AX<<16, +0x06=BX<<16, +0x2E=CX",
        "MAP low-nine-bit rewrite precedes allocation; 10B5 itself has no solid probe",
    },
    {
        "01F7:1E89", "instantiate_are_declaration_1E04",
        0xffff, 0x0000,
        "returned +0x17=DS:81D2[type]+2, +0x1A=FS record, +0x06=(record Y+origin)<<16, +0x02=(record X+origin)<<16",
        "callback/phase and position enter the pooled scheduler; type is runtime data",
    },
    {
        "01F7:390E", "callback_contact_effect_dispatch_38EC",
        0x4519, 0x0000,
        "returned +0x29=player +0x28, +0x17=1, +0x06=player Y, +0x02=player X",
        "effect-only 4519 path; no direct player movement or descriptor write",
    },
    {
        "01F7:6432", "probe_contact_tile_offset_6370",
        0x6328, 0x0000,
        "returned +0x38=contact subtype, +0x2A=contact code, +0x02=owner X+offset, +0x06=contact Y<<16 in negative mode or +0x08=contact Y in ordinary mode, +0x32/+0x2E/+0x36=0",
        "contact child can reach 16CE and therefore later MAP state; no direct player write",
    },
    {
        "01F7:654E", "probe_contact_right_648E",
        0x6328, 0x0000,
        "returned +0x38=contact subtype, +0x2A=contact code, +0x02=owner X+5, +0x08=contact Y, +0x06=0, +0x32/+0x2E/+0x36=0",
        "same 6328 child route; the fixed +5 X placement is distinct from 6370",
    },
};

// 01F7:6484 is not a second allocation site.  It stores DS:5003=5 and
// forwards into 6370, so its factory edge is the 6432 relocation above.
struct FactoryWrapper {
    const char *address;
    const char *callee;
    const char *return_contract;
};

constexpr FactoryWrapper kPlayerFactoryWrappers[] = {
    {"01F7:6484", "01F7:6370", "forwards the contact CF result"},
    {"01F7:648E", "01F7:6328", "duplicates the fixed +5 probe and allocates at 654E"},
};

// Caller-selected family matrix, kept address-qualified until runtime
// records and descriptor data identify their authored meaning.
struct FactoryFamily {
    std::uint16_t requested_callback;
    const char *family;
    const char *initializer_or_next_callback;
    const char *classification;
};

constexpr FactoryFamily kFactoryFamilies[] = {
    {0x3f27, "player", "01F7:3FF8", "simulation"},
    {0x6328, "contact child", "01F7:6328 -> 16CE", "simulation-adjacent; MAP feedback open"},
    {0x10b5, "MAP effect child", "01F7:10B5 -> 1693", "presentation/lifetime after MAP write"},
    {0x4519, "callback-tail contact effect", "01F7:4519 -> 45AB -> 470C", "external effect"},
    {0xffff, "ARE-selected callback", "DS:81D2[type]", "runtime data; no semantic name guessed"},
};

// ---------------- Contact-child callback closure ----------------

// The following bodies are the simulation-relevant continuation of the
// player contact factory.  They are kept in this research-only C-like source
// because the native engine does not yet own the original 0x78-byte object
// pool or a mutable MAP.  The code records the exact word operations and the
// point where presentation-only helpers begin; it is not a second gameplay
// implementation.

extern void map_effect_tile_rewrite_16CE(std::uint16_t ax_cell_x_pixels,
                                         std::uint16_t bx_cell_y_pixels,
                                         std::uint16_t cx_effect_selector,
                                         std::uint16_t dx_tile_word);
extern void address_named_ico_renderer_prefix_10CC(PooledObject *object);
extern void address_named_ico_renderer_planes_1186(PooledObject *object);

static std::uint16_t align_low_byte_to_16(std::uint16_t value) {
    // 01F7:633D/633F are AND AL,0xF0 and AND BL,0xF0.  The high byte is
    // preserved; expressing this as a whole-word mask would be wrong for
    // wrapped or negative pixel words.
    return static_cast<std::uint16_t>((value & 0xff00U) |
                                      (value & 0x00f0U));
}

// 01F7:6328.  The callback's return flags are not consumed by the scheduler.
// The 16CE call is the only continuation that can alter later simulation
// state; its MAP/object contract is represented below.
void contact_child_callback_6328(PooledObject *object) {
    const std::uint16_t decremented = static_cast<std::uint16_t>(
        object->u16(0x32) - 1U);
    object->u16(0x32) = decremented;
    if (static_cast<std::int16_t>(decremented) >= 0) {
        return;                         // 632E RET
    }

    object->u16(0x32) = 8;
    const std::uint16_t x = align_low_byte_to_16(object->u16(0x04));
    const std::uint16_t y = align_low_byte_to_16(object->u16(0x08));
    std::uint16_t selector = object->u16(0x2A);
    if (static_cast<std::int8_t>(object->u8(0x38)) < 0) {
        selector = static_cast<std::uint16_t>(
            selector + object->u16(0x2E));
    } else {
        selector = static_cast<std::uint16_t>(
            selector - object->u16(0x2E));
    }
    map_effect_tile_rewrite_16CE(x, y, selector, selector);

    const std::uint16_t phase = static_cast<std::uint16_t>(
        object->u16(0x2E) + 1U);
    object->u16(0x2E) = phase;
    if (phase == 4) {
        object->u16(0x18) = 0;           // 6369: terminal pool release
    }
}

struct MapEffectWorld {
    // 01F7:657A is the far MAP base/selector pair.  The concrete segmented
    // pointer is deliberately abstracted here; the arithmetic below is the
    // exact 16-bit cell selection performed by 16CE.
    std::uint16_t mapRowStride657E;
    std::uint16_t mapCell(std::uint16_t xCell,
                          std::uint16_t yCell) const;
    void writeMapCell(std::uint16_t xCell,
                      std::uint16_t yCell,
                      std::uint16_t value);
};

extern MapEffectWorld g_map_effect_world;

// 01F7:16CE.  No caller consumes return flags.  DX's 0x8000 test uses the
// original unmasked argument, while the stored replacement keeps only its
// low nine bits.  The returned 10B5 object is a phase-1 pooled effect.
void map_effect_tile_rewrite_16CE(std::uint16_t ax_cell_x_pixels,
                                  std::uint16_t bx_cell_y_pixels,
                                  std::uint16_t cx_effect_selector,
                                  std::uint16_t dx_tile_word) {
    const std::uint16_t replacement = static_cast<std::uint16_t>(
        dx_tile_word & 0x01ffU);
    if ((dx_tile_word & 0x8000U) == 0) {
        const std::uint16_t xCell = static_cast<std::uint16_t>(
            ax_cell_x_pixels >> 4);
        const std::uint16_t yCell = static_cast<std::uint16_t>(
            bx_cell_y_pixels >> 4);
        const std::uint16_t oldWord =
            g_map_effect_world.mapCell(xCell, yCell);
        g_map_effect_world.writeMapCell(
            xCell, yCell,
            static_cast<std::uint16_t>((oldWord & 0xfe00U) | replacement));
    }

    PooledObject *effect = object_pool_factory_0E06(0x10b5, 0x11b2).object;
    effect->u16(0x2c) = 3;
    effect->u16(0x12) = 0xffff;
    effect->i32(0x02) = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(ax_cell_x_pixels) << 16);
    effect->i32(0x06) = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(bx_cell_y_pixels) << 16);
    effect->u16(0x2e) = cx_effect_selector;
}

// 01F7:1693.  CF=1 rejects the object; CLC accepts it.  The comparisons are
// signed 16-bit comparisons from JLE/JGE, not unsigned screen-coordinate
// tests.  No player or callback-global write occurs here.
bool effect_camera_visibility_1693(const PooledObject *object,
                                   std::uint16_t camera_x,
                                   std::uint16_t camera_y) {
    const std::int16_t x = static_cast<std::int16_t>(object->u16(0x04));
    const std::int16_t y = static_cast<std::int16_t>(object->u16(0x08));
    const std::int16_t right = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(camera_x + 0x0140U));
    if (x > right) {
        return false;
    }
    const std::int16_t left = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(right - 0x0150U));
    if (x < left) {
        return false;
    }
    const std::int16_t bottom = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(camera_y + 0x00afU));
    if (y > bottom) {
        return false;
    }
    const std::int16_t top = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(bottom - 0x00beU));
    return y >= top;
}

// 01F7:10B5.  The lifetime and visibility prefix is simulation-auditable;
// 11B4 and the values it fills in the 0x81xx presentation scratch area are
// intentionally address-named after this point.  The callback has no solid
// MAP probe and no direct player/global movement write.
void update_ico_effect_10B5(PooledObject *object,
                            std::uint16_t camera_x,
                            std::uint16_t camera_y) {
    const std::uint16_t remaining = static_cast<std::uint16_t>(
        object->u16(0x2c) - 1U);
    object->u16(0x2c) = remaining;
    if (remaining == 0) {
        object->u16(0x18) = 0;           // 10BB: terminal pool release
        return;
    }
    if (!effect_camera_visibility_1693(object, camera_x, camera_y)) {
        return;                          // 10C8 JC 11B0
    }

    address_named_ico_renderer_prefix_10CC(object);
    address_named_ico_renderer_planes_1186(object);
}

// Static stopping rule:
//
// * 0E06 itself is closed at common-header initialization and 1036
//   registration.
// * 4519/45AB/470C stop at the pooled effect contract because they do not
//   write player movement, descriptor cells, or callback-global simulation
//   state.
// * 10B5 stops after 1693 because its remaining writes are renderer state;
//   the preceding 16CE MAP rewrite stays a contract since it can affect a
//   later descriptor query.
// * 6328 stops at 16CE for the same reason.
// * 1E04 remains data-driven at DS:81D2; its caller contract, phase write,
//   source-record claim, and position publication are statically closed.
