
#ifndef ABUF_SHADE_FUNC
#define ABUF_SHADE_FUNC abuf_get_color
vec4 abuf_get_color(uint pos, float depth) {
  return ABUF_FRAG(pos, 0);
}
#endif

void abuf_mix_frag(vec4 frag_color, inout vec4 color) {
  frag_color.rgb *= frag_color.a;
  color += frag_color * (1.0 - color.a);
}

bool abuf_blend(inout vec4 color, float opaque_depth) {
  const ivec2 frag_pos = ivec2(gl_FragCoord.xy);
  uint current = gua_resolution.x * frag_pos.y + frag_pos.x;
  int frag_count = 0;

  while (frag_count < ABUF_MAX_FRAGMENTS) {

    uvec2 frag = unpackUint2x32(frag_list[current]);
    current = frag.x;
    if (current == 0) {
      break;
    } 

    float z = UNLIN_DEPTH(UNPACK_DEPTH(frag.y)) * 2.0 - 1.0;
    if (z > opaque_depth) {
      break;
    }
    vec4 shaded_color = ABUF_SHADE_FUNC(current - abuf_list_offset, z);
    abuf_mix_frag(shaded_color, color);

    ++frag_count;

#if ABUF_CULL
    if (color.a >= ABUF_CULL_THRES)
      return false;
#endif
  }
  return true;
}