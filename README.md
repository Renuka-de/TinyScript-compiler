# TinyScript: A Lightweight DSL Compiler for IoT Control Logic

TinyScript is a complete academic project that demonstrates a lex/yacc-based compiler pipeline for a small domain-specific language oriented around IoT automation.

## Project Structure

- `src/` - lex/yacc and C compiler/runtime implementation
- `docs/` - architecture documentation and IEEE-style report
- `examples/` - TinyScript sample programs
- `tests/` - test cases and execution notes
- `web/` - optional UI for interactive TinyScript compilation and simulation

## Features

- Custom TinyScript DSL for IoT devices
- Full compiler pipeline: lexer, parser, AST, semantic analysis, IR, optimizer, runtime
- Event-driven execution and schedule-based automation
- Optimizations: constant folding, dead code elimination
- Multi-device scheduling and simulated sensor environment
- Logging system for runtime diagnostics

## Getting Started

### Build (Windows)

```bat
build.bat
```

### Build (Unix-like)

```sh
make
```

### Run Example

```sh
./tinyscript examples/thermostat.ts --run
```

### Output

- Compiled IR text file: `program.tsb`
- Runtime simulation logs printed to console

## Recommended Files

- `docs/architecture.md` - system architecture and diagrams
- `docs/report.md` - IEEE-style academic report
- `examples/thermostat.ts` - temperature control use case
- `examples/event_control.ts` - event-driven automation
- `examples/schedule.ts` - scheduled device orchestration

## Notes

This project is intentionally designed for academic review and submission quality, with modular C source files, documentation, and clear architecture descriptions.
