@echo off
setlocal
if not exist src\tinyscript.y (
  echo Error: src\tinyscript.y not found.
  exit /b 1
)
if not exist src\tinyscript.l (
  echo Error: src\tinyscript.l not found.
  exit /b 1
)
where flex >nul 2>&1
if errorlevel 1 (
  echo Error: flex not found. Install Flex and Bison or use WSL.
  exit /b 1
)
where bison >nul 2>&1
if errorlevel 1 (
  echo Error: bison not found. Install Flex and Bison or use WSL.
  exit /b 1
)
where gcc >nul 2>&1
if errorlevel 1 (
  echo Error: gcc not found. Install a C compiler or use WSL.
  exit /b 1
)

bison -d -o src\parser.c src\tinyscript.y
if errorlevel 1 goto error
flex -o src\lexer.c src\tinyscript.l
if errorlevel 1 goto error

gcc -std=c99 -Wall -Wextra -O2 -o tinyscript.exe src\parser.c src\lexer.c src\ast.c src\ir.c src\runtime.c src\main.c
if errorlevel 1 goto error

echo Build succeeded.
exit /b 0
:error
echo Build failed.
exit /b 1
endlocal
