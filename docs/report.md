# TinyScript: A Lightweight DSL Compiler for IoT Control Logic

## Abstract

TinyScript is a compact domain-specific language and compiler designed to express IoT automation concisely while retaining compiler design pedagogy. The system employs lex/yacc and C to build a complete end-to-end compiler with lexical analysis, syntax analysis, semantic checking, IR generation, optimization, and simulation execution.

## Introduction

Embedded and IoT systems often require simple automation logic for sensors and actuators. General-purpose languages are too heavy for non-expert users. TinyScript introduces a readable DSL that can describe device control, event triggers, scheduling, and sensor-driven logic with minimal syntax.

## Literature Survey

DSLs are used widely in IoT platforms like Node-RED, Arduino scripts, and cloud rule engines. Node-RED uses visual flow-based programming, while Arduino sketches use C/C++ boilerplate. TinyScript sits between these models by providing a textual, compact language with strong compiler semantics.

### Comparison

- Node-RED: visual wiring, ideal for non-programmers, but less portable and harder to version control.
- Arduino scripts: powerful and low-level, but verbose for simple automation.
- TinyScript: textual DSL with explicit grammar, optimized compilation, and runtime simulation.

## Proposed System

TinyScript compiler is structured into:

- Lexical Analysis: tokenize keywords, identifiers, numbers, time literals, strings, and symbols.
- Syntax Analysis: parse TinyScript grammar into an AST using Bison.
- Semantic Analysis: validate device declarations, variable use, and sensor names.
- Intermediate Representation: convert AST into IR nodes suitable for optimization.
- Optimization: constant folding and dead code elimination.
- Target Code Generation: emit a serialized IR file and execute it through a virtual runtime.

## Implementation

The implementation uses standard C with Flex and Bison.

### DSL Syntax

TinyScript supports:

- `DEVICE <name>`
- `<device> ON` / `<device> OFF`
- `IF <expr> THEN ... ELSE ... END`
- `LOOP <count> TIMES DO ... END`
- `ON <expr> THEN ... END`
- `SCHEDULE AT HH:MM DO ... END`
- `LOG "message"`
- arithmetic and comparisons

### Example

```
DEVICE fan
IF temperature > 30 THEN fan ON ELSE fan OFF END
```

### Compiler Phases

- `src/tinyscript.l` tokenizes input.
- `src/tinyscript.y` produces an AST.
- `src/ir.c` converts AST to IR and applies optimizations.
- `src/runtime.c` simulates device state and sensor values.

## Results & Discussion

TinyScript executes DSL programs in a simulated 24-hour environment. The runtime supports events and schedules with multiple devices.

### Sample Output

- `fan ON` when temperature exceeds 30°C
- periodic motion-driven light control
- schedule-based device activation at fixed times

### Performance

The system is lightweight: the compiler performs linear-time parsing and simple AST/IR traversals. Optimization is constant-time for static branches.

### Accuracy

Semantic analysis verifies device declarations and expression correctness, reducing runtime errors.

## Conclusion

TinyScript demonstrates how a small compiler for IoT automation can be built with lex/yacc and C, while still supporting advanced features like scheduling, event-driven execution, optimization, and extensibility.

## References

- Aho, Sethi, Ullman. "Compilers: Principles, Techniques, and Tools." 2nd ed.
- Fowler, M. "Domain-Specific Languages." Addison-Wesley, 2010.
- Node-RED documentation and Arduino language model.
