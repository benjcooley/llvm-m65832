## Project Notes (M65832 stdlib + emulator)

### Current status
- We shifted toward B-based frame addressing (locals via `B+$offset`) and A-destructive arithmetic sequences.
- After the change, core baremetal test suite now fails broadly (many FAIL), so we need a focused recovery path.
- `string_memcpy` in picolibc still fails (expected 10, got 0) and likely points to pointer/indirect addressing issues.

### Recent changes
- Added `SB` instruction forms (imm32 + dp) to set B in the backend.
- Frame base now uses B:
  - Prologue updates B to SP after stack allocation.
  - `getFrameRegister` returns B.
  - Frame index references now use B + offset.
- Loads/stores for locals now emit `LDA/STA B+$offset` in codegen.
- Arithmetic ops now use A-destructive sequences (LDA/ADC/SBC/STA) instead of extended ALU dp forms.
- Added fast Ninja build script: `build_llc_fast.sh` (builds clang in `build-fast`).

### Assembly verification
- `stdlib_atoi.c` now shows locals as `LDA B+$0008`, `STA B+$0004`, etc.
- This removes repeated SP/X address recomputation for locals.

### Build iteration
- New Ninja build directory: `llvm-m65832/build-fast`.
- Script: `llvm-m65832/build_llc_fast.sh`.

### Other work in parallel
- Separate LLM is working on a GPU project: proper GPU thread warp OoO execution, with a new system bus for CPU/GPU memory arbitration.
