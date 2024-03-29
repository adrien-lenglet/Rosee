struct RayPayload {
	bool hit;
	vec3 pos;
	vec3 albedo;
	vec3 normal;
	vec3 normal_geom;
};

struct Instance {
	mat3 mv_normal;
	uint model;
	uint material;
};

struct Vertex_pnu {
	vec3 p;
	vec3 n;
	vec2 u;
};

struct Vertex_pntbu {
	vec3 p;
	vec3 n;
	vec3 t;
	vec3 b;
	vec2 u;
};

struct Vertex_pn {
	vec3 p;
	vec3 n;
};

struct Material_albedo {
	uint albedo;
};

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_16bit_storage : enable

layout(constant_id = 0) const uint model_pool_size = 1;
layout(constant_id = 1) const uint samplers_pool_size = 1;

layout(set = 1, binding = 0) uniform sampler2D samplers[samplers_pool_size];

layout(scalar, set = 1, binding = 1) readonly buffer Instances {
	Instance instances[];
} instances;

layout(scalar, set = 1, binding = 2) readonly buffer Models_pnu {
	Vertex_pnu vertices[];
} models_pnu[model_pool_size];

layout(scalar, set = 1, binding = 3) readonly buffer Models_pn_i16_v {
	Vertex_pn vertices[];
} models_pn_i16_v[model_pool_size];

layout(scalar, set = 1, binding = 4) readonly buffer Models_pn_i16_i {
	uint16_t indices[];
} models_pn_i16_i[model_pool_size];

layout(scalar, set = 1, binding = 5) readonly buffer Models_pntbu {
	Vertex_pntbu vertices[];
} models_pntbu[model_pool_size];

layout(scalar, set = 1, binding = 6) readonly buffer Materials_albedo {
	Material_albedo materials[];
} materials_albedo;

Vertex_pnu vertex_read_pnu(uint model, uint primitive, vec2 bary)
{
	Vertex_pnu v0 = models_pnu[model].vertices[primitive * 3];
	Vertex_pnu v1 = models_pnu[model].vertices[primitive * 3 + 1];
	Vertex_pnu v2 = models_pnu[model].vertices[primitive * 3 + 2];
	float b0 = 1.0 - bary.x - bary.y;
	float b1 = bary.x;
	float b2 = bary.y;
	return Vertex_pnu(
		v0.p * b0 + v1.p * b1 + v2.p * b2,
		v0.n * b0 + v1.n * b1 + v2.n * b2,
		v0.u * b0 + v1.u * b1 + v2.u * b2);
}

Vertex_pn vertex_read_pn_i16(uint model, uint primitive, vec2 bary)
{
	Vertex_pn v0 = models_pn_i16_v[model].vertices[uint(models_pn_i16_i[model].indices[primitive * 3])];
	Vertex_pn v1 = models_pn_i16_v[model].vertices[uint(models_pn_i16_i[model].indices[primitive * 3 + 1])];
	Vertex_pn v2 = models_pn_i16_v[model].vertices[uint(models_pn_i16_i[model].indices[primitive * 3 + 2])];
	float b0 = 1.0 - bary.x - bary.y;
	float b1 = bary.x;
	float b2 = bary.y;
	return Vertex_pn(
		v0.p * b0 + v1.p * b1 + v2.p * b2,
		v0.n * b0 + v1.n * b1 + v2.n * b2);
}

Vertex_pntbu vertex_read_pntbu(uint model, uint primitive, vec2 bary)
{
	Vertex_pntbu v0 = models_pntbu[model].vertices[primitive * 3];
	Vertex_pntbu v1 = models_pntbu[model].vertices[primitive * 3 + 1];
	Vertex_pntbu v2 = models_pntbu[model].vertices[primitive * 3 + 2];
	float b0 = 1.0 - bary.x - bary.y;
	float b1 = bary.x;
	float b2 = bary.y;
	return Vertex_pntbu(
		v0.p * b0 + v1.p * b1 + v2.p * b2,
		v0.n * b0 + v1.n * b1 + v2.n * b2,
		v0.t * b0 + v1.t * b1 + v2.t * b2,
		v0.b * b0 + v1.b * b1 + v2.b * b2,
		v0.u * b0 + v1.u * b1 + v2.u * b2);
}