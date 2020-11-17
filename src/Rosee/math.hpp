#pragma once

namespace Rosee {

template <typename T>
const T& min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T>
T& max(const T &a, const T &b)
{
	return a > b ? a : b;
}

}