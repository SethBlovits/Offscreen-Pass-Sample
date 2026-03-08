#ifndef offscreen_pass_hlsl_h
#define offscreen_pass_hlsl_h

#if !defined(SLUGS_GRAPHICS_H)
#error "Please include slugs_graphics.h before including this file"
#endif
#define HLSL_SHADER_SOURCE_OFFSCREEN_PASS "offscreen_pass.hlsl"
#define VERTEX_SHADER_SOURCE_OFFSCREEN_PASS "offscreen_pass_vs.cso" 
#define FRAGMENT_SHADER_SOURCE_OFFSCREEN_PASS "offscreen_pass_ps.cso" 
#define BINDSLOT_OFFSCREEN_PASS_MVPConstantBuffer 0
#define BINDSLOT_OFFSCREEN_PASS_g_sampler 0
#define BINDSLOT_OFFSCREEN_PASS_albedo 0

#define OFFSCREEN_PASS_SHADER_DESC (slg_shader_desc){\
.filename = "offscreen_pass.hlsl",\
.vert_shader_target = "vs_5_0",\
.vert_shader_name = "offscreen_pass_vs.cso",\
.frag_shader_target = "ps_5_0",\
.frag_shader_name = "offscreen_pass_ps.cso"\
}

typedef struct OFFSCREEN_PASS_HLSL_UNIFORMS{
	slg_buffer MVPConstantBuffer;
	slg_texture albedo;
}OFFSCREEN_PASS_HLSL_UNIFORMS;

slg_uniforms OFFSCREEN_PASS_HLSL_MAKE_UNIFORMS(OFFSCREEN_PASS_HLSL_UNIFORMS uniform_desc){
	slg_uniforms out_uniforms = {0};
	out_uniforms.cbv_buffer[BINDSLOT_OFFSCREEN_PASS_MVPConstantBuffer] = uniform_desc.MVPConstantBuffer;
	out_uniforms.samplers[BINDSLOT_OFFSCREEN_PASS_g_sampler] = true;
	out_uniforms.srv_buffer[BINDSLOT_OFFSCREEN_PASS_albedo] = uniform_desc.albedo;
	return out_uniforms;
}
#endif