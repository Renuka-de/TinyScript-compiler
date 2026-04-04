CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99 -Wall -Wextra -O2
SRC = src/ast.c src/ir.c src/runtime.c src/main.c src/parser.c src/lexer.c
TARGET = tinyscript

all: $(TARGET)

src/parser.c src/parser.h: src/tinyscript.y
	$(BISON) -d -o src/parser.c src/tinyscript.y

src/lexer.c: src/tinyscript.l src/parser.h
	$(FLEX) -o src/lexer.c src/tinyscript.l

$(TARGET): src/lexer.c src/parser.c src/ast.c src/ir.c src/runtime.c src/main.c
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	if exist $(TARGET) del /f /q $(TARGET)
	if exist src\lexer.c del /f /q src\lexer.c
	if exist src\parser.c del /f /q src\parser.c
	if exist src\parser.h del /f /q src\parser.h
