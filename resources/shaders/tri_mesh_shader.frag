@include "common/header.glsl"

//layout (early_fragment_tests) in;

@include "common/gua_fragment_shader_input.glsl"
@include "common/gua_camera_uniforms.glsl"

uniform float gua_texel_width;
uniform float gua_texel_height;

vec2 gua_get_quad_coords() {
  return vec2(gl_FragCoord.x * gua_texel_width, gl_FragCoord.y * gua_texel_height);
}

@material_uniforms@

@include "common/gua_fragment_shader_output.glsl"
@include "common/gua_global_variable_declaration.glsl"

@include "common/gua_abuffer_collect.glsl"

@material_method_declarations_frag@

#if @enable_virtual_texturing@

@include "common/virtual_texturing_functions.frag"
#endif

void main() {

  @material_input@
  @include "common/gua_global_variable_assignment.glsl"

  #if @enable_virtual_texturing@
    gua_color = vec3(0.0, 1.0, 0.0);
    gua_uvs.w = gua_current_vt_idx;
  #else
    gua_emissivity = 1.0;
    gua_metalness = 0.0;
    gua_color = vec3(1.0, 0.0, 0.0);
    
    gua_uvs.z = 0.0;
    gua_uvs.w = 0;
  #endif

    //gua_color = vec3(1.0, 1.0, 1.0);
  // normal mode or high fidelity shadows

  if (gua_rendering_mode != 1) {
    @material_method_calls_frag@
  }

  #if @enable_virtual_texturing@
    writeVTCoords( int(gua_uvs.w) );
  #else
  #endif

  submit_fragment(gl_FragCoord.z);
}
