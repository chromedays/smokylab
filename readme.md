<!-- ![](/screenshots/s1.png) -->

# Smokylab
C/C++ Game Engine

## Requirements
- Windows 10
- clang or Visual Studio 2019 with clang toolchains
- CMake (> 3.14)
## How to build
```shell
python3 scripts/build.py --clear
python3 scripts/build.py --configure
python3 scripts/build.py --build-test
```

## Compilation database (compile_commands.json) for vscode
```shell
python3 scripts/build.py --compdb
```

