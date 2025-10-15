#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1

python -m pip install glad --break-system-packages

set -- \
  GL_ARB_bindless_texture \
  GL_ARB_texture_storage \
  GL_ARB_texture_buffer_object \
  GL_ARB_depth_texture \
  GL_ARB_draw_instanced \
  GL_ARB_draw_buffers \
  GL_ARB_texture_cube_map \
  GL_ARB_texture_cube_map_array \
  GL_ARB_texture_multisample \
  GL_ARB_instanced_arrays \
  GL_ARB_sample_shading \
  GL_ARB_debug_output \
  GL_ARB_timer_query \
  GL_ARB_direct_state_access

extensions=$(printf '%s,' "$@")
extensions=${extensions%,} # Drop trailing comma.

python -m glad \
  --profile=core \
  --api=gl=4.6 \
  --generator=c \
  --out-path=modules/glad \
  --extensions="${extensions}"