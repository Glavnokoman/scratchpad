#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#include <algorithm>
#include <numeric>
#include <vector>
#include "vector_erase_indexes.hpp"

TEST_CASE("test vector erasuse", "[vec_erasure]"){
	auto vec_in = std::vector<int>{1, 2, 3, 4, 5, 6};
	
	SECTION("remove nothing"){
		auto idx_remove = std::vector<ptrdiff_t>{};
		auto vec_out = erase_indexes_stable(vec_in, idx_remove);
		CHECK(vec_out == vec_in);
	}
	SECTION("remove all"){
		auto idx_remove = std::vector<ptrdiff_t>(vec_in.size());
		std::iota(begin(idx_remove), end(idx_remove), 0);
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_remove);
		auto vec_ref = std::vector<int>{};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove first"){
		auto idx_rm = std::vector<ptrdiff_t>{0};
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{2, 3, 4, 5, 6};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove last"){
		auto idx_rm = std::vector<ptrdiff_t>{5};
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{1, 2, 3, 4, 5};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove second"){
		auto idx_rm = std::vector<ptrdiff_t>{1};

		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{1, 3, 4, 5, 6};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove one before last"){
		auto idx_rm = std::vector<ptrdiff_t>{4};
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{1, 2, 3, 4, 6};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but first"){
		auto idx_rm = std::vector<ptrdiff_t>{1, 2, 3, 4, 5};
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{1};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but last"){
		auto idx_rm = std::vector<ptrdiff_t>{0, 1, 2, 3, 4};
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{6};
		
		CHECK(vec_tst == vec_ref);
	}
	SECTION("remove all but first and last"){
		auto idx_rm = std::vector<ptrdiff_t>{1, 2, 3, 4};
		
		auto vec_tst = erase_indexes_stable(vec_in, idx_rm);
		auto vec_ref = std::vector<int>{1, 6};
		
		CHECK(vec_tst == vec_ref);
	}
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
