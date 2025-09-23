# Enter repo directory.
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1
echo "args: $@"

cmake -S . -B build --fresh -DTEST_DRAW_TRIANGLE_WITH_SHADER=1
cmake --build build -j 8
