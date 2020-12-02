#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct Frame {
	vec3 color;
	vec2 pos;
	float size;
};

layout(set = 0, binding = 0) readonly buffer Dyn {
	Frame f[];
} d;

layout(location = 0) in vec2 in_pos;
layout(location = 0) out vec3 out_color;

void main(void)
{
	gl_Position = vec4(d.f[gl_InstanceIndex].pos, 0, 1.0);
	gl_PointSize = d.f[gl_InstanceIndex].size;
	out_color = d.f[gl_InstanceIndex].color;
}
