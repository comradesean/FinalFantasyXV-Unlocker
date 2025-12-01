# FFXV Windows Edition - Complete Memory & Code Flow Diagram

## Introduction

This document provides a comprehensive visual and technical breakdown of how Final Fantasy XV Windows Edition handles exclusive item unlocks. It's written for someone who may not be familiar with reverse engineering, assembly language, or memory manipulation.

---

## Table of Contents

1. [Basics: How Games Store Data](#basics-how-games-store-data)
2. [The Two Key Data Structures](#the-two-key-data-structures)
3. [The Unlock Check Loop - Bird's Eye View](#the-unlock-check-loop---birds-eye-view)
4. [Detailed Code Flow with Assembly Explained](#detailed-code-flow-with-assembly-explained)
5. [Why Some Items Don't Unlock](#why-some-items-dont-unlock)
6. [Visual Memory Map](#visual-memory-map)
7. [Register Reference](#register-reference)

---

## Basics: How Games Store Data

### Memory Addresses

Every piece of data in a running program lives at a specific "address" in memory. Think of it like a street address for data. In FFXV, addresses look like `0x140752038` - this is a hexadecimal (base-16) number.

### Hexadecimal Primer

- Hex uses 0-9 and A-F (where A=10, B=11, C=12, D=13, E=14, F=15)
- `0x10` in hex = 16 in decimal
- `0x1F` in hex = 31 in decimal
- `0xFF` in hex = 255 in decimal

### Assembly Language

The CPU executes very simple instructions called "assembly" or "machine code." Common instructions:
- `mov eax, 5` - Put the value 5 into register EAX
- `cmp eax, 10` - Compare EAX with 10 (sets flags for later jumps)
- `je 751FAD` - "Jump if Equal" - go to address 751FAD if the last comparison was equal
- `jmp 751F6D` - "Jump" unconditionally to address 751F6D
- `call 7A5790` - Call a function at that address
- `nop` - "No Operation" - do nothing (used to disable code)

### Registers

Registers are tiny, super-fast storage slots in the CPU. FFXV uses a 64-bit CPU, so registers can hold large values:
- **RAX, RBX, RCX, RDX** - General purpose (64-bit)
- **EAX, EBX, ECX, EDX** - Lower 32 bits of the above
- **AL, BL, CL, DL** - Lowest 8 bits (1 byte) of the above
- **RBP** - Often used as a loop counter or base pointer
- **R8-R15** - Additional registers (64-bit), R12B is lowest byte of R12

---

## The Two Key Data Structures

FFXV uses two tables in memory to control item unlocks:

### 1. Byte Table (The "ON/OFF Switch" Table)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           BYTE TABLE                                        │
│                     Base Address: 0x140752038                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  This table has ONE BYTE per item. Each byte is either:                     │
│    0x00 = Item is DISABLED (skipped during unlock check)                    │
│    0x01 = Item is ENABLED (processed during unlock check)                   │
│                                                                             │
│  To find an item's byte: Base + Item_ID                                     │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│ Address      │ Item ID │ Value │ Item Name                                  │
├──────────────┼─────────┼───────┼────────────────────────────────────────────┤
│ 0x140752038  │  0x00   │  00   │ Blazefire Saber                            │
│ 0x140752039  │  0x01   │  00   │ (unknown)                                  │
│ ...          │  ...    │  00   │ ...                                        │
│ 0x14075204B  │  0x13   │  00   │ Noodle Helmet                              │
│ ...          │  ...    │  00   │ ...                                        │
│ 0x140752058  │  0x20   │  00   │ Weatherworn Regalia Decal (Twitch Prime)   │
│ 0x140752059  │  0x21   │  00   │ Kooky Chocobo Tee (Twitch Prime)           │
│ 0x14075205A  │  0x22   │  00   │ Kooky Chocobo (Twitch Prime)               │
│ 0x14075205B  │  0x23   │  00   │ 10,000 GIL (Twitch Prime)                  │
│ 0x14075205C  │  0x24   │  00   │ 100 AP (Twitch Prime)                      │
│ 0x14075205D  │  0x25   │  00   │ 16 Rare Coins (Twitch Prime)               │
│ 0x14075205E  │  0x26   │  00   │ FFXV Powerup Pack (MS Store)               │
│ 0x14075205F  │  0x27   │  00   │ FFXV Fashion Collection (Steam) ★          │
│ 0x140752060  │  0x28   │  00   │ FFXV Decal Selection (Origin)              │
│ 0x140752061  │  0x29   │  00   │ HEV Suit - COMRADES (Steam) ★              │
│ 0x140752062  │  0x2A   │  00   │ Sims 4 Pack (Origin)                       │
│ 0x140752064  │  0x2C   │  00   │ Scientist Glasses (Steam) ★                │
│ 0x140752065  │  0x2D   │  00   │ Crowbar - COMRADES (Steam) ★               │
│ 0x140752066  │  0x2E   │  00   │ Half-Life Costume (Steam) ★                │
│ 0x140752067  │  0x2F   │  00   │ Crowbar (Steam) ★                          │
│ 0x140752068  │  0x30   │  00   │ 8700K Accessories (Intel Promo) ★          │
│ 0x14075206B  │  0x33   │  00   │ Alien Shield (Promo) ★                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ ★ = Problematic items that don't unlock even when set to 0x01               │
└─────────────────────────────────────────────────────────────────────────────┘

EXAMPLE: To enable Blazefire Saber, write 0x01 to address 0x140752038
         To enable Fashion Collection, write 0x01 to address 0x14075205F
```

### 2. Jump Table (The "Router" Table)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           JUMP TABLE                                        │
│                     Base Address: 0x14075206C                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  This table has a 4-BYTE OFFSET per item index.                             │
│  The offset tells the code WHERE TO JUMP for each item's ownership check.   │
│                                                                             │
│  Item Index = Item_ID - 0x1F                                                │
│  Table Entry Address = Base + (Item_Index × 4)                              │
│  Final Jump Destination = 0x140000000 + Offset_Value                        │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│ Table Addr   │ Index │ Offset (dec) │ Jump To      │ Item                   │
├──────────────┼───────┼──────────────┼──────────────┼────────────────────────┤
│ 0x14075206C  │  0    │ 7675109      │ 0x140751F45  │ (index 0 items)        │
│ 0x140752070  │  1    │ ?            │ ?            │                        │
│ ...          │  ...  │ ...          │ ...          │                        │
│ 0x14075208C  │  8    │ 7675132      │ 0x140751F5C  │ Fashion Collection ★   │
│ 0x140752094  │  10   │ 7675231      │ 0x140751FBF  │ HEV Suit ★             │
│ 0x1407520A8  │  15   │ 7675519      │ 0x1407520FF  │ Half-Life Costume ★    │
│ 0x1407520B0  │  17   │ 7675165      │ 0x140751D1D  │ 8700K ★                │
│ 0x1407520BC  │  20   │ 7675198      │ 0x140751FBE  │ Alien Shield ★         │
├─────────────────────────────────────────────────────────────────────────────┤
│ ★ = These items jump to DIFFERENT code than working items!                  │
│     Working items → 0x751F45 (does ownership check, can return "owned")     │
│     Fashion Coll → 0x751F5C (SKIPS ownership check entirely!)               │
│     8700K        → 0x751D1D (does check, but always returns "not owned")    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## The Unlock Check Loop - Bird's Eye View

When you open the game menu, FFXV runs a loop that checks every possible exclusive item:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    FFXV EXCLUSIVE ITEM UNLOCK LOOP                          │
│                                                                             │
│         Triggered when: Loading main menu or in-game menu                   │
│         Location: ffxv_s.exe + 0x751C8B through 0x751FC1                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FOR EACH ITEM (0x00 to 0x33):                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │     STEP 1: BYTE TABLE        │
                    │     "Is this item enabled?"   │
                    │                               │
                    │  Read byte at table + item_id │
                    └───────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                    ▼                               ▼
            ┌───────────────┐               ┌───────────────┐
            │  Byte = 0x00  │               │  Byte = 0x01  │
            │    (OFF)      │               │    (ON)       │
            └───────────────┘               └───────────────┘
                    │                               │
                    ▼                               ▼
            ┌───────────────┐               ┌───────────────────────────┐
            │ SKIP ITEM     │               │ STEP 2: JUMP TABLE        │
            │ (next item)   │               │ "Where's this item's      │
            └───────────────┘               │  ownership check code?"   │
                                            └───────────────────────────┘
                                                        │
                                                        ▼
                                            ┌───────────────────────────┐
                                            │ JUMP TO ITEM'S HANDLER    │
                                            │ (different for each item) │
                                            └───────────────────────────┘
                                                        │
                    ┌───────────────────────────────────┼───────────────────┐
                    │                                   │                   │
                    ▼                                   ▼                   ▼
        ┌───────────────────────┐       ┌───────────────────┐   ┌───────────────────┐
        │ WORKING ITEMS         │       │ FASHION COLLECTION │   │ 8700K             │
        │ (e.g., Blazefire)     │       │                    │   │                   │
        │                       │       │                    │   │                   │
        │ → Goes to 0x751F45    │       │ → Goes to 0x751F5C │   │ → Goes to 0x751D1D│
        │ → Calls ownership     │       │ → SKIPS ownership  │   │ → Calls ownership │
        │   check function      │       │   check entirely!  │   │   check function  │
        │ → Returns AL=1 or 0   │       │ → DL stays 0       │   │ → Returns AL=0    │
        └───────────────────────┘       └───────────────────┘   └───────────────────┘
                    │                               │                       │
                    └───────────────────────────────┼───────────────────────┘
                                                    │
                                                    ▼
                                    ┌───────────────────────────────┐
                                    │ STEP 3: OWNERSHIP CHECK       │
                                    │ "Does the player own this?"   │
                                    │                               │
                                    │ Check: Is DL register = 1?    │
                                    │ Location: 0x751F5A            │
                                    └───────────────────────────────┘
                                                    │
                                    ┌───────────────┴───────────────┐
                                    │                               │
                                    ▼                               ▼
                            ┌───────────────┐               ┌───────────────┐
                            │   DL = 0      │               │   DL = 1      │
                            │  (not owned)  │               │   (owned)     │
                            └───────────────┘               └───────────────┘
                                    │                               │
                                    ▼                               ▼
                            ┌───────────────┐               ┌───────────────┐
                            │ SKIP UNLOCK   │               │ GRANT ITEM!   │
                            │ (next item)   │               │ Call 0x7A5790 │
                            └───────────────┘               └───────────────┘
                                    │                               │
                                    └───────────────┬───────────────┘
                                                    │
                                                    ▼
                                    ┌───────────────────────────────┐
                                    │      NEXT ITEM IN LOOP        │
                                    │   (repeat until item 0x33)    │
                                    └───────────────────────────────┘
```

---

## Detailed Code Flow with Assembly Explained

### PHASE 1: Loop Initialization

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751C8B                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   mov ebp, FFFFFFE0        ; EBP = -32 (starting index)                     │
│       │                                                                     │
│       └─► EBP is the loop counter. It starts at -32 (0xFFFFFFE0)            │
│           and counts up. The item ID is calculated as EBP + 0x1F.           │
│           So when EBP = -32, item_id = -32 + 31 = -1 (before first item)    │
│           When EBP = -31, item_id = 0 (Blazefire Saber)                     │
│           When EBP = 8, item_id = 8 + 31 = 39 = 0x27 (Fashion Collection)   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 2: Calculate Item ID

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CA0                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   mov cl, 01               ; CL = 1 (used later)                            │
│                                                                             │
│   lea eax, [rbp+1F]        ; EAX = EBP + 0x1F = item_id                     │
│       │                                                                     │
│       └─► "LEA" means "Load Effective Address" - it does math              │
│           This calculates: EAX = RBP + 31                                   │
│           Now EAX contains the current item ID (0x00 to 0x33)               │
│                                                                             │
│   EXAMPLE: When EBP = 8                                                     │
│            EAX = 8 + 31 = 39 = 0x27 (Fashion Collection)                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 3: Bounds Check

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CA5                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   cmp eax, 33              ; Compare EAX with 0x33 (max item ID)            │
│       │                                                                     │
│       └─► This checks if the item ID is within valid range (0-51)          │
│           Sets CPU flags based on comparison result                         │
│                                                                             │
│   ja 751CC4                ; "Jump if Above" - if EAX > 0x33, jump          │
│       │                                                                     │
│       └─► JA = Jump if Above (unsigned comparison)                         │
│           If item_id > 51, skip to 751CC4                                   │
│           This is a safety check for invalid item IDs                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 4: Byte Table Lookup (The ON/OFF Check)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CAC                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   movzx eax, byte ptr [r8+rax+00752038]                                     │
│       │         │          │                                                │
│       │         │          └─► 0x752038 is the byte table offset            │
│       │         └─► [r8+rax+00752038] means: read memory at                 │
│       │              address = R8 + RAX + 0x752038                          │
│       │              R8 = 0x140000000 (base), RAX = item_id                 │
│       │              So this reads from 0x140752038 + item_id               │
│       │                                                                     │
│       └─► MOVZX = "Move with Zero Extend"                                   │
│           Reads 1 byte and puts it in EAX (filling upper bits with 0)       │
│                                                                             │
│   RESULT: EAX now contains 0x00 or 0x01 (the byte table value)              │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   mov ecx, [r8+rax*4+00752030]                                              │
│       │         │                                                           │
│       │         └─► This reads from a SECOND table at 0x752030              │
│       │             Uses the byte value (0 or 1) as an index                │
│       │             Entry 0 = address of SKIP code                          │
│       │             Entry 1 = address of CONTINUE code                      │
│       │                                                                     │
│       └─► ECX = jump target offset                                          │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   add rcx, r8              ; RCX = RCX + R8 (add base address)              │
│                            ; RCX now = full 64-bit address to jump to       │
│                                                                             │
│   jmp rcx                  ; Jump to the calculated address!                │
│       │                                                                     │
│       └─► If byte was 0x00: jumps to 751CC2 (SKIP)                         │
│           If byte was 0x01: jumps to 751CC4 (CONTINUE to unlock logic)      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

VISUAL:
                    ┌─────────────────┐
     Byte = 0x00 ──►│ JMP to 751CC2   │──► Item SKIPPED
                    │   (skip path)   │
                    └─────────────────┘

                    ┌─────────────────┐
     Byte = 0x01 ──►│ JMP to 751CC4   │──► Continue to ownership check
                    │ (continue path) │
                    └─────────────────┘
```

### PHASE 5A: Skip Path

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CC2  (SKIP PATH - when byte = 0x00)                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   xor cl, cl               ; CL = 0 (clear the register)                    │
│       │                                                                     │
│       └─► XOR a register with itself always results in 0                    │
│           This marks the item as "skipped"                                  │
│           Code then continues to loop end (next item)                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 5B: Continue Path (Level 2 Check)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CC4  (CONTINUE PATH - when byte = 0x01)              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   movzx r12d, cl           ; R12D = CL (save the "enabled" flag)            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CC8  (LEVEL 2 - STEAM/PLATFORM CHECK)                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   xor r12b, 01             ; Toggle the lowest bit of R12                   │
│       │                    ; If R12B was 1, now it's 0. If 0, now it's 1.   │
│       │                                                                     │
│       └─► This flips a flag used later in the unlock process                │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   cmp ebp, 14              ; Compare item index (EBP) with 0x14 (20)        │
│       │                                                                     │
│       └─► Items with index > 20 are "special" and need extra checks         │
│           Index 20 = item_id 0x33 (Alien Shield)                            │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   ja 751F5A                ; "Jump if Above" - if EBP > 0x14, jump to       │
│       │                    ; the DL ownership check directly                │
│       │                                                                     │
│       └─► NOTE: NONE of our problematic items trigger this jump!            │
│           Fashion Collection: EBP = 8  (not > 20, so NO jump)               │
│           8700K:             EBP = 17 (not > 20, so NO jump)                │
│           Alien Shield:      EBP = 20 (not > 20, equal doesn't count)       │
│                                                                             │
│           All problematic items FALL THROUGH to the Jump Table below!       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 6: Jump Table Dispatch (The Router)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751CD5  (JUMP TABLE - Routes each item differently)     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   movsxd rax, ebp          ; RAX = EBP (item index, sign-extended to 64-bit)│
│       │                                                                     │
│       └─► MOVSXD = Move with Sign Extension (32-bit to 64-bit)             │
│           RAX now holds the item index                                      │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   mov ecx, [r8+rax*4+0075206C]                                              │
│       │         │     │                                                     │
│       │         │     └─► 0x75206C is the jump table offset                 │
│       │         │         Full address = 0x14075206C                        │
│       │         │                                                           │
│       │         └─► rax*4 because each entry is 4 bytes                     │
│       │             Index 0: 0x14075206C + 0  = 0x14075206C                  │
│       │             Index 1: 0x14075206C + 4  = 0x140752070                  │
│       │             Index 8: 0x14075206C + 32 = 0x14075208C (Fashion Coll.) │
│       │             Index 17: 0x14075206C + 68 = 0x1407520B0 (8700K)        │
│       │                                                                     │
│       └─► ECX = the 4-byte offset value from the table                      │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   add rcx, r8              ; RCX = offset + 0x140000000 (base address)      │
│                            ; RCX now = full address to jump to              │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   jmp rcx                  ; JUMP to the item-specific handler!             │
│       │                                                                     │
│       └─► THIS IS THE KEY DIVERGENCE POINT!                                │
│           Different items jump to DIFFERENT code:                           │
│                                                                             │
│           ┌────────────────────┬──────────────┬────────────────────────┐    │
│           │ Item               │ Jumps To     │ What Happens           │    │
│           ├────────────────────┼──────────────┼────────────────────────┤    │
│           │ Blazefire Saber    │ 0x140751F45  │ Ownership check called │    │
│           │ Fashion Collection │ 0x140751F5C  │ SKIPS to JE (bad!)     │    │
│           │ 8700K Accessories  │ 0x140751D1D  │ Check called, returns 0│    │
│           └────────────────────┴──────────────┴────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

VISUAL: The Jump Table acts like a phone switchboard, routing each item
        to its own handler code:

                         JUMP TABLE
                    ┌─────────────────┐
    Index 0 (0x00) ─┤ offset: 751F45  ├──► Blazefire handler (works!)
                    ├─────────────────┤
    Index 1 (0x01) ─┤ offset: ?       ├──► ...
                    ├─────────────────┤
    ...             │ ...             │
                    ├─────────────────┤
    Index 8 (0x27) ─┤ offset: 751F5C  ├──► Fashion Collection (BROKEN!)
                    ├─────────────────┤              │
    ...             │ ...             │              │ Skips ownership
                    ├─────────────────┤              │ check entirely!
    Index 17 (0x30)─┤ offset: 751D1D  ├──► 8700K handler (returns "not owned")
                    ├─────────────────┤
    Index 20 (0x33)─┤ offset: 751FBE  ├──► Alien Shield handler
                    └─────────────────┘
```

### PHASE 7A: Working Item Path (Blazefire Saber)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F45  (WORKING ITEMS - e.g., Blazefire Saber)         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   mov edx, 000EEACA        ; EDX = 0xEEACA (ownership check parameter)      │
│       │                    ; This is likely a product/DLC ID                │
│       │                                                                     │
│       └─► Each item type has a different ID for the ownership check         │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   mov rcx, [rax+30]        ; RCX = pointer to some game object              │
│   mov rax, [rcx]           ; RAX = vtable pointer (for virtual function)    │
│   call qword ptr [rax+38]  ; CALL the ownership check function!             │
│       │                                                                     │
│       └─► This calls a function that checks if player owns this item        │
│           Returns: AL = 1 (owned) or AL = 0 (not owned)                     │
│           For Blazefire, this returns 1 (since it's "unobtainable" DLC      │
│           that the game treats as "owned by everyone")                      │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   xor r12b, r12b           ; R12B = 0 (clear flag)                          │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   movzx edx, al            ; EDX = AL (copy return value to EDX)            │
│       │                    ; Now DL (low byte of EDX) = ownership result    │
│       │                                                                     │
│       └─► DL = 1 means "owned", DL = 0 means "not owned"                   │
│                                                                             │
│   ─────── CONTINUES TO PHASE 8 (OWNERSHIP GATE) ───────                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 7B: Fashion Collection Path (BROKEN!)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F5C  (FASHION COLLECTION - PROBLEMATIC!)             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ╔═══════════════════════════════════════════════════════════════════╗     │
│   ║  PROBLEM: Fashion Collection jumps DIRECTLY here, skipping the    ║     │
│   ║  entire ownership check at 751F45-751F57!                         ║     │
│   ║                                                                   ║     │
│   ║  Working items:     751F45 → 751F4A → 751F4E → 751F51 → 751F57    ║     │
│   ║                                         (call)     (set DL)       ║     │
│   ║                                                                   ║     │
│   ║  Fashion Collection: ─────────────────────────────────► 751F5C   ║     │
│   ║                      (SKIPS EVERYTHING!)                          ║     │
│   ╚═══════════════════════════════════════════════════════════════════╝     │
│                                                                             │
│   At this point:                                                            │
│   - DL register = 0 (was never set to 1 because check was skipped!)         │
│   - The JE instruction below will see DL=0 and skip the unlock              │
│                                                                             │
│   ─────── CONTINUES TO PHASE 8 (OWNERSHIP GATE) ───────                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 7C: 8700K Path (Different but Still Broken)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751D1D  (8700K ACCESSORIES - ALSO PROBLEMATIC!)         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   lea rcx, [ffxv_s.exe+44B7B90]   ; Setup for function call                 │
│   call qword ptr [ffxv_s.exe+3067E68]  ; Some initialization call           │
│                                                                             │
│   mov rcx, [rax+30]               ; Get object pointer                      │
│   mov rax, [rcx]                  ; Get vtable                              │
│   mov edx, 000C049A               ; EDX = 0xC049A (8700K's ownership ID)    │
│       │                                                                     │
│       └─► Different ID than Blazefire's 0xEEACA!                           │
│           This is probably the Intel 8700K promotion ID                     │
│                                                                             │
│   call qword ptr [rax+38]         ; Call ownership check                    │
│       │                                                                     │
│       └─► This DOES call the ownership check...                            │
│           But it returns AL = 0 (not owned) because you don't have         │
│           the Intel promotion registered to your account!                   │
│                                                                             │
│   jmp 751F57                      ; Jump to the DL setup code               │
│                                                                             │
│   ─────── AT 751F57 ───────                                                 │
│                                                                             │
│   movzx edx, al                   ; DL = 0 (from the failed check)          │
│                                                                             │
│   ─────── CONTINUES TO PHASE 8 (OWNERSHIP GATE) ───────                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 8: The Ownership Gate (Where Everything Converges)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F5A  (OWNERSHIP GATE - All paths lead here!)         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ╔═══════════════════════════════════════════════════════════════════╗     │
│   ║  THIS IS THE CRITICAL CHECKPOINT                                  ║     │
│   ║  All item paths converge here for the final ownership decision    ║     │
│   ╚═══════════════════════════════════════════════════════════════════╝     │
│                                                                             │
│   test dl, dl              ; Test if DL is zero                             │
│       │                    ; Sets Zero Flag (ZF) if DL = 0                  │
│       │                                                                     │
│       └─► TEST performs AND operation and sets flags                        │
│           DL AND DL = DL, so if DL = 0, Zero Flag is set                    │
│           if DL = 1 (or any non-zero), Zero Flag is NOT set                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F5C  (THE GATE - Jump or Fall Through)               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   je 751FAD                ; "Jump if Equal" (really: Jump if Zero Flag set)│
│       │                                                                     │
│       │   ┌─────────────────────────────────────────────────────────────┐   │
│       │   │ JE = Jump if Equal = Jump if Zero Flag is set              │   │
│       │   │                                                             │   │
│       │   │ If DL = 0: Zero Flag IS set     → JUMP to 751FAD (skip!)   │   │
│       │   │ If DL = 1: Zero Flag NOT set    → Fall through (unlock!)   │   │
│       │   └─────────────────────────────────────────────────────────────┘   │
│       │                                                                     │
│       └─► This is the "gate" that decides: UNLOCK or SKIP?                 │
│                                                                             │
│   BYTES: 74 4F                                                              │
│          │  │                                                               │
│          │  └─► 4F = offset to jump (79 bytes forward to 751FAD)           │
│          └─► 74 = opcode for JE (Jump if Equal/Zero)                       │
│                                                                             │
│   ═══════════════════════════════════════════════════════════════════════   │
│   WHY PATCHING THIS AFFECTS ALL ITEMS:                                      │
│                                                                             │
│   ALL items (Fashion Collection, 8700K, Blazefire, etc.) eventually         │
│   reach this instruction. If we NOP it (replace with 90 90), then           │
│   ALL items fall through to the unlock code, regardless of DL value!        │
│   ═══════════════════════════════════════════════════════════════════════   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

VISUAL: The Ownership Gate

                ALL ITEMS CONVERGE HERE
                         │
                         ▼
              ┌─────────────────────┐
              │  test dl, dl        │  ← Check DL register
              │  (at 751F5A)        │
              └─────────────────────┘
                         │
                         ▼
              ┌─────────────────────┐
              │  je 751FAD          │  ← THE GATE (at 751F5C)
              │  (bytes: 74 4F)     │
              └─────────────────────┘
                         │
         ┌───────────────┴───────────────┐
         │                               │
    DL = 0 (not owned)              DL = 1 (owned)
    ZF = 1 (set)                    ZF = 0 (not set)
         │                               │
         ▼                               ▼
    JUMP to 751FAD                  FALL THROUGH
    (skip unlock)                   (continue to unlock)
         │                               │
         ▼                               ▼
    ┌──────────┐                  ┌──────────────┐
    │ SKIP!    │                  │ UNLOCK ITEM! │
    │ Next item│                  │ Call 7A5790  │
    └──────────┘                  └──────────────┘
```

### PHASE 9: The Unlock Code

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F5E  (UNLOCK PATH - When DL = 1)                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   jmp 751F6D               ; Jump to unlock preparation                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F60  (RSI SETUP - Some items land here directly)     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   mov rsi, [rsp+000000A0]  ; RSI = pointer from stack (needed for unlock)   │
│                            ; This sets up the object pointer for the call   │
│                                                                             │
│   mov ebx, 00000001        ; EBX = 1                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F6D  (R12B CHECK)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   test r12b, r12b          ; Check R12B flag                                │
│   je 751F95                ; If R12B = 0, jump to 751F95                    │
│                                                                             │
│   ... (some additional logic for R12B = 1 case) ...                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F95  (FINAL UNLOCK CALL SETUP)                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   lea edx, [rbp+20]        ; EDX = EBP + 0x20 = item_id + 1                 │
│                            ; (slight adjustment for the function call)      │
│                                                                             │
│   mov r8b, 01              ; R8B = 1 (parameter)                            │
│                                                                             │
│   mov rcx, rsi             ; RCX = RSI (object pointer, first parameter)    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751F9E  (THE UNLOCK FUNCTION CALL!)                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ╔═══════════════════════════════════════════════════════════════════╗     │
│   ║  THIS IS WHERE THE ITEM ACTUALLY GETS UNLOCKED!                   ║     │
│   ╚═══════════════════════════════════════════════════════════════════╝     │
│                                                                             │
│   call ffxv_s.exe+7A5790   ; Call the unlock function!                      │
│       │                                                                     │
│       └─► Parameters:                                                       │
│           RCX = object pointer (from RSI)                                   │
│           EDX = item_id + 1                                                 │
│           R8B = 1                                                           │
│                                                                             │
│           This function grants the item to the player!                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751FA3                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   jmp 751FAD               ; Jump to loop end                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### PHASE 10: Loop End

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ADDRESS: ffxv_s.exe+751FAD  (LOOP END - All paths end here)                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   inc ebp                  ; EBP = EBP + 1 (next item index)                │
│   ...                                                                       │
│   cmp eax, 36              ; Compare with max                               │
│   jl 751CA0                ; If not done, jump back to loop start           │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   Loop repeats for each item from index -32 to ~20 (item IDs 0x00 to 0x33)  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Why Some Items Don't Unlock

### Summary Table

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     WHY EACH ITEM CATEGORY FAILS                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  WORKING ITEMS (Blazefire, Twitch Prime, etc.):                             │
│  ├─► Jump table sends them to 751F45                                        │
│  ├─► Ownership check is called                                              │
│  ├─► Check returns AL=1 (owned)                                             │
│  ├─► DL=1 at the gate                                                       │
│  └─► Gate passed → UNLOCKED!                                                │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  FASHION COLLECTION (0x27):                                                 │
│  ├─► Jump table sends it to 751F5C (DIRECTLY to the gate!)                  │
│  ├─► SKIPS the ownership check entirely                                     │
│  ├─► DL=0 (was never set)                                                   │
│  └─► Gate fails → NOT UNLOCKED                                              │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  8700K ACCESSORIES (0x30):                                                  │
│  ├─► Jump table sends it to 751D1D                                          │
│  ├─► Ownership check IS called (with ID 0x000C049A)                         │
│  ├─► Check returns AL=0 (Intel promo not owned)                             │
│  ├─► DL=0 at the gate                                                       │
│  └─► Gate fails → NOT UNLOCKED                                              │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  HALF-LIFE ITEMS (0x29, 0x2C, 0x2D, 0x2E, 0x2F):                            │
│  ├─► Jump to different addresses (751FBF, 7520FF, etc.)                     │
│  ├─► May have different check mechanism (CF flag mentioned in notes)        │
│  └─► Status: NOT FULLY TRACED YET                                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Visual Memory Map

```
FFXV_S.EXE MEMORY LAYOUT (Relevant Sections)
═══════════════════════════════════════════════════════════════════════════════

BASE ADDRESS: 0x140000000

    ┌─────────────────────────────────────────────────────────────────────────┐
    │                              CODE SECTION                               │
    │                         (Executable Instructions)                       │
    ├─────────────────────────────────────────────────────────────────────────┤
    │                                                                         │
    │   0x140751C8B ─┬─ UNLOCK LOOP START                                     │
    │               │   mov ebp, FFFFFFE0                                     │
    │               │                                                         │
    │   0x140751CA0 ─┼─ ITEM ID CALCULATION                                   │
    │               │   lea eax, [rbp+1F]                                     │
    │               │                                                         │
    │   0x140751CA5 ─┼─ BOUNDS CHECK                                          │
    │               │   cmp eax, 33                                           │
    │               │   ja 751CC4                                             │
    │               │                                                         │
    │   0x140751CAC ─┼─ BYTE TABLE LOOKUP                                     │
    │               │   movzx eax, byte ptr [r8+rax+00752038]                 │
    │               │   ...                                                   │
    │               │   jmp rcx  (to skip or continue path)                   │
    │               │                                                         │
    │   0x140751CC2 ─┼─ SKIP PATH                                             │
    │               │   xor cl, cl                                            │
    │               │                                                         │
    │   0x140751CC4 ─┼─ CONTINUE PATH                                         │
    │               │   movzx r12d, cl                                        │
    │               │                                                         │
    │   0x140751CC8 ─┼─ LEVEL 2 CHECK                                         │
    │               │   xor r12b, 01                                          │
    │               │   cmp ebp, 14                                           │
    │               │   ja 751F5A                                             │
    │               │                                                         │
    │   0x140751CD5 ─┼─ JUMP TABLE DISPATCH                                   │
    │               │   movsxd rax, ebp                                       │
    │               │   mov ecx, [r8+rax*4+0075206C]                          │
    │               │   add rcx, r8                                           │
    │               │   jmp rcx  ─────────────────────────────┐               │
    │               │                                         │               │
    │               │   ┌─────────────────────────────────────┼───────────┐   │
    │               │   │         ITEM-SPECIFIC HANDLERS      │           │   │
    │               │   │                                     ▼           │   │
    │   0x140751D1D ─┼──┼─ 8700K HANDLER ◄────────────────────            │   │
    │               │   │   lea rcx, [...]                                │   │
    │               │   │   call [...]                                    │   │
    │               │   │   mov edx, 000C049A                             │   │
    │               │   │   call [rax+38]                                 │   │
    │               │   │   jmp 751F57 ──────────────────────┐            │   │
    │               │   │                                    │            │   │
    │               │   │        ... other handlers ...      │            │   │
    │               │   │                                    │            │   │
    │   0x140751F45 ─┼──┼─ WORKING ITEMS HANDLER ◄───────────┼────────────┘   │
    │               │   │   mov edx, 000EEACA                │                │
    │               │   │   mov rcx, [rax+30]                │                │
    │               │   │   mov rax, [rcx]                   │                │
    │               │   │   call qword ptr [rax+38]          │                │
    │               │   │   xor r12b, r12b                   │                │
    │               │   │                                    │                │
    │   0x140751F57 ─┼──┼─ DL SETUP ◄────────────────────────┘                │
    │               │   │   movzx edx, al                                     │
    │               │   │                                                     │
    │   0x140751F5A ─┼──┼─ OWNERSHIP GATE (test dl, dl)                       │
    │               │   │                                                     │
    │   0x140751F5C ─┼──┼─ THE GATE (je 751FAD) ◄─── Fashion Collection lands │
    │               │   │   │                        here DIRECTLY!           │
    │               │   │   │                                                 │
    │               │   │   ├─► DL=0? JUMP to 751FAD (skip)                   │
    │               │   │   └─► DL=1? Fall through (unlock)                   │
    │               │   │                                                     │
    │   0x140751F5E ─┼──┼─ jmp 751F6D (if owned)                              │
    │               │   │                                                     │
    │   0x140751F60 ─┼──┼─ RSI SETUP                                          │
    │               │   │   mov rsi, [rsp+A0]                                 │
    │               │   │   mov ebx, 1                                        │
    │               │   │                                                     │
    │   0x140751F6D ─┼──┼─ R12B CHECK                                         │
    │               │   │   test r12b, r12b                                   │
    │               │   │   je 751F95                                         │
    │               │   │                                                     │
    │   0x140751F95 ─┼──┼─ UNLOCK CALL SETUP                                  │
    │               │   │   lea edx, [rbp+20]                                 │
    │               │   │   mov r8b, 01                                       │
    │               │   │   mov rcx, rsi                                      │
    │               │   │                                                     │
    │   0x140751F9E ─┼──┼─ UNLOCK FUNCTION CALL                               │
    │               │   │   call ffxv_s.exe+7A5790  ◄─── ITEM GRANTED HERE!   │
    │               │   │                                                     │
    │   0x140751FA3 ─┼──┼─ jmp 751FAD                                         │
    │               │   │                                                     │
    │   0x140751FAD ─┼──┴─ LOOP END                                           │
    │               │      inc ebp                                            │
    │               │      ...                                                │
    │               │      jl 751CA0  (repeat loop)                           │
    │               │                                                         │
    └───────────────┴─────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────────────┐
    │                              DATA SECTION                               │
    │                         (Tables and Variables)                          │
    ├─────────────────────────────────────────────────────────────────────────┤
    │                                                                         │
    │   0x140752030 ─── HANDLER OFFSET TABLE (for byte 0/1 dispatch)          │
    │                   [0] = offset to skip path (751CC2)                    │
    │                   [1] = offset to continue path (751CC4)                │
    │                                                                         │
    │   0x140752038 ─┬─ BYTE TABLE START (Item Enable Flags)                  │
    │               │   Each byte = 0x00 (disabled) or 0x01 (enabled)         │
    │               │                                                         │
    │               ├─ [+0x00] 0x140752038 = Blazefire Saber                  │
    │               ├─ [+0x13] 0x14075204B = Noodle Helmet                    │
    │               ├─ [+0x20] 0x140752058 = Weatherworn Regalia Decal        │
    │               ├─ [+0x27] 0x14075205F = Fashion Collection ★             │
    │               ├─ [+0x30] 0x140752068 = 8700K Accessories ★              │
    │               └─ [+0x33] 0x14075206B = Alien Shield ★                   │
    │                                                                         │
    │   0x14075206C ─┬─ JUMP TABLE START (Per-Item Code Destinations)         │
    │               │   Each entry = 4-byte offset                            │
    │               │   Final address = 0x140000000 + offset                  │
    │               │                                                         │
    │               ├─ [Index 0]  0x14075206C = 7675109 → 751F45 (works!)     │
    │               ├─ [Index 8]  0x14075208C = 7675132 → 751F5C (broken!)    │
    │               ├─ [Index 17] 0x1407520B0 = 7675165 → 751D1D (broken!)    │
    │               └─ [Index 20] 0x1407520BC = 7675198 → 751FBE              │
    │                                                                         │
    └─────────────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────────────┐
    │                          UNLOCK FUNCTION                                │
    ├─────────────────────────────────────────────────────────────────────────┤
    │                                                                         │
    │   0x1407A5790 ─── THE ACTUAL UNLOCK FUNCTION                            │
    │                   Called with:                                          │
    │                   - RCX = object pointer                                │
    │                   - EDX = item_id + 1                                   │
    │                   - R8B = 1                                             │
    │                                                                         │
    │                   This function grants the item to the player!          │
    │                                                                         │
    └─────────────────────────────────────────────────────────────────────────┘
```

---

## Register Reference

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    REGISTER USAGE IN UNLOCK LOOP                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  REGISTER │ SIZE   │ PURPOSE IN THIS CODE                                   │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RBP/EBP  │ 64/32  │ Loop counter (item index)                              │
│           │        │ Starts at -32 (0xFFFFFFE0), increments each iteration  │
│           │        │ Item ID = EBP + 0x1F                                   │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RAX/EAX  │ 64/32  │ Item ID (after lea eax, [rbp+1F])                      │
│           │        │ Also used for byte table lookup result                 │
│           │        │ Also used for function return values                   │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  AL       │ 8      │ Lowest byte of RAX                                     │
│           │        │ Ownership check return value (0=not owned, 1=owned)    │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RDX/EDX  │ 64/32  │ Ownership check parameter (product/DLC ID)             │
│           │        │ Examples: 0x000EEACA (Blazefire), 0x000C049A (8700K)   │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  DL       │ 8      │ Lowest byte of RDX                                     │
│           │        │ THE CRITICAL OWNERSHIP FLAG                            │
│           │        │ 0 = not owned (gate fails)                             │
│           │        │ 1 = owned (gate passes)                                │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RCX/ECX  │ 64/32  │ Jump target calculation                                │
│           │        │ Also first parameter for function calls                │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  R8       │ 64     │ Base address: 0x140000000                              │
│           │        │ Added to offsets to get full addresses                 │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  R12/R12B │ 64/8   │ Flag used in unlock logic                              │
│           │        │ Toggled with XOR r12b, 01                              │
│           │        │ Checked at 751F6D with test r12b, r12b                 │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RSI      │ 64     │ Object pointer for unlock function call                │
│           │        │ Set at 751F60: mov rsi, [rsp+A0]                       │
│  ─────────┼────────┼──────────────────────────────────────────────────────  │
│  RBX/EBX  │ 64/32  │ Set to 1 at 751F68                                     │
│           │        │ Used in unlock logic                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

CPU FLAGS RELEVANT TO THIS CODE:
────────────────────────────────
  Zero Flag (ZF):
    - Set when result of operation is zero
    - Used by JE (Jump if Equal) - really "Jump if Zero Flag set"
    - test dl, dl sets ZF=1 if DL=0, ZF=0 if DL≠0

  Carry Flag (CF):
    - Set on unsigned overflow
    - Mentioned in notes for Half-Life items (may need CF=0)
    - JA (Jump if Above) checks both ZF=0 AND CF=0
```

---

## Quick Reference Card

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FFXV UNLOCK QUICK REFERENCE                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  TO ENABLE AN ITEM:                                                         │
│  1. Find item's byte address: 0x140752038 + item_id                         │
│  2. Write 0x01 to that address                                              │
│  3. Go to game menu to trigger unlock check                                 │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  THE OWNERSHIP GATE (why some items don't unlock):                          │
│  Location: 0x140751F5C                                                      │
│  Instruction: je 751FAD (bytes: 74 4F)                                      │
│  Behavior: If DL=0, skip unlock. If DL=1, proceed to unlock.                │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  TO UNLOCK ALL PLATFORM EXCLUSIVES (brute force):                           │
│  1. Change bytes at 0x140751F5C from "74 4F" to "90 90" (NOP NOP)           │
│  2. This removes the ownership check for ALL items                          │
│  3. Set byte=01 for items you want, trigger menu                            │
│  WARNING: This unlocks MORE than just the items you enabled!                │
│                                                                             │
│  ─────────────────────────────────────────────────────────────────────────  │
│                                                                             │
│  KEY ADDRESSES:                                                             │
│  Byte Table:     0x140752038                                                │
│  Jump Table:     0x14075206C                                                │
│  Ownership Gate: 0x140751F5C                                                │
│  Unlock Function: 0x1407A5790                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```
