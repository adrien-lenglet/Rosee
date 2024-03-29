layout(set = 0, binding = 0) uniform Illum {
	mat4 cam_proj;
	mat4 view;
	mat4 view_normal;
	mat4 view_inv;
	mat4 view_normal_inv;
	mat4 last_view;
	mat4 last_view_inv;
	mat4 view_cur_to_last;
	mat4 view_last_to_cur;
	mat4 view_last_to_cur_normal;
	vec3 rnd_sun[256];
	vec3 rnd_diffuse[256];
	vec3 sun;
	vec2 size;
	vec2 size_inv;
	vec2 depth_size;
	vec2 depth_size_inv;
	vec2 cam_ratio;
	uvec2 probe_extent;
	uint probe_size_l2;
	uint probe_size;
	float cam_near;
	float cam_far;
	float cam_a;
	float cam_b;
} il;