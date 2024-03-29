#include "Cmp.hpp"
#include <cstring>

namespace Rosee {

void Id::init(void*, size_t)
{
}

void Id::destr(void*, size_t)
{
}

void Transform::init(void*, size_t)
{
}

void Transform::destr(void*, size_t)
{
}

void Point2D::init(void*, size_t)
{
}

void Point2D::destr(void*, size_t)
{
}

void Render::init(void*, size_t)
{
}

void Render::destr(void*, size_t)
{
}

void MVP::init(void*, size_t)
{
}

void MVP::destr(void*, size_t)
{
}

void MV_normal::init(void*, size_t)
{
}

void MV_normal::destr(void*, size_t)
{
}

void MW_local::init(void*, size_t)
{
}

void MW_local::destr(void*, size_t)
{
}

void RT_instance::init(void*, size_t)
{
}

void RT_instance::destr(void*, size_t)
{
}

namespace Cmp {

static constexpr sarray<size_t, max> get_sizes(void)
{
	sarray<size_t, max> res;

	list::fill_sizes(res.cdata());
	return res;
}

static constexpr sarray<init_t, max> get_inits(void)
{
	sarray<init_t, max> res;

	list::fill_inits(res.cdata());
	return res;
}

static constexpr sarray<init_t, max> get_destrs(void)
{
	sarray<destr_t, max> res;

	list::fill_destrs(res.cdata());
	return res;
}

size_t size[max];
init_t init[max];
destr_t destr[max];

struct Init {
	Init(void)
	{
		constexpr auto sizes = get_sizes();
		std::memcpy(size, sizes.data(), sizeof(*size) * sizes.size());
		constexpr auto inits = get_inits();
		std::memcpy(init, inits.data(), sizeof(*init) * inits.size());
		constexpr auto destrs = get_destrs();
		std::memcpy(destr, destrs.data(), sizeof(*destr) * destrs.size());
	}
};

static auto init_lists = Init();

}
}