# TinyScript Architecture

## Overview

TinyScript is architected as a classic compiler pipeline tailored for IoT automation. The pipeline converts a human-readable TinyScript DSL into a runtime-ready intermediate form, then executes that form in a virtual IoT environment.

## Component Diagram

```
+----------------+      +-------------+      +-----------+      +--------------+      +-----------+
| User DSL Input | ---> | Lexical     | ---> | Syntax    | ---> | Semantic     | ---> | IR / Code |
| (TinyScript)   |      | Analyzer    |      | Analyzer  |      | Analyzer     |      | Generator |
+----------------+      +-------------+      +-----------+      +--------------+      +-----------+
                                                                                             |
                                                                                             v
                                                                                      +---------------+
                                                                                      | Execution /   |
                                                                                      | Simulation VM |
                                                                                      +---------------+
```

## Data Flow

1. `lexer` reads text and produces tokens.
2. `parser` applies grammar rules and builds an AST.
3. `semantic analyzer` checks devices, variables, sensors, and control flow.
4. `IR generator` converts the AST into an intermediate representation.
5. `optimizer` performs constant folding and dead code elimination.
6. `code generator` emits a target program file and/or executes it through the runtime.

## Component Responsibilities

- `src/tinyscript.l` - tokenizes TinyScript keywords, identifiers, numbers, time literals, and strings.
- `src/tinyscript.y` - parses grammar rules into a structured AST.
- `src/ast.h`, `src/ast.c` - defines AST nodes and expression structures.
- `src/ir.h`, `src/ir.c` - defines intermediate IR nodes, optimization passes, and IR serialization.
- `src/runtime.c` - simulates IoT devices, built-in sensors, event triggers, schedule management, and logging.
- `src/main.c` - orchestrates compilation, output, and optional execution.

## Deployment Flow

```
TinyScript text -> Compiler -> program.tsb -> Runtime Simulation
```

## Execution Layer

The runtime layer supports:

- Device ON/OFF actions
- Event-driven triggers using `ON condition THEN ... END`
- Time-based scheduling using `SCHEDULE AT HH:MM DO ... END`
- Multiple devices and variable state
- Built-in sensor simulation for temperature, humidity, light, and motion

## Diagrams

### Block Diagram

```
[Input Editor]
      |
      v
[Lexer] -> [Parser] -> [AST]
             |         |
             v         v
       [Semantic Analyzer] -> [IR Generator] -> [Optimizer] -> [Target Code]
                                                          |
                                                          v
                                                    [Simulation VM]
```

### Data Flow Diagram

```
User Script
   |
   v
Tokens -> Parse Tree -> AST -> Semantic Model -> IR -> Optimized IR -> Executable Simulation
```

### Interaction Summary

- Users write TinyScript in a text editor.
- The compiler lexes and parses the DSL.
- Semantic analysis catches invalid device actions and misused variables.
- The optimizer simplifies static expressions and removes unreachable code.
- The runtime executes compiled behavior in a virtual IoT environment.
