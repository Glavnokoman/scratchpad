#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#include "vector_erase_indexes.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

template<class F>
auto test_function()-> void {
	auto vec_in = std::vector<int>{1, 2, 3, 4, 5, 6};
	auto f = F{};
	
	SECTION("remove nothing"){
		auto idx_remove = std::vector<ptrdiff_t>{};
		auto vec_out = f(vec_in, idx_remove);
		CHECK(vec_out == vec_in);
	}
	SECTION("remove all"){
		auto idx_remove = std::vector<ptrdiff_t>(vec_in.size());
		std::iota(begin(idx_remove), end(idx_remove), 0);
		auto vec_ref = std::vector<int>{};
		
		auto vec_tst = f(vec_in, idx_remove);
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove first"){
		auto idx_rm = std::vector<ptrdiff_t>{0};
		auto vec_ref = std::vector<int>{2, 3, 4, 5, 6};
		
		auto vec_tst = f(vec_in, idx_rm);
		if(!f.is_stable()){
			std::sort(begin(vec_tst), end(vec_tst));
		}
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove last"){
		auto idx_rm = std::vector<ptrdiff_t>{5};
		auto vec_ref = std::vector<int>{1, 2, 3, 4, 5};
		
		auto vec_tst = f(vec_in, idx_rm);
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove second"){
		auto idx_rm = std::vector<ptrdiff_t>{1};
		auto vec_ref = std::vector<int>{1, 3, 4, 5, 6};

		auto vec_tst = f(vec_in, idx_rm);
		if(!f.is_stable()){
			std::sort(begin(vec_tst), end(vec_tst));
		}
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove one before last"){
		auto idx_rm = std::vector<ptrdiff_t>{4};
		auto vec_ref = std::vector<int>{1, 2, 3, 4, 6};
		
		auto vec_tst = f(vec_in, idx_rm);
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but first"){
		auto idx_rm = std::vector<ptrdiff_t>{1, 2, 3, 4, 5};
		auto vec_ref = std::vector<int>{1};
		
		auto vec_tst = f(vec_in, idx_rm);
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but last"){
		auto idx_rm = std::vector<ptrdiff_t>{0, 1, 2, 3, 4};
		auto vec_ref = std::vector<int>{6};
		
		auto vec_tst = f(vec_in, idx_rm);
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but first and last"){
		auto idx_rm = std::vector<ptrdiff_t>{1, 2, 3, 4};
		auto vec_ref = std::vector<int>{1, 6};
		
		auto vec_tst = f(vec_in, idx_rm);
		CHECK(vec_tst == vec_ref);
	}
}

struct EraserStable{
	template<class T>
	auto operator()(std::vector<T> in, const std::vector<ptrdiff_t>& idx)-> std::vector<T> {
		return erase_indexes_stable(in, idx);
	}
	auto is_stable()-> bool {return true;}
};

struct EraserStableSorted{
	template<class T>
	auto operator()(std::vector<T> in, const std::vector<ptrdiff_t>& idx)-> std::vector<T> {
		return erase_sorted_indexes_stable(in, idx);
	}
	auto is_stable()-> bool { return true; }
};

struct EraserUnstableSorted{
	template<class T>
	auto operator()(std::vector<T> in, const std::vector<ptrdiff_t>& idx)-> std::vector<T> {
		return erase_sorted_indexes_unstable(in, idx);
	}
	auto is_stable()-> bool { return false; }
};	


TEST_CASE("test stable erasing vector elements by index list (lame)", "[vec_erasure_stable_lame]"){
	test_function<EraserStable>();
}

TEST_CASE("test stable erasing vector elements by index list", "[vec_erasure_stable]"){
	test_function<EraserStableSorted>();
}

TEST_CASE("test unstable erasing vector elements by index list", "[vec_erasure_unstable]"){
	test_function<EraserUnstableSorted>();
}


int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
