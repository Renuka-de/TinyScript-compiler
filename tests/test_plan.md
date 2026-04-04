# TinyScript Test Plan

## Goals

- Validate parsing of TinyScript DSL constructs
- Confirm semantic checks for devices, sensors, and variables
- Verify optimization of constant expressions and dead code removal
- Ensure runtime simulation executes event-driven and schedule-based behaviors

## Test Cases

1. `examples/thermostat.ts`
   - Should compile without errors
   - Should log fan and LED state changes based on simulated temperature

2. `examples/event_control.ts`
   - Should compile
   - Should trigger the siren and LED when simulated motion occurs

3. `examples/schedule.ts`
   - Should compile
   - Should execute schedule actions at the designated times

4. Error case: undefined device
   - `fan ON` without `DEVICE fan`
   - Compiler should report a semantic error

5. Error case: invalid expression
   - `IF temperature >> 30 THEN`
   - Parser should reject invalid operators

## Execution Notes

- Build using `make` or `build.bat`
- Run with `./tinyscript examples/thermostat.ts --run`
- Inspect generated `program.tsb` for IR output
