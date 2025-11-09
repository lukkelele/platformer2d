# platformer2d

A 2D game written in C++.
<br>
<br>
I wanted to focus on learning more about rendering, physics simulation and CMake
and thus this project was born.

## Showcase

:warning: Barely any progress has been made yet, lots to come :warning:

![testlevel-1](doc/img/2025-11-09/testlevel-1.png)
![testlevel-gif](doc/gif/platformer2d-2025-11-09_1.gif)

###

 * application ([header](src/core/application.h)/[source](src/core/application.cpp))
 * renderer ([header](src/renderer/renderer.h)/[source](src/renderer/renderer.cpp))
 * player ([header](src/game/player.h)/[source](src/game/player.cpp))
 * actor ([header](src/scene/actor.h)/[source](src/scene/actor.cpp))
 * body ([header](src/physics/body.h)/[source](src/physics/body.cpp))

### CURRENT TEST USED FOR DEVELOPMENT
The project main.cpp is not used as of yet.
 * testlevel ([header](src/game/level/testlevel.h)/[source](src/game/level/testlevel.cpp))
 * [test/game/instance/test.cpp](test/game/instance/test.cpp)
 * [test/game/instance/main.cpp](test/game/instance/main.cpp)

## Todo
:black_square_button: GCC/clang support<br>
:black_square_button: Documented setup steps<br>
:black_square_button: Project description about design choices made in regards to assets and level data<br>
:black_square_button: Player texture sprite<br>
:black_square_button: github actions pipeline<br>
:black_square_button: I do NOT LIKE!!!!! the vcpkg dependency, might remove that<br>
:black_square_button: Projectile collision (for player weapon :boom:)<br>
:black_square_button: Movable enemies<br>
:black_square_button: Behaviour trees<br>
:black_square_button: Network replication (?)<br>
:black_square_button: freetype as dependency<br>
:black_square_button: Font rendering<br>
:black_square_button: tracy as dependency<br>
:black_square_button: Fix ANNOYING!!! auto include with CMake extension in Visual Studio<br>

<br>

## Setup
Clone the repo and setup all submodules.  
```
git clone --recursive https://github.com/lukkelele/platformer2d
```
Make sure to run `git submodule update --init --recursive` if the repo is cloned **without** the `--recursive` flag.

### Dependencies
The game is dependent on several external libraries.  
Most dependencies are submodules but some need manual installation.

#### glad
The files for glad need to be generated.  
Use the `build_glad.sh` script in the `scripts` directory or run the generation code.
```
# OpenGL 4.6
python -m pip install glad --break-system-packages
python -m glad --profile=core --api=gl=4.6 --generator=c --out-path=modules/glad
```

#### vcpkg
Other dependencies use vcpkg.  
[github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg)  

#### msdf-atlas-gen
Depends on vcpkg.
```
cd external/msdf-atlas-gen
cmake -S . -B build -DCMAKE_BUILD_TYPE=release
```

#### freetype
Install with vcpkg or download from https://freetype.org/download.html  
```
# Linux
vcpkg install freetype

# Windows
vcpkg.exe install freetype:x64-windows
```

