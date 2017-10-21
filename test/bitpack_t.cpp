#include <bitpack.hpp>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

namespace {

	// test bitpack
	struct H {
		enum {size = 32};
		
		using f0_8  = bitpack<0, 8>;
		using f0_1  = bitpack<0, 1>;
		using f6_7  = bitpack<6, 7>;
		using f6_9  = bitpack<6, 9>;
		using f7_8  = bitpack<7, 8> ;
		using f5_15 = bitpack<5, 15>;
		using f0_32 = bitpack<0, 32>;
		using f1_17 = bitpack<1, 17>;
	};
	
	uint8_t buf[4] = {0b11001010, 0b00110101, 0b11101100, 0b00101101};
	const uint8_t cbuf[4] = {0b11001010, 0b00110101, 0b11101100, 0b00101101};
} // namespace name

TEST_CASE("test something", "[test_smth]"){
	uint8_t v0_8 = H::f0_8(buf);
	CHECK(v0_8 == 0b11001010);
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
