# M65832 Stdlib Smoke Tests

This folder contains lightweight smoke tests for the M65832 stdlib
build used in day-to-day development.

## Run Smoke Tests

From the repo root:

```
./m65832-stdlib/test/run_tests.sh
```

Filter tests by glob:

```
./m65832-stdlib/test/run_tests.sh test_string*
./m65832-stdlib/test/run_tests.sh test_stdlib.c
```

List available tests:

```
./m65832-stdlib/test/run_tests.sh --list
```

## Full Picolibc Test Suite

The full picolibc test suite is external and is run via Meson using the
M65832 emulator wrapper:

```
./m65832-stdlib/picolibc/run_tests.sh
```

See `m65832-stdlib/picolibc/README.md` for details.
