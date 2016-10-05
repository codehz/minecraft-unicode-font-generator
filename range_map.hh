#ifndef RANGE_MAP_HH
#define RANGE_MAP_HH

#include <set>

template <typename Num>
using range = std::pair<Num, Num>;

template <typename Num>
range<Num> make_range(Num lower, Num upper) {
	if (upper < lower) return std::make_pair(upper, lower);
	else return std::make_pair(lower, upper);
}

template <typename Num>
range<Num> make_single_range(Num num) {
	return make_range(num, num);
}

template <typename Num>
struct range_cmp {
	bool operator()(range<Num> const &a, range<Num> const &b) const {
		return a.second < b.first;
	}
};

template<typename Num, typename Type>
using range_map = std::set<range<Num>, range_cmp<Num>>;

#endif
