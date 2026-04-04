# Testing TinyScript

This folder contains validation notes and sample test cases for the TinyScript compiler.

## How to Run

1. Build the compiler:
   - Windows: `build.bat`
   - Unix: `make`
2. Execute a sample program:
   - `./tinyscript examples/thermostat.ts --run`
3. Inspect the generated IR in `program.tsb`

## Notes

If the environment does not have `flex`, `bison`, or `gcc`, install those tools or use WSL/Mingw.
