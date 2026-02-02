# M65832 Picolibc Integration

This folder contains the M65832-specific integration pieces for the
`picolibc-m65832` fork (cross file, emulator wrapper, syscalls, crt0,
and helper scripts).

## Test Suite Location

The full picolibc test suite lives in the external fork:

- `/Users/benjamincooley/projects/picolibc-m65832`

We keep a dedicated build directory for tests:

- `/Users/benjamincooley/projects/picolibc-build-m65832-tests`

## Running the Picolibc Tests

From the repo root:

```
./m65832-stdlib/picolibc/run_tests.sh
```

Filter tests or list them (passed to `meson test`):

```
./m65832-stdlib/picolibc/run_tests.sh --list
./m65832-stdlib/picolibc/run_tests.sh string
```

This script configures Meson with the M65832 cross file and runs the test
suite using the emulator wrapper in `run-m65832.sh`.

## Install-Only Build

To build and install picolibc without tests:

```
./m65832-stdlib/scripts/build_picolibc.sh
```

## Notes

- The emulator wrapper path is hard-coded in `run-m65832.sh`.
- If Meson reports link alias support issues, see `picolibc/meson.build`
  around the printf alias checks.
