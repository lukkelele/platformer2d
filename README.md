# platformer2d

A 2D game written in C++.

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
```
git clone https://github.com/microsoft/vcpkg.git

#---------------------------------------------------
# Run the bootstrap script
#---------------------------------------------------
# Linux
cd vcpkg; .\bootstrap-vcpkg.sh -disableMetrics

# Windows
cd vcpkg; .\bootstrap-vcpkg.bat -disableMetrics

#---------------------------------------------------
# Configure the VCPKG_ROOT environment variable
#---------------------------------------------------
# Linux
export VCPKG_ROOT="/path/to/vcpkg"
export PATH="${PATH}:/path/to/vcpkg"

# Windows (Powershell)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
```

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
