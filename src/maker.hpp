#pragma once
#include <algorithm>
#include <vector>

template<class ret_t, class range_t, class f_t>
auto make(const range_t& range, f_t f)-> ret_t {
	using std::begin;
	using std::end;

	ret_t ret;
	ret.reserve(std::distance(begin(range), end(range)));
	std::transform(begin(range), end(range), std::back_inserter(ret), f);
	return ret;
}

int main(int argc, char const *argv[]) {
	auto v1 = std::vector<double>(10, 1.0);
	auto v2 = make<std::vector<int>>(v1, [](auto x){return int(x)*10;});
	return 0;
}
