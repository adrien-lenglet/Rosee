#include <iostream>
#include "Rosee/Map.hpp"
#include "Rosee/Renderer.hpp"
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <ctime>
#include <cmath>
#include <glm/gtx/transform.hpp>
#include "Rosee/c.hpp"

#ifdef DEBUG
static inline constexpr bool is_debug = true;
#else
static inline constexpr bool is_debug = false;
#endif

using namespace Rosee;

class Noise
{
	ivec2 m_seed;
	std::minstd_rand m_gen;

	double zrand(void)
	{
		return static_cast<double>(m_gen()) / static_cast<double>(std::numeric_limits<decltype(m_gen())>::max());
	}

	double nrand(void)
	{
		return zrand() * 2.0 - 1.0;
	}

	glm::dvec2 gradient(const ivec2 &pos)
	{
		auto seq = std::seed_seq{pos.x, pos.y};
		m_gen.seed(seq);
		while (true) {
			auto c = glm::dvec2(nrand(), nrand());
			auto d = glm::dot(c, c);
			if (d > 0.0 && d <= 1.0)
				return glm::normalize(c);
		}
		//double ang = zrand() * pi * 2.0;
		//return glm::dvec2(std::cos(ang), std::sin(ang));
	}

	double grad_scal(const ivec2 &pos, const glm::dvec2 &dir)
	{
		return glm::dot(gradient(pos), dir);
	}

public:
	Noise(const ivec2 &seed) :
		m_seed(seed)
	{
	}

	double sample(const glm::dvec2 &pos)
	{
		ivec2 ipos(pos);
		auto spos = m_seed + ipos;
		auto lpos = pos - glm::dvec2(ipos);

		//	g0	g1
		//
		//
		//	g2	g3
		auto g0 = grad_scal(spos, -lpos);
		auto g1 = grad_scal(ivec2(spos.x + 1, spos.y), glm::dvec2(1.0 - lpos.x, - lpos.y));
		auto g2 = grad_scal(ivec2(spos.x, spos.y + 1), glm::dvec2(-lpos.x, 1.0 - lpos.y));
		auto g3 = grad_scal(ivec2(spos.x + 1, spos.y + 1), glm::dvec2(1.0 - lpos.x, 1.0 - lpos.y));

		auto g01 = glm::mix(g0, g1, lpos.x);
		auto g23 = glm::mix(g2, g3, lpos.x);
		return glm::mix(g01, g23, lpos.y);
	}
};

class World
{
	Noise m_0;
	Noise m_1;

public:
	static constexpr int64_t chunk_size = 64;

	World(void) :
		m_0(ivec2(0, 0)),
		m_1(ivec2(64000, 25000))
	{
	}

	glm::dvec3 sample(const glm::dvec2 &p)
	{
		auto pos = p + glm::dvec2(0.5);
		auto h = m_0.sample(pos * 0.1) * 3.0 + m_1.sample(pos * 0.0099) * 10.0;
		return glm::vec3(pos.x, h, pos.y);
	}

	glm::dvec3 sample_normal(const glm::dvec2 &pos)
	{
		static constexpr double bias = 0.1;
		auto a = sample(pos);
		auto b = sample(glm::dvec2(pos.x + bias, pos.y + bias * 0.5));
		auto c = sample(glm::dvec2(pos.x + bias * 0.5, pos.y + bias));
		return glm::normalize(glm::cross(b - a, c - a));
	}

	Model createChunk(Renderer &r, const ivec2 &pos, size_t scale, AccelerationStructure *acc)
	{
		static constexpr int64_t chunk_size_gen = chunk_size + 1;

		int64_t scav = static_cast<int64_t>(1) << scale;
		auto start = ivec2(pos.x * scav * chunk_size, pos.y * scav * chunk_size);
		static constexpr size_t vert_count = chunk_size_gen * chunk_size_gen;
		Vertex::pn vertices[vert_count];
		for (size_t i = 0; i < chunk_size_gen; i++)
			for (size_t j = 0; j < chunk_size_gen; j++) {
				auto &cur = vertices[i * chunk_size_gen + j];
				auto p2d = glm::dvec2(start + ivec2(j * scav, i * scav));
				cur.p = glm::dvec3(j * scav, sample(p2d).y, i * scav);
				cur.n = -sample_normal(p2d);
			}
		static constexpr size_t ind_stride = chunk_size_gen * 2 + 1;
		static constexpr size_t ind_count = ind_stride * (chunk_size_gen - 1);
		uint16_t indices[ind_count];
		std::memset(indices, 0xFF, ind_count * sizeof(uint16_t));
		for (int64_t i = 0; i < (chunk_size_gen - 1); i++) {
			for (int64_t j = 0; j < chunk_size_gen; j++) {
				indices[ind_stride * i + j * 2] = i * chunk_size_gen + j;
				indices[ind_stride * i + j * 2 + 1] = (i + 1) * chunk_size_gen + j;
			}
		}

		Model res;
		res.primitiveCount = ind_count;
		size_t buf_size = vert_count * sizeof(Vertex::pn);
		size_t ind_size = ind_count * sizeof(uint16_t);
		res.vertexBuffer = r.createVertexBuffer(buf_size);
		res.indexBuffer = r.createIndexBuffer(ind_size);
		res.indexType = VK_INDEX_TYPE_UINT16;
		r.loadBuffer(res.vertexBuffer, buf_size, vertices);
		r.loadBuffer(res.indexBuffer, ind_size, indices);

		if (r.needsAccStructure()) {
			size_t a_ind_stride = (chunk_size_gen - 1) * 6;
			size_t a_ind_count = (chunk_size_gen - 1) * a_ind_stride;
			uint16_t a_indices[a_ind_count];
			for (int64_t i = 0; i < (chunk_size_gen - 1); i++) {
				for (int64_t j = 0; j < (chunk_size_gen - 1); j++) {
					a_indices[a_ind_stride * i + j * 6] = (i + 1) * chunk_size_gen + j;
					a_indices[a_ind_stride * i + j * 6 + 1] = i * chunk_size_gen + j + 1;
					a_indices[a_ind_stride * i + j * 6 + 2] = i * chunk_size_gen + j;
					a_indices[a_ind_stride * i + j * 6 + 3] = (i + 1) * chunk_size_gen + j;
					a_indices[a_ind_stride * i + j * 6 + 4] = (i + 1) * chunk_size_gen + j + 1;
					a_indices[a_ind_stride * i + j * 6 + 5] = i * chunk_size_gen + j + 1;
				}
			}

			auto indexBuffer = r.createIndexBuffer(sizeof(a_indices));
			r.loadBuffer(indexBuffer, sizeof(a_indices), a_indices);

			*acc = r.createBottomAccelerationStructure(vert_count, sizeof(Vertex::pn), res.vertexBuffer, VK_INDEX_TYPE_UINT16, a_ind_count, indexBuffer, VK_GEOMETRY_OPAQUE_BIT_KHR);
			acc->indexType = VK_INDEX_TYPE_UINT16;
			acc->indexBuffer = indexBuffer;
		}
		return res;
	}
};

/*static double decrease(double val, double hm)
{
	if (val > 0.0 && val > hm)
		return val - hm;
	else if (val < 0.0 && val < -hm)
		return val + hm;
	else
		return 0.0;
}*/

class Game
{
	Renderer m_r;
	Map m_m;
	World m_w;

	static int64_t next_chunk_size_n(int64_t val)
	{
		auto [ures, is_neg] = iabs_save_sign(val);
		ures /= 2;
		auto res = iabs_restore_sign(ures, is_neg);
		while (res * 2 + 1 >= val)
			res--;
		while ((abs(res) % 2) == 1)
			res--;
		return res;
	}

	static int64_t next_chunk_size_p(int64_t val)
	{
		auto [ures, is_neg] = iabs_save_sign(val);
		ures /= 2;
		auto res = iabs_restore_sign(ures, is_neg);
		while (res * 2 - 1 < val)
			res++;
		while ((abs(res) % 2) == 1)
			res++;
		return res;
	}

	void gen_chunk(Pipeline *pipeline, Material *material, uint32_t model_index, Model *model, AccelerationStructurePool &acc_pool, const ivec2 &cpos, size_t scale)
	{
		int64_t scav = static_cast<int64_t>(1) << scale;
		AccelerationStructure *acc = nullptr;
		if (m_r.needsAccStructure())
			acc = acc_pool.allocate();
		*model = m_w.createChunk(m_r, cpos, scale, acc);
		auto [b, n] = m_m.addBrush<Id, Transform, MVP, MV_normal, MW_local, OpaqueRender, RT_instance>(1);
		auto off = glm::dvec3(cpos.x * scav * World::chunk_size, 0.0, cpos.y * scav * World::chunk_size);
		//std::cout << "chunk kj: " << k << ", " << j << std::endl;
		//std::cout << "chunk: " << off.x << ", " << off.y << std::endl;
		b.get<Transform>()[n] = glm::translate(off);
		auto &r = b.get<OpaqueRender>()[n];
		r.pipeline = pipeline;
		r.material = material;
		r.model = model;
		if (m_r.needsAccStructure()) {
			auto &rt = b.get<RT_instance>()[n];
			rt.mask = 1;
			rt.instanceShaderBindingTableRecordOffset = 1;
			rt.accelerationStructureReference = acc->reference;
			rt.model = model_index;
			m_r.bindModel_pn_i16(model_index, r.model->vertexBuffer, acc->indexBuffer);
			rt.material = 0;
		}
	}

	void gen_chunks(Pipeline *pipeline, Material *material, ModelPool &model_pool, AccelerationStructurePool &acc_pool)
	{
		auto pos = ivec2(0, 0);
		auto pos_end = pos + ivec2(0);
		size_t scale = -1;
		//size_t chunk_count = 0;

		for (size_t i = 0; i < 8; i++) {
			auto npos = ivec2(next_chunk_size_n(pos.x), next_chunk_size_n(pos.y));
			auto npos_end = ivec2(next_chunk_size_p(pos_end.x), next_chunk_size_p(pos_end.y));
			auto nscale = scale + 1;

			/*std::cout << "npos: " << npos.x << ", " << npos.y << std::endl;
			std::cout << "npos_end: " << npos_end.x << ", " << npos_end.y << std::endl;
			std::cout << "scav: " << scav << std::endl;*/

			for (int64_t j = npos.y; j < npos_end.y; j++)
				for (int64_t k = npos.x; k < npos_end.x; k++) {
					auto cpos = ivec2(k, j);
					auto cpos_sca = cpos * static_cast<int64_t>(2);
					if (!(cpos_sca.x >= pos.x && cpos_sca.y >= pos.y && cpos_sca.x < pos_end.x && cpos_sca.y < pos_end.y)) {
						//chunk_count++;
						uint32_t model_index = model_pool.currentIndex();
						gen_chunk(pipeline, material, model_index, model_pool.allocate(), acc_pool, cpos, nscale);
					}
				}

			pos = npos;
			pos_end = npos_end;
			scale = nscale;
		}
		//std::cout << "chunk count: " << chunk_count << std::endl;
	}

public:
	void run(void)
	{
		auto grass = m_r.allocateImage(m_r.loadImage("res/img/grass.png"));
		auto vokselia_spawn_albedo = m_r.allocateImage(m_r.loadImage("res/mod/vokselia_spawn_albedo.png", false), false);
		//auto vokselia_spawn_albedo = m_r.allocateImage(m_r.loadImage("res/img/sand_001/albedo.png"));
		/*auto normal = */m_r.allocateImage(m_r.loadImage("res/img/sand_001/normal.png", true, VK_FORMAT_R8G8B8A8_UNORM), true, VK_FORMAT_R8G8B8A8_UNORM);
		/*auto normal = */m_r.allocateImage(m_r.loadImage("res/img/sand_001/height.png", true, VK_FORMAT_R8G8B8A8_UNORM), true, VK_FORMAT_R8G8B8A8_UNORM);
		/*auto normal = */m_r.allocateImage(m_r.loadImage("res/img/sand_001/ao.png", true, VK_FORMAT_R8G8B8A8_UNORM), true, VK_FORMAT_R8G8B8A8_UNORM);

		Material_albedo mat_alb[] {
			{grass},
			{vokselia_spawn_albedo}
		};
		Material mat[2];
		reinterpret_cast<Material_albedo&>(mat[0]).albedo = grass;
		reinterpret_cast<Material_albedo&>(mat[1]).albedo = vokselia_spawn_albedo;
		size_t first_mat = ~0ULL;
		{
			for (size_t i = 0; i < array_size(mat_alb); i++) {
				auto m = m_r.allocateMaterial();
				if (first_mat == ~0ULL)
					first_mat = m;
			}
		}

		if (m_r.needsAccStructure())
			m_r.bindMaterials_albedo(first_mat, array_size(mat_alb), mat_alb);

		/*{
			auto [b, n] = m_m.addBrush<Id, Transform, MVP, MV_normal, OpaqueRender, RT_instance>(1);
			b.get<Transform>()[n] = glm::scale(glm::dvec3(100.0));
			auto &r = b.get<OpaqueRender>()[n];
			r.pipeline = m_r.pipeline_opaque_tb;
			r.material = &mat[1];
			uint32_t model_ndx = m_r.m_model_pool.currentIndex();
			r.model = m_r.m_model_pool.allocate();
			AccelerationStructure *acc = m_r.needsAccStructure() ? m_r.m_acc_pool.allocate() : nullptr;
			*r.model = m_r.loadModelTb("res/mod/vokselia_spawn.obj", acc);
			if (m_r.needsAccStructure()) {
				auto &rt = b.get<RT_instance>()[n];
				rt.mask = 1;
				rt.instanceShaderBindingTableRecordOffset = 2;
				rt.accelerationStructureReference = acc->reference;
				rt.model = model_ndx;
				m_r.bindModel_pntbu(model_ndx, r.model->vertexBuffer);
				rt.material = 1;
			}
		}*/

		m_r.instanciateModel(m_m, "res/mod/Sponza-master/", "sponza.obj");
		/*{
			auto [b, n] = m_m.addBrush<Id, Transform, MVP, MV_normal, OpaqueRender, RT_instance>(1);
			b.get<Transform>()[n] = glm::scale(glm::dvec3(1.0));
			auto &r = b.get<OpaqueRender>()[n];
			r.pipeline = pipeline_opaque_tb;
			r.material = material_albedo;
			uint32_t model_ndx = model_pool.currentIndex();
			r.model = model_pool.allocate();
			AccelerationStructure *acc = m_r.needsAccStructure() ? acc_pool.allocate() : nullptr;
			*r.model = m_r.loadModelFull("res/mod/sponza/", "sponza.obj", acc);
			if (m_r.needsAccStructure()) {
				auto &rt = b.get<RT_instance>()[n];
				rt.mask = 1;
				rt.instanceShaderBindingTableRecordOffset = 2;
				rt.accelerationStructureReference = acc->reference;
				rt.model = model_ndx;
				m_r.bindModel_pntbu(model_ndx, r.model->vertexBuffer);
				rt.material = 1;
			}
		}*/
		//gen_chunks(pipeline_opaque_uvgen, material_grass, model_pool, acc_pool);

		auto bef = std::chrono::high_resolution_clock::now();
		double t = 0.0;
		auto camera_pos = glm::dvec3(0.0);
		auto camera_speed = glm::dvec3(0.0);
		bool cursor_mode = false;
		m_r.setCursorMode(false);
		auto base_cursor = glm::dvec2(0.0);
		bool esc_prev = false;
		bool esc = false;
		bool first_it = true;
		bool is_noclip = true;
		bool grounded = false;
		while (true) {
			m_r.resetFrame();
			auto now = std::chrono::high_resolution_clock::now();
			auto delta = static_cast<std::chrono::duration<double>>(now - bef).count();
			bef = now;
			t += delta;
			glm::dmat4 last_view, view, proj;
			const double near = 0.1, far = 64000.0;
			const double ang_rad = pi / 180.0;

			m_r.pollEvents();
			if (m_r.shouldClose())
				break;

			const double ratio = static_cast<double>(m_r.swapchainExtent().width) / static_cast<double>(m_r.swapchainExtent().height),
				fov = 70.0 * ang_rad;
			{
				proj = glm::perspectiveLH_ZO<float>(fov, ratio, far, near);
				proj[1][1] *= -1.0;

				auto cursor = base_cursor;
				if (!cursor_mode)
					cursor = m_r.cursor() - base_cursor;
				{
					esc_prev = esc;
					esc = m_r.keyState(GLFW_KEY_ESCAPE);
				}
				if (esc && !esc_prev) {
					if (!cursor_mode)
						base_cursor = cursor;
					else
						base_cursor = m_r.cursor() - base_cursor;
					cursor_mode = !cursor_mode;
					m_r.setCursorMode(cursor_mode);
				}
				const float sensi = 0.1;
				double move = m_r.keyState(GLFW_KEY_LEFT_SHIFT) ? 20.0 : 2.0;
				//double move = m_r.keyState(GLFW_KEY_LEFT_SHIFT) ? 1000.0 : 100.0;
				double movenc = 10.0;
				auto view_rot = glm::rotate((-cursor.x * ang_rad) * sensi, glm::dvec3(0.0, 1.0, 0.0));
				view_rot = glm::rotate(std::clamp(-cursor.y * sensi, -90.0, 90.0) * ang_rad, glm::dvec3(1.0, 0.0, 0.0)) * view_rot;
				auto view_rot_inverse = glm::inverse(view_rot);
				auto dir_fwd_w = view_rot_inverse * glm::dvec4(0.0, 0.0, 1.0, 0.0);
				auto dir_fwd = glm::dvec3(dir_fwd_w.x, dir_fwd_w.y, dir_fwd_w.z);
				auto dir_side_w = view_rot_inverse * glm::dvec4(1.0, 0.0, 0.0, 0.0);
				auto dir_side = glm::dvec3(dir_side_w.x, dir_side_w.y, dir_side_w.z);

				auto dir_side_aw = view_rot_inverse * glm::dvec4(-1.0, 0.0, 1.0, 0.0);
				auto dir_side_a = glm::dvec3(dir_side_aw.x, dir_side_aw.y, dir_side_aw.z);
				auto dir_side_dw = view_rot_inverse * glm::dvec4(1.0, 0.0, 1.0, 0.0);
				auto dir_side_d = glm::dvec3(dir_side_dw.x, dir_side_dw.y, dir_side_dw.z);
				if (m_r.keyReleased(GLFW_KEY_N)) {
					is_noclip = !is_noclip;
					if (!is_noclip)
						camera_speed = glm::dvec3(0.0);
				}
				if (is_noclip) {
					if (m_r.keyState(GLFW_KEY_W))
						camera_pos += dir_fwd * move * delta;
					if (m_r.keyState(GLFW_KEY_S))
						camera_pos -= dir_fwd * move * delta;
					if (m_r.keyState(GLFW_KEY_A))
						camera_pos -= dir_side * move * delta;
					if (m_r.keyState(GLFW_KEY_D))
						camera_pos += dir_side * move * delta;
				} else {
					/*if (grounded) {
						auto mul = 1.0 - delta;
						camera_speed.x *= mul;
						camera_speed.z *= mul;
					}*/
					camera_speed.y -= 9.8 * delta;
					if (grounded) {
						if (m_r.keyState(GLFW_KEY_SPACE))
							camera_speed.y = 5.0;
					}
					if (m_r.keyState(GLFW_KEY_W))
						camera_speed += dir_fwd * movenc * delta;
					if (m_r.keyState(GLFW_KEY_S)) {
						auto mul = 1.0 - delta;
						camera_speed.x *= mul;
						camera_speed.z *= mul;
					}
					if (m_r.keyState(GLFW_KEY_A)) {
						if (!grounded) {
							auto len = glm::length(camera_speed);
							camera_speed = camera_speed + len * dir_side_a * delta;
							camera_speed = glm::normalize(camera_speed) * (glm::mix(len, glm::length(camera_speed), 0.1));
						} else
							camera_speed -= dir_side * movenc * delta;
					}
					if (m_r.keyState(GLFW_KEY_D)) {
						if (!grounded) {
							auto len = glm::length(camera_speed);
							camera_speed = camera_speed + len * dir_side_d * delta;
							camera_speed = glm::normalize(camera_speed) * (glm::mix(len, glm::length(camera_speed), 0.1));
						} else
							camera_speed += dir_side * movenc * delta;
					}
					auto fpos = camera_pos;
					camera_pos += camera_speed * delta;
					const double ph = 1.8;
					auto h = m_w.sample(glm::dvec2(camera_pos.x, camera_pos.z)).y;
					grounded = false;
					if (h > (camera_pos.y - ph)) {
						camera_pos.y = h + ph;
						if (glm::length(camera_pos - fpos) > 0.0) {
							auto len = glm::length(camera_speed);
							camera_speed = (camera_pos - fpos) * (1.0 / delta);
							if (glm::length(camera_speed) > len)
								camera_speed = glm::normalize(camera_speed) * len;
						}
						grounded = true;
					}
				}
				last_view = view;
				view = view_rot * glm::translate(-camera_pos);
				if (first_it)
					last_view = view;
				auto vp = proj * view;

				m_m.query<MVP>([&](Brush &b){
					auto size = b.size();
					auto mvp = b.get<MVP>();
					auto mv_normal = b.get<MV_normal>();
					auto trans = b.get<Transform>();
					for (size_t i = 0; i < size; i++) {
						mvp[i] = vp * trans[i];
						auto normal = view * trans[i];
						for (size_t j = 0; j < 3; j++)
							for (size_t k = 0; k < 3; k++) {
								mv_normal[i][j][k] = normal[j][k];
							}
					}
				});

				m_m.query<MW_local>([&](Brush &b){
					auto size = b.size();
					auto mw_local = b.get<MW_local>();
					auto trans = b.get<Transform>();
					for (size_t i = 0; i < size; i++) {
						for (size_t j = 0; j < 3; j++)
							for (size_t k = 0; k < 3; k++) {
								mw_local[i][j][k] = trans[i][j][k];
							}
					}
				});
			}
			m_r.render(m_m, Camera{last_view, view, proj, static_cast<float>(far), static_cast<float>(near), glm::vec2(ratio, -1.0) * glm::vec2(std::tan(fov / 2.0))});
			first_it = false;
		}

		m_r.waitIdle();
	}

	Game(bool validate, bool useRenderDoc) :
		m_r(3, validate, useRenderDoc)
	{
	}
	~Game(void)
	{
	}
};

int main(int argc, char **argv)
{
	bool validate = false;
	bool is_render_doc = false;
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-v") == 0)
			validate = true;
		if (std::strcmp(argv[i], "-r") == 0)
			is_render_doc = true;
	}

	auto g = Game(validate || is_debug, is_render_doc);
	g.run();
	return 0;
}