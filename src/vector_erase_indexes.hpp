#pragma once

#include <algorithm>
#include <cassert>
#include <vector>
#include <utility>

using std::begin;
using std::end;

template<class T>
auto offit(T&& container, typename std::remove_reference_t<T>::difference_type offset) {
	return std::next(begin(std::forward<T>(container)), offset);
}

/// Remove multiple indexes from a vector maintaining the order of elements.
/// Lame, only here for benchmarking/testing.
template<class T>
auto erase_indexes_stable_(std::vector<T>& inout
                           , const std::vector<ptrdiff_t>& idx
                           )-> void 
{
	auto tr = idx; // reverse sorted indexes to remove
	std::sort(begin(tr), end(tr), [](auto x1, auto x2){return x1 > x2;});
	for(auto id: tr){
		inout.erase(offit(inout, id));
	}
}

/// Remove multiple indexes from a vector maintaining the order of elements. 
/// Lame, only here for benchmarking/testing.
template<class T>
auto erase_indexes_stable(std::vector<T> in
                          , const std::vector<ptrdiff_t>& idx
                          )-> std::vector<T> 
{
	auto tr = idx; // reverse sorted indexes to remove
	std::sort(begin(tr), end(tr), [](auto x1, auto x2){return x1 > x2;});
	for(auto id: tr){
		in.erase(offit(in, id));
	}
	return in;
}

/// remove multiple indexes from a vector maintaining the order of elements.
/// @pre indexes to remove must be sorted (increasing order)
template<class T>
auto erase_sorted_indexes_stable(std::vector<T> in
                                 , const std::vector<ptrdiff_t>& idx
                                 )-> std::vector<T>
{
	assert(std::is_sorted(begin(idx), end(idx)));
	if(idx.empty()){
		return in;
	}
	
	auto move_to = offit(in, idx.front());
	for(auto dit = begin(idx); dit != std::prev(end(idx)); ++dit){
		move_to = std::move( std::next(offit(in, *dit)), offit(in, *std::next(dit)), move_to);
	}
	std::move(std::next(offit(in, idx.back())), end(in), move_to);
	in.resize(in.size() - idx.size());
	return in;
}

/// remove multiple indexes from a vector not maintaining the order of elements.
/// @pre indexes to remove must be sorted (increasing order)
template<class T>
auto erase_sorted_indexes_unstable(std::vector<T> in
                                   , const std::vector<ptrdiff_t>& idx
                                   )-> std::vector<T>
{
	assert(std::is_sorted(begin(idx), end(idx)));
	
	auto hr = idx.begin();  // leading index left to be removed
	auto tt = idx.rbegin(); // last index left to be removed
	for(ptrdiff_t lid = in.size() - 1; /**/; --lid, ++hr){
		while(lid == *tt){
			lid -= 1;
			++tt;
		}
		if(lid > *hr){
			iter_swap(offit(in, lid), offit(in, *hr));
		} else {
			break;
		}
	}
	in.resize(in.size() - idx.size());
	
	return in;
}
