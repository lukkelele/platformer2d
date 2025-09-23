# Enter repo directory.
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1
echo "script args: $@"

cmake -S . -B build --fresh -DTEST_DRAW_TRIANGLE_WITH_SHADER=1
cmake --build build -j 8

echo ""
echo "Running test"
./build/Debug/platformer2d.exe
