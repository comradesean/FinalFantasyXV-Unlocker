# FFXV Windows Edition - Complete Unlock System Reference

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Key Findings](#key-findings)
3. [Available Unlock Methods](#available-unlock-methods)
4. [Technical Fundamentals](#technical-fundamentals)
5. [Memory Structures](#memory-structures)
6. [Complete Item Reference](#complete-item-reference)
7. [Code Flow Analysis](#code-flow-analysis)
8. [Detailed Assembly Walkthrough](#detailed-assembly-walkthrough)
9. [Why Some Items Don't Unlock](#why-some-items-dont-unlock)
10. [Anti-Tamper Protection](#anti-tamper-protection)
11. [Patch Byte Reference](#patch-byte-reference)
12. [Register Reference](#register-reference)
13. [Cheat Engine Scripts](#cheat-engine-scripts)
14. [Attempted Solutions Log](#attempted-solutions-log)
15. [Unexplored Areas](#unexplored-areas)

---

## Executive Summary

### The Problem

FFXV Windows Edition contains numerous exclusive items tied to:
- Discontinued promotions (Twitch Prime, King's Knight mobile game)
- Platform-specific purchases (Steam, Origin, Microsoft Store)
- Limited-time promotional events (Intel 8700K, Valve/Half-Life crossover)

These items exist in the game code but are inaccessible through normal gameplay.

### The Solution

After extensive reverse engineering and live debugging, we determined that:

1. **Individual selection of Steam Exclusive and Promotional items is NOT possible** due to FFXV's anti-tamper protection
2. **Data modifications work** - Changes to byte table and jump table do not trigger crashes
3. **Code modifications crash** - Any modification to executable code, even a single byte, causes immediate crash
4. **Two "Unlock All" methods exist** - One includes Steam Workshop items, one excludes them

### Quick Start

For most users who just want to unlock items:

| Goal | Method | Patches Needed |
|------|--------|----------------|
| Unlock selectable items only | Byte table | Set byte to 0x01 at item address |
| Unlock ALL platform exclusives (no Workshop) | Unlock 3 | NOP at 751F5C |
| Unlock EVERYTHING (with Workshop) | Unlock 1 + 2 | Patches at 751CA8 and 751CC8 |

---

## Key Findings

1. **Anti-Tamper Protection**: FFXV crashes when any executable code is modified, even by a single byte
2. **Data Modifications Work**: Changes to data tables (byte table, jump table) do not trigger crashes
3. **The Ownership Gate**: All items converge at address `751F5A` where DL register determines unlock
4. **Per-Item Control Impossible**: The only way to selectively set DL=1 requires code modification, which crashes the game
5. **Two Table System**: Byte table controls ON/OFF, Jump table routes to item-specific handlers

---

## Available Unlock Methods

### Method 1: Byte Table Only (Selectable Items)

**Use Case**: Unlock items that work with simple byte table modification

**How It Works**:
- Set the byte at `0x140752038 + item_id` to `0x01`
- Load the game menu to trigger unlock check
- Item is granted

**Items This Works For**:
- All Normally Unavailable items (Blazefire Saber, Noodle Helmet, etc.)
- All Twitch Prime items
- Microsoft Store Exclusive (Powerup Pack)
- Origin Exclusives (Decal Selection, Sims 4 Pack)

**Items This Does NOT Work For** (need Unlock 1+2 or Unlock 3):
- Steam Exclusives (Fashion Collection, Half-Life items)
- Promotional items (8700K, Alien Shield)

---

### Method 2: Unlock 1 + Unlock 2 (Everything with Steam Workshop)

**Use Case**: Unlocks ALL exclusive items including Steam Workshop items

**Patches Required**:
- **Unlock 1** (Bounds Bypass): `ffxv_s.exe+751CA8` - Change `77 1A` to `EB 1A`
- **Unlock 2** (Steam Bypass): `ffxv_s.exe+751CC8` - Change `41 80 F4 01 83 FD 14` to `4D 31 E4 41 38 D4 F8`

**IMPORTANT**: Unlock 2 requires Unlock 1 to be enabled first, or the game will crash!

**Items Unlocked**:
- All Normally Unavailable items
- All Twitch Prime items
- All Steam Exclusives (Fashion Collection, Half-Life items)
- All Origin Exclusives
- Microsoft Store Exclusive
- All Promotional items (8700K, Alien Shield)
- Steam Workshop items (HEV Suit COMRADES, Scientist Glasses COMRADES, Crowbar COMRADES)

**Warning**: This method unlocks Steam Workshop items which may affect multiplayer/workshop functionality.

---

### Method 3: Unlock 3 (Everything without Steam Workshop)

**Use Case**: Unlocks exclusive items WITHOUT affecting Steam Workshop

**Patch Required**:
- **Unlock 3** (DL Bypass): `ffxv_s.exe+751F5C` - Change `74 4F` to `90 90` (NOP the je)

**Items Unlocked**:
- All Normally Unavailable items
- All Twitch Prime items
- All Steam Exclusives (main game versions)
- All Origin Exclusives
- Microsoft Store Exclusive
- All Promotional items

**Does NOT Unlock**:
- Steam Workshop items (HEV Suit COMRADES, Scientist Glasses COMRADES, Crowbar COMRADES)

**Note**: Due to anti-tamper, this may only work via Cheat Engine with bypass enabled.

---

## Technical Fundamentals

### Memory Addresses

Every piece of data in a running program lives at a specific "address" in memory. In FFXV, addresses look like `0x140752038` - this is a hexadecimal (base-16) number.

### Hexadecimal Primer

- Hex uses 0-9 and A-F (where A=10, B=11, C=12, D=13, E=14, F=15)
- `0x10` in hex = 16 in decimal
- `0x1F` in hex = 31 in decimal
- `0xFF` in hex = 255 in decimal

### Assembly Language Basics

Common instructions used in FFXV's unlock code:
- `mov eax, 5` - Put the value 5 into register EAX
- `cmp eax, 10` - Compare EAX with 10 (sets flags for later jumps)
- `je 751FAD` - "Jump if Equal" - go to address 751FAD if the last comparison was equal
- `ja 751F5A` - "Jump if Above" - go to address if greater (unsigned)
- `jmp 751F6D` - "Jump" unconditionally to address
- `call 7A5790` - Call a function at that address
- `nop` - "No Operation" - do nothing (used to disable code)
- `test dl, dl` - AND DL with itself, sets Zero Flag if DL=0
- `xor cl, cl` - XOR register with itself, always results in 0

### Registers

FFXV uses a 64-bit CPU:
- **RAX, RBX, RCX, RDX** - General purpose (64-bit)
- **EAX, EBX, ECX, EDX** - Lower 32 bits of the above
- **AL, BL, CL, DL** - Lowest 8 bits (1 byte) of the above
- **RBP** - Often used as a loop counter or base pointer
- **R8-R15** - Additional registers (64-bit), R12B is lowest byte of R12

---

## Memory Structures

### Byte Table (Item Enable Flags)

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
│  Formula: Item Address = 0x140752038 + Item_ID                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Example**:
- Blazefire Saber (Item ID `0x00`): `0x140752038 + 0x00` = `0x140752038`
- Fashion Collection (Item ID `0x27`): `0x140752038 + 0x27` = `0x14075205F`

### Jump Table (Per-Item Code Routing)

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
│  NOTE: This is DATA, not code - modifications don't crash!                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Item Index Calculation

```
item_index (EBP) = item_id - 0x1F
item_id (EAX) = EBP + 0x1F
```

**Examples**:
| Item | Item ID | EBP Value (Index) |
|------|---------|-------------------|
| Blazefire Saber | `0x00` | `0xFFFFFFE1` (-31) |
| Kooky Chocobo | `0x22` | `0x03` |
| Fashion Collection | `0x27` | `0x08` |
| 8700K | `0x30` | `0x11` |
| Alien Shield | `0x33` | `0x14` |

---

## Complete Item Reference

### Normally Unavailable Items

Items from discontinued promotions or removed features. **Work with byte table alone.**

| Address | Item ID | Index | Item Name | Original Promotion |
|---------|---------|-------|-----------|-------------------|
| `0x140752038` | `0x00` | -31 | Blazefire Saber | Unknown |
| `0x14075204B` | `0x13` | -12 | Noodle Helmet | Cup Noodle Promotion |
| `0x14075204F` | `0x17` | -8 | King's Knight Sticker Set | King's Knight Mobile Game |
| `0x140752053` | `0x1B` | -4 | Kingglaives Pack (COMRADES) | Unknown |
| `0x140752054` | `0x1C` | -3 | Party Pack (COMRADES) | Unknown |
| `0x140752055` | `0x1D` | -2 | Memories of KING'S KNIGHT | King's Knight Mobile Game |
| `0x140752056` | `0x1E` | -1 | King's Knight Tee | King's Knight Mobile Game |

### Twitch Prime Drops

Items from the discontinued Twitch Prime promotion. **Work with byte table alone.**

| Address | Item ID | Index | Item Name | Bundle Pair |
|---------|---------|-------|-----------|-------------|
| `0x140752058` | `0x20` | 1 | Weatherworn Regalia Decal | 16 Rare Coins |
| `0x140752059` | `0x21` | 2 | Kooky Chocobo Tee (COMRADES) | 100 AP |
| `0x14075205A` | `0x22` | 3 | Kooky Chocobo | 10,000 GIL |
| `0x14075205B` | `0x23` | 4 | 10,000 GIL | Kooky Chocobo |
| `0x14075205C` | `0x24` | 5 | 100 AP | Kooky Chocobo Tee |
| `0x14075205D` | `0x25` | 6 | 16 Rare Coins | Weatherworn Regalia Decal |

### Microsoft Store Exclusive

**Works with byte table alone.**

| Address | Item ID | Index | Item Name | Notes |
|---------|---------|-------|-----------|-------|
| `0x14075205E` | `0x26` | 7 | FFXV Powerup Pack | Dodanuki sword, 10 Phoenix Downs, 10 Elixirs |

### Steam Exclusives ★ NO INDIVIDUAL SELECTION

**Require Unlock 1+2 or Unlock 3. Cannot be individually selected due to anti-tamper.**

| Address | Item ID | Index | Item Name | Notes |
|---------|---------|-------|-----------|-------|
| `0x14075205F` | `0x27` | 8 | FFXV Fashion Collection | Steam Pre-Order Bonus |
| `0x140752061` | `0x29` | 10 | HEV Suit (COMRADES) | Half-Life Crossover, Workshop item |
| `0x140752064` | `0x2C` | 13 | Scientist Glasses (COMRADES) | Half-Life Crossover, Workshop item |
| `0x140752065` | `0x2D` | 14 | Crowbar (COMRADES) | Half-Life Crossover, Workshop item |
| `0x140752066` | `0x2E` | 15 | Half-Life Costume | Half-Life Crossover |
| `0x140752067` | `0x2F` | 16 | Crowbar | Half-Life Crossover |

### Origin Exclusives

**Work with byte table alone.**

| Address | Item ID | Index | Item Name | Notes |
|---------|---------|-------|-----------|-------|
| `0x140752060` | `0x28` | 9 | FFXV Decal Selection | Origin Pre-Order Bonus |
| `0x140752062` | `0x2A` | 11 | FINAL FANTASY XV THE SIMS 4 PACK | Origin Exclusive |

### Promotional Items ★ NO INDIVIDUAL SELECTION

**Require Unlock 1+2 or Unlock 3. Cannot be individually selected due to anti-tamper.**

| Address | Item ID | Index | Item Name | Promotion |
|---------|---------|-------|-----------|-----------|
| `0x140752068` | `0x30` | 17 | 8700K Accessories | Intel i7-8700K Promotion |
| `0x14075206B` | `0x33` | 20 | Alien Shield | Unknown Promotion |

### Unknown/Unused Item Slots

| Item ID Range | Address Range | Status |
|---------------|---------------|--------|
| `0x01` - `0x12` | `0x140752039` - `0x14075204A` | Unknown |
| `0x14` - `0x16` | `0x14075204C` - `0x14075204E` | Unknown |
| `0x18` - `0x1A` | `0x140752050` - `0x140752052` | Unknown |
| `0x1F` | `0x140752057` | Unknown |
| `0x2B` | `0x140752063` | Unknown |
| `0x31` | `0x140752069` | Episode Ardyn (Paid DLC - Excluded) |
| `0x32` | `0x14075206A` | Unknown |

---

## Code Flow Analysis

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FFXV EXCLUSIVE ITEM UNLOCK LOOP                          │
│                                                                             │
│         Triggered when: Loading main menu or in-game menu                   │
│         Location: ffxv_s.exe + 0x751C8B through 0x751FC1                    │
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
            │ SKIP ITEM     │               │ STEP 2: LEVEL 2 CHECK     │
            │ (next item)   │               │ (Steam/platform check)    │
            └───────────────┘               └───────────────────────────┘
                                                        │
                                                        ▼
                                            ┌───────────────────────────┐
                                            │ STEP 3: JUMP TABLE        │
                                            │ Route to item handler     │
                                            └───────────────────────────┘
                                                        │
                    ┌───────────────────────────────────┼───────────────────┐
                    │                                   │                   │
                    ▼                                   ▼                   ▼
        ┌───────────────────────┐       ┌───────────────────┐   ┌───────────────────┐
        │ WORKING ITEMS         │       │ FASHION COLLECTION │   │ 8700K             │
        │ (e.g., Blazefire)     │       │                    │   │                   │
        │ → Goes to 0x751F45    │       │ → Goes to 0x751F5C │   │ → Goes to 0x751D1D│
        │ → Calls ownership     │       │ → SKIPS ownership  │   │ → Calls ownership │
        │   check function      │       │   check entirely!  │   │   check, returns 0│
        │ → Sets DL=1           │       │ → DL stays 0       │   │ → DL=0            │
        └───────────────────────┘       └───────────────────┘   └───────────────────┘
                    │                               │                       │
                    └───────────────────────────────┼───────────────────────┘
                                                    │
                                                    ▼
                                    ┌───────────────────────────────┐
                                    │ STEP 4: OWNERSHIP GATE        │
                                    │ "Does the player own this?"   │
                                    │                               │
                                    │ test dl, dl (at 0x751F5A)     │
                                    │ je 751FAD   (at 0x751F5C)     │
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
```

### Detailed Code Flow

```
LOOP START (751C8B)
    │   mov ebp, FFFFFFE0  ; EBP = -32 (starting index)
    │
    ▼
ITEM ID CALCULATION (751CA2)
    │   lea eax, [rbp+1F]  ; EAX = item_id
    │
    ▼
LEVEL 1 - BOUNDS CHECK (751CA5)
    │   cmp eax, 33        ; max item_id = 0x33
    │   ja 751CC4          ; if above, skip to unlock path
    │
    ▼
BYTE TABLE LOOKUP (751CAC)
    │   movzx eax, byte ptr [r8+rax+00752038]  ; read byte
    │   mov ecx, [r8+rax*4+00752030]           ; get handler offset
    │   add rcx, r8                             ; calculate address
    │   jmp rcx                                 ; jump to handler
    │
    ├─► byte=0: Routes to SKIP PATH (751CC2)
    │       xor cl, cl  ; item skipped
    │
    └─► byte=1: Routes to UNLOCK PATH (751CC4)
            │   movzx r12d, cl
            │
            ▼
    LEVEL 2 CHECK (751CC8)
        │   xor r12b, 01         ; toggle r12b
        │   cmp ebp, 14          ; compare index with 0x14
        │   ja 751F5A            ; if index > 0x14, jump to ownership gate
        │
        ▼
    JUMP TABLE DISPATCH (751CD5)
        │   movsxd rax, ebp                    ; rax = item_index
        │   mov ecx, [r8+rax*4+0075206C]       ; load jump offset
        │   add rcx, r8                        ; add base address
        │   jmp rcx                            ; jump to item handler
        │
        ▼
    ITEM HANDLERS (various addresses)
        │   Each item has specific handler
        │   Sets DL = 0 or 1 based on ownership check
        │
        ▼
    OWNERSHIP GATE (751F5A) ◄── ALL items converge here
        │   test dl, dl
        │   je 751FAD (skip if DL=0)
        │
        ├─► DL=0: SKIP unlock (jump to 751FAD)
        └─► DL=1: PROCEED to unlock
                    │
                    ▼
            UNLOCK SETUP (751F60 - 751F95)
                │   mov rsi, [rsp+A0]    ; get object pointer
                │   lea edx, [rbp+20]    ; EDX = item_id
                │   mov r8b, 01          ; parameter
                │   mov rcx, rsi         ; object pointer
                │
                ▼
            UNLOCK FUNCTION (751F9E)
                │   call ffxv_s.exe+7A5790  ; ITEM GRANTED!
                │
                ▼
            LOOP END (751FAD)
                    inc ebp              ; next item index
                    jl 751CA0            ; repeat for next item
```

---

## Detailed Assembly Walkthrough

### Phase 1: Loop Initialization (751C8B)

```asm
mov ebp, FFFFFFE0        ; EBP = -32 (starting index)
                         ; Item ID = EBP + 0x1F
                         ; When EBP = -32, item_id = -1 (before first item)
                         ; When EBP = -31, item_id = 0 (Blazefire Saber)
```

### Phase 2: Item ID Calculation (751CA0-751CA2)

```asm
mov cl, 01               ; CL = 1 (used later)
lea eax, [rbp+1F]        ; EAX = EBP + 0x1F = item_id
                         ; LEA = "Load Effective Address" - does math
                         ; Example: When EBP = 8, EAX = 8 + 31 = 39 = 0x27
```

### Phase 3: Bounds Check (751CA5-751CA8)

```asm
cmp eax, 33              ; Compare EAX with 0x33 (max item ID = 51)
ja 751CC4                ; JA = Jump if Above (unsigned)
                         ; If item_id > 51, skip to unlock path
                         ; This is a safety check for invalid item IDs
```

### Phase 4: Byte Table Lookup (751CAC-751CC0)

```asm
movzx eax, byte ptr [r8+rax+00752038]
; MOVZX = "Move with Zero Extend"
; Reads 1 byte from address: R8 + RAX + 0x752038
; R8 = 0x140000000 (base), RAX = item_id
; So reads from 0x140752038 + item_id
; EAX now contains 0x00 or 0x01

mov ecx, [r8+rax*4+00752030]
; Reads from a second table at 0x752030
; Uses byte value (0 or 1) as index
; Entry 0 = offset to SKIP code (751CC2)
; Entry 1 = offset to CONTINUE code (751CC4)

add rcx, r8              ; RCX = offset + base address
jmp rcx                  ; Jump to calculated address
                         ; If byte was 0x00: jumps to 751CC2 (SKIP)
                         ; If byte was 0x01: jumps to 751CC4 (CONTINUE)
```

### Phase 5A: Skip Path (751CC2)

```asm
xor cl, cl               ; CL = 0 (clear register)
                         ; XOR a register with itself = 0
                         ; Marks item as "skipped"
```

### Phase 5B: Continue Path - Level 2 Check (751CC4-751CCF)

```asm
movzx r12d, cl           ; R12D = CL (save "enabled" flag)

xor r12b, 01             ; Toggle lowest bit of R12
                         ; If R12B was 1, now 0. If 0, now 1.

cmp ebp, 14              ; Compare item index with 0x14 (20)
ja 751F5A                ; If EBP > 0x14, jump to ownership gate
                         ; NOTE: All problematic items have index <= 0x14
                         ; Fashion Collection: EBP = 8 (no jump)
                         ; 8700K: EBP = 17 (no jump)
                         ; Alien Shield: EBP = 20 (equal, no jump)
```

### Phase 6: Jump Table Dispatch (751CD5-751CE3)

```asm
movsxd rax, ebp          ; RAX = EBP (sign-extended to 64-bit)

mov ecx, [r8+rax*4+0075206C]
; Read from jump table at 0x14075206C
; Each entry is 4 bytes (rax*4)
; Index 0: 0x14075206C + 0  = 0x14075206C
; Index 8: 0x14075206C + 32 = 0x14075208C (Fashion Collection)
; Index 17: 0x14075206C + 68 = 0x1407520B0 (8700K)

add rcx, r8              ; RCX = offset + 0x140000000
jmp rcx                  ; JUMP to item-specific handler!
```

**Jump Destinations**:
| Item | Jumps To | What Happens |
|------|----------|--------------|
| Blazefire Saber | 751F45 | Ownership check called, returns 1 |
| Fashion Collection | 751F5C | SKIPS directly to JE (DL=0, fails!) |
| 8700K | 751D1D | Ownership check called, returns 0 |

### Phase 7A: Working Item Path (751F45)

```asm
mov edx, 000EEACA        ; EDX = ownership check parameter (product ID)
mov rcx, [rax+30]        ; Get object pointer
mov rax, [rcx]           ; Get vtable
call qword ptr [rax+38]  ; OWNERSHIP CHECK - returns AL (0 or 1)
                         ; For Blazefire, returns 1 (treated as "owned")

xor r12b, r12b           ; Clear r12b
movzx edx, al            ; DL = ownership result
                         ; DL = 1 means "owned"
; Falls through to ownership gate at 751F5A
```

### Phase 7B: Fashion Collection Path (751F5C)

```asm
; Fashion Collection SKIPS everything above and lands directly here!
; DL was never set to 1, so it remains 0

je 751FAD                ; DL = 0, Zero Flag set, JUMPS to skip!
```

### Phase 7C: 8700K Path (751D1D)

```asm
lea rcx, [ffxv_s.exe+44B7B90]
call qword ptr [ffxv_s.exe+3067E68]  ; Some initialization

mov rcx, [rax+30]        ; Get object pointer
mov rax, [rcx]           ; Get vtable
mov edx, 000C049A        ; Different ownership ID than Blazefire
call qword ptr [rax+38]  ; OWNERSHIP CHECK - returns 0 (not owned)

jmp 751F57               ; Jump to DL setup
                         ; movzx edx, al → DL = 0
; Falls through to ownership gate at 751F5A with DL=0
```

### Phase 8: The Ownership Gate (751F5A-751F5C)

```asm
test dl, dl              ; Test if DL is zero
                         ; Sets Zero Flag (ZF) if DL = 0
                         ; TEST performs AND: DL AND DL = DL

je 751FAD                ; JE = "Jump if Equal" = "Jump if Zero Flag set"
                         ; If DL = 0: ZF set → JUMP to 751FAD (skip!)
                         ; If DL = 1: ZF not set → Fall through (unlock!)

; BYTES: 74 4F
;        │  └─► 4F = offset (79 bytes forward to 751FAD)
;        └─► 74 = opcode for JE
```

**Why patching this affects ALL items**: ALL items converge at this instruction. NOPing it (`90 90`) lets every item fall through to unlock code.

### Phase 9: Unlock Code (751F60-751F9E)

```asm
; 751F5E
jmp 751F6D               ; Jump to unlock preparation (if DL=1)

; 751F60
mov rsi, [rsp+000000A0]  ; RSI = object pointer from stack
mov ebx, 00000001        ; EBX = 1

; 751F6D
test r12b, r12b          ; Check R12B flag
je 751F95                ; If R12B = 0, skip to unlock call setup

; 751F95
lea edx, [rbp+20]        ; EDX = item_id + 1
mov r8b, 01              ; R8B = 1 (parameter)
mov rcx, rsi             ; RCX = object pointer

; 751F9E - THE UNLOCK!
call ffxv_s.exe+7A5790   ; ITEM GRANTED HERE!
                         ; Parameters: RCX=object, EDX=item_id+1, R8B=1

; 751FA3
jmp 751FAD               ; Jump to loop end
```

### Phase 10: Loop End (751FAD)

```asm
inc ebp                  ; EBP = EBP + 1 (next item index)
; ... loop continuation ...
cmp eax, 36              ; Compare with max
jl 751CA0                ; If not done, repeat loop
```

---

## Why Some Items Don't Unlock

### Summary

| Item Category | Problem | DL Value at Gate | Result |
|---------------|---------|------------------|--------|
| Working items (Blazefire, etc.) | None | DL = 1 | ✓ Unlocks |
| Fashion Collection | Skips ownership check entirely | DL = 0 | ✗ Fails |
| 8700K | Ownership check returns 0 | DL = 0 | ✗ Fails |
| Half-Life items | Similar to above | DL = 0 | ✗ Fails |

### Detailed Breakdown

**WORKING ITEMS (Blazefire, Twitch Prime, etc.)**:
1. Jump table sends them to 751F45
2. Ownership check is called
3. Check returns AL=1 (owned)
4. DL=1 at the gate
5. Gate passed → **UNLOCKED!**

**FASHION COLLECTION (0x27)**:
1. Jump table sends it to 751F5C (DIRECTLY to the gate!)
2. SKIPS the ownership check entirely
3. DL=0 (was never set)
4. Gate fails → **NOT UNLOCKED**

**8700K ACCESSORIES (0x30)**:
1. Jump table sends it to 751D1D
2. Ownership check IS called (with ID 0x000C049A)
3. Check returns AL=0 (Intel promo not owned)
4. DL=0 at the gate
5. Gate fails → **NOT UNLOCKED**

**HALF-LIFE ITEMS (0x29, 0x2C, 0x2D, 0x2E, 0x2F)**:
- Jump to different addresses (751FBF, 7520FF, etc.)
- May have different check mechanism (CF flag mentioned in notes)
- All ultimately fail at the ownership gate with DL=0

---

## Anti-Tamper Protection

### Key Discovery

**FFXV has anti-tamper protection** that detects any modification to executable code and crashes the game.

### Evidence

1. Single-byte code change (`74 4F` → `74 4E`) = **CRASH**
2. Code cave injection = **CRASH**
3. Any `jmp` patch to code section = **CRASH**
4. Data modifications (byte table, jump table) = **Works, no crash**

### Detection Method (Suspected)

FFXV likely uses one of these methods:
- **Code checksum verification**: Periodically hashes code sections and compares to expected values
- **Hardware breakpoint detection**: Detects debugging attempts
- **Inline integrity checks**: Functions verify their own code before executing

### What This Means

| Modification Type | Example | Result |
|-------------------|---------|--------|
| Data (byte table) | `140752038 = 01` | ✓ Works |
| Data (jump table) | `14075208C = 7675616` | ✓ Works (no crash, may not unlock) |
| Code (any change) | `751F5C: 74→90` | ✗ CRASH |
| Code (code cave) | `jmp` to allocated memory | ✗ CRASH |

### Why Individual Selection is Impossible

To unlock Fashion Collection (0x27) or 8700K (0x30) individually, we would need to:
1. Inject code that checks the current item (RBP register)
2. If it's an item we want, set `DL=1`
3. Otherwise, leave `DL=0`

**This requires code modification, which triggers the anti-tamper and crashes.**

The only options are:
1. **All or nothing** - NOP the ownership gate (Unlock 3)
2. **Use Unlock 1+2** - Different bypass, unlocks everything

---

## Patch Byte Reference

### Unlock 1 - Bounds Bypass

| Property | Value |
|----------|-------|
| **Address** | `ffxv_s.exe+751CA8` |
| **Original** | `77 1A` (ja +1A) |
| **Patched** | `EB 1A` (jmp +1A) |
| **Effect** | Forces all items through unlock path |

### Unlock 2 - Steam Bypass

| Property | Value |
|----------|-------|
| **Address** | `ffxv_s.exe+751CC8` |
| **Original** | `41 80 F4 01 83 FD 14` (xor r12b,01; cmp ebp,14) |
| **Patched** | `4D 31 E4 41 38 D4 F8` (xor r12,r12; cmp r12b,dl; clc) |
| **Effect** | Clears flags for Steam/Promo items |
| **Requires** | Unlock 1 must be enabled first or game crashes |

### Unlock 3 - DL Bypass (Ownership Gate)

| Property | Value |
|----------|-------|
| **Address** | `ffxv_s.exe+751F5C` |
| **Original** | `74 4F` (je +4F) |
| **Patched** | `90 90` (nop nop) |
| **Effect** | Removes DL=0 skip, all items that reach gate unlock |
| **Note** | Due to anti-tamper, may only work via Cheat Engine with bypass |

---

## Register Reference

### Register Sizes

| 64-bit | 32-bit | 16-bit | 8-bit (low) |
|--------|--------|--------|-------------|
| RAX | EAX | AX | AL |
| RBX | EBX | BX | BL |
| RCX | ECX | CX | CL |
| RDX | EDX | DX | DL |
| RBP | EBP | BP | - |
| R8 | R8D | R8W | R8B |
| R12 | R12D | R12W | R12B |

### Register Usage in Unlock Loop

| Register | Purpose |
|----------|---------|
| **RBP/EBP** | Loop counter (item index). Starts at -32, increments each iteration. Item ID = EBP + 0x1F |
| **RAX/EAX** | Item ID (after `lea eax, [rbp+1F]`). Also used for byte table lookup and function return values |
| **AL** | Ownership check return value (0=not owned, 1=owned) |
| **RDX/EDX** | Ownership check parameter (product/DLC ID). Examples: 0x000EEACA (Blazefire), 0x000C049A (8700K) |
| **DL** | **THE CRITICAL OWNERSHIP FLAG**. 0 = gate fails, 1 = gate passes |
| **RCX/ECX** | Jump target calculation. Also first parameter for function calls |
| **R8** | Base address: 0x140000000. Added to offsets for full addresses |
| **R12/R12B** | Flag used in unlock logic. Toggled with `xor r12b, 01`. Checked at 751F6D |
| **RSI** | Object pointer for unlock function call. Set at 751F60 |
| **RBX/EBX** | Set to 1 at 751F68. Used in unlock logic |

### CPU Flags

| Flag | Description |
|------|-------------|
| **Zero Flag (ZF)** | Set when result is zero. `test dl, dl` sets ZF=1 if DL=0. JE checks this flag. |
| **Carry Flag (CF)** | Set on unsigned overflow. JA checks both ZF=0 AND CF=0. May be relevant for Half-Life items. |

### Register State at Key Points

**At Jump Table Dispatch (751CE3)**:
- **R8** = `0x140000000` (base address)
- **RAX** = item index (same as EBP)
- **RBP** = item index (`item_id - 0x1F`)
- **RCX** = jump destination

**At Ownership Gate (751F5A)**:
- **RBP** = item index
- **DL** = ownership result (0=not owned, 1=owned)
- **R12B** = flag used later in unlock path

---

## Cheat Engine Scripts

### Enable Single Item (Example: Blazefire Saber)

```asm
[ENABLE]
140752038:
  db 01

[DISABLE]
140752038:
  db 00
```

### Enable All Normally Unavailable Items

```asm
[ENABLE]
140752038:
  db 01  // Blazefire Saber
14075204B:
  db 01  // Noodle Helmet
14075204F:
  db 01  // King's Knight Sticker Set
140752053:
  db 01  // Kingglaives Pack (COMRADES)
140752054:
  db 01  // Party Pack (COMRADES)
140752055:
  db 01  // Memories of KING'S KNIGHT
140752056:
  db 01  // King's Knight Tee

[DISABLE]
140752038:
  db 00
14075204B:
  db 00
14075204F:
  db 00
140752053:
  db 00
140752054:
  db 00
140752055:
  db 00
140752056:
  db 00
```

### Enable All Twitch Prime Drops

```asm
[ENABLE]
140752058:
  db 01  // Weatherworn Regalia Decal
140752059:
  db 01  // Kooky Chocobo Tee (COMRADES)
14075205A:
  db 01  // Kooky Chocobo
14075205B:
  db 01  // 10,000 GIL
14075205C:
  db 01  // 100 AP
14075205D:
  db 01  // 16 Rare Coins

[DISABLE]
140752058:
  db 00
140752059:
  db 00
14075205A:
  db 00
14075205B:
  db 00
14075205C:
  db 00
14075205D:
  db 00
```

### Enable All Steam Exclusives (Byte Table Only - Won't Work!)

```asm
// NOTE: These items require Unlock 1+2 or Unlock 3 to actually unlock
[ENABLE]
14075205F:
  db 01  // FFXV Fashion Collection
140752061:
  db 01  // HEV Suit (COMRADES)
140752064:
  db 01  // Scientist Glasses (COMRADES)
140752065:
  db 01  // Crowbar (COMRADES)
140752066:
  db 01  // Half-Life Costume
140752067:
  db 01  // Crowbar

[DISABLE]
14075205F:
  db 00
140752061:
  db 00
140752064:
  db 00
140752065:
  db 00
140752066:
  db 00
140752067:
  db 00
```

### Enable All Origin Exclusives

```asm
[ENABLE]
140752060:
  db 01  // FFXV Decal Selection
140752062:
  db 01  // FINAL_FANTASY_XV_THE_SIMS_4_PACK

[DISABLE]
140752060:
  db 00
140752062:
  db 00
```

### Enable Microsoft Store Exclusive

```asm
[ENABLE]
14075205E:
  db 01  // FFXV Powerup Pack

[DISABLE]
14075205E:
  db 00
```

### Enable Promotional Items (Byte Table Only - Won't Work!)

```asm
// NOTE: These items require Unlock 1+2 or Unlock 3 to actually unlock
[ENABLE]
140752068:
  db 01  // 8700K Accessories
14075206B:
  db 01  // Alien Shield

[DISABLE]
140752068:
  db 00
14075206B:
  db 00
```

### Unlock 1 - Bounds Bypass

```asm
[ENABLE]
aobscanmodule(aob_check1,ffxv_s.exe,83 F8 33 77 1A)
registersymbol(aob_check1)
aob_check1+3:
  db EB 1A  // ja -> jmp (always jump)

[DISABLE]
aob_check1+3:
  db 77 1A
unregistersymbol(aob_check1)
```

### Unlock 2 - Steam Bypass (REQUIRES Unlock 1!)

```asm
[ENABLE]
aobscanmodule(aob_check2,ffxv_s.exe,41 80 F4 01 83 FD 14)
registersymbol(aob_check2)
aob_check2:
  xor r12,r12
  cmp r12l,dl
  clc

[DISABLE]
aob_check2:
  db 41 80 F4 01 83 FD 14
unregistersymbol(aob_check2)
```

### Unlock 3 - DL Bypass

```asm
[ENABLE]
aobscanmodule(aob_check3,ffxv_s.exe,84 D2 74 4F EB 0D)
registersymbol(aob_check3)
aob_check3+2:
  nop 2  // Remove je instruction

[DISABLE]
aob_check3+2:
  db 74 4F
unregistersymbol(aob_check3)
```

### UNLOCK ALL (Everything)

```asm
[ENABLE]
// ==========================================
// FFXV EXCLUSIVE UNLOCKS - ALL ITEMS
// ==========================================

// Normally Unavailable
140752038:
  db 01  // Blazefire Saber
14075204B:
  db 01  // Noodle Helmet
14075204F:
  db 01  // King's Knight Sticker Set
140752053:
  db 01  // Kingglaives Pack (COMRADES)
140752054:
  db 01  // Party Pack (COMRADES)
140752055:
  db 01  // Memories of KING'S KNIGHT
140752056:
  db 01  // King's Knight Tee

// Twitch Prime Drops
140752058:
  db 01  // Weatherworn Regalia Decal
140752059:
  db 01  // Kooky Chocobo Tee (COMRADES)
14075205A:
  db 01  // Kooky Chocobo
14075205B:
  db 01  // 10,000 GIL
14075205C:
  db 01  // 100 AP
14075205D:
  db 01  // 16 Rare Coins

// Microsoft Store
14075205E:
  db 01  // FFXV Powerup Pack

// Steam Exclusives (require Unlock 1+2 or Unlock 3)
14075205F:
  db 01  // FFXV Fashion Collection
140752061:
  db 01  // HEV Suit (COMRADES)
140752064:
  db 01  // Scientist Glasses (COMRADES)
140752065:
  db 01  // Crowbar (COMRADES)
140752066:
  db 01  // Half-Life Costume
140752067:
  db 01  // Crowbar

// Origin Exclusives
140752060:
  db 01  // FFXV Decal Selection
140752062:
  db 01  // FINAL_FANTASY_XV_THE_SIMS_4_PACK

// Promotional (require Unlock 1+2 or Unlock 3)
140752068:
  db 01  // 8700K Accessories
14075206B:
  db 01  // Alien Shield

[DISABLE]
// Normally Unavailable
140752038:
  db 00
14075204B:
  db 00
14075204F:
  db 00
140752053:
  db 00
140752054:
  db 00
140752055:
  db 00
140752056:
  db 00

// Twitch Prime Drops
140752058:
  db 00
140752059:
  db 00
14075205A:
  db 00
14075205B:
  db 00
14075205C:
  db 00
14075205D:
  db 00

// Microsoft Store
14075205E:
  db 00

// Steam Exclusives
14075205F:
  db 00
140752061:
  db 00
140752064:
  db 00
140752065:
  db 00
140752066:
  db 00
140752067:
  db 00

// Origin Exclusives
140752060:
  db 00
140752062:
  db 00

// Promotional
140752068:
  db 00
14075206B:
  db 00
```

---

## Attempted Solutions Log

During reverse engineering, the following approaches were tested:

| Approach | Result | Reason |
|----------|--------|--------|
| Set byte to 0x01 only | Partial success | Works for some items, problematic items have different jump destinations |
| NOP `je` at 751F5C | Unlocks everything | Shared code path - cannot select individual items |
| `mov dl, 1` at 751F5C | Same as NOP | Shared code path |
| Change jump table to 751F45 | Crash/black screen | Ownership call needs specific register setup |
| Change jump table to 751F6D | Crash | Missing RSI setup |
| Change jump table to 751F60 | No crash, no unlock | R12B check fails |
| Change jump table to 751F95 | Crash | Missing RSI setup |
| Code cave injection | Crash | Anti-tamper detected code modification |
| Single byte code change (74→4E) | Crash | Anti-tamper detected code modification |

---

## Unexplored Areas

### Half-Life Items

Items 0x29, 0x2C, 0x2D, 0x2E, 0x2F haven't been fully traced:
- Notes mentioned "CF needs to be cleared"
- May have a different code path than Fashion Collection / 8700K
- Jump destinations: 751FBF, 7520FF, etc.

### Why 751F60 Doesn't Trigger Unlock

- Entry point 751F60 is stable (no crash when jump table modified)
- But doesn't actually unlock items
- Need to trace what happens after - does it fail at `test r12b, r12b`?
- R12B might need to be set to 1

### Ownership Call Parameters

- Blazefire: `0x000EEACA`
- 8700K: `0x000C049A`
- These are likely DLC/product IDs
- Could there be a parameter that always returns "owned"?

---

## Key Addresses Summary

| Address | Description |
|---------|-------------|
| `0x140752038` | Byte Table Base |
| `0x14075206C` | Jump Table Base |
| `0x140751C8B` | Loop Start |
| `0x140751CA5` | Bounds Check |
| `0x140751CAC` | Byte Table Lookup |
| `0x140751CC8` | Level 2 Check |
| `0x140751CD5` | Jump Table Dispatch |
| `0x140751F45` | Working Items Handler |
| `0x140751D1D` | 8700K Handler |
| `0x140751F5A` | Ownership Gate (test dl, dl) |
| `0x140751F5C` | The Gate (je 751FAD) |
| `0x140751F9E` | Unlock Function Call |
| `0x1407A5790` | Actual Unlock Function |
| `0x140751FAD` | Loop End |

---

## Version Information

- **Game**: Final Fantasy XV Windows Edition (Steam)
- **Executable**: ffxv_s.exe
- **Research Date**: 2025
- **Base Address**: 0x140000000 (typical for 64-bit executables)

---

*This document consolidates all reverse engineering research for the FFXV Windows Edition exclusive item unlock system.*
