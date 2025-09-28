# Enter repo directory.
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1
echo "script args: $@"

cmake -S . -B build --fresh -DLK_TEST_DRAW_TRIANGLE_SHADER=1 -DLK_TEST_DRAW_TRIANGLE_SHADER_CONFIGURABLE=1
cmake --build build -j 8

echo ""
echo "Running test"
./build/x64-Debug/test/draw_triangle_shader_configurable.exe
