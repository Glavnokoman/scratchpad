#include <bitpack.hpp>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

namespace {
	struct G {
		enum {size = 64};
		
		using f0_8 = BitRange<0, 8>;
		using f0_1  = BitRange<0, 1>;
		using f6_7  = BitRange<6>;
		using f5_8  = BitRange<5, 8>;
		using f6_9  = BitRange<6, 9>;
		using f7_8  = BitRange<7, 8> ;
		using f5_15 = BitRange<5, 15>;
		using f1_17 = BitRange<1, 17>;
		using f0_32 = BitRange<0, 32>;
		using f35_40 = BitRange<35, 40>;
		using f32_64 = BitRange<32, 64>;
		using f64_88 = BitRange<64, 88>;
	};
	
	const uint8_t cbuf[12] = { 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                         , 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                         , 0b01001010, 0b00110101, 0b11101100, 0b00101101 };
	uint8_t buf[12] = {0u};
} // namespace name

TEST_CASE("read bitranges", "[read_bitfoelds"){
	CHECK(bits<G::f0_8>(cbuf) == cbuf[0]);
	
//	bits<G::f0_8>(cbuf) = 0;
	
	bits<G::f0_8>(buf) = cbuf[0];
	CHECK(bits<G::f0_8>(buf) == cbuf[0]);
}

TEST_CASE("read bitfields", "[read_bitfields]"){
	CHECK(bits<G::f0_8>(cbuf) == cbuf[0]);
	CHECK(bits<G::f0_1>(cbuf) == false);
	CHECK(bits<G::f6_7>(cbuf) == true);
	CHECK(bits<G::f5_8>(cbuf) == 0b010u);
	CHECK(bits<G::f6_9>(cbuf) == 0b100u);
	CHECK(bits<G::f7_8>(cbuf) == false);
	CHECK(bits<G::f5_15>(cbuf) == 0b0100011010u);
	CHECK(bits<G::f1_17>(cbuf) == 0b1001010001101011u);
	CHECK(bits<G::f0_32>(cbuf) == 0b01001010001101011110110000101101u);
	CHECK(bits<G::f35_40>(cbuf) == 0b01010u);
	CHECK(bits<G::f32_64>(cbuf) == 0b01001010001101011110110000101101u);
	CHECK(bits<G::f64_88>(cbuf) == 0b010010100011010111101100u);
}

TEST_CASE("write bitfields", "[write_bitfields]"){
	bits<G::f0_8>(buf) = cbuf[0];
	CHECK(bits<G::f0_8>(buf) == cbuf[0]);
	
	bits<G::f0_1>(buf) = false;
	CHECK(bits<G::f0_1>(buf) == false);

	bits<G::f6_7>(buf) = true;
	CHECK(bits<G::f6_7>(buf) == true);
	
	bits<G::f5_8>(buf) = 0b010u;
	CHECK(bits<G::f5_8>(buf) == 0b010u);
	
	bits<G::f6_9>(buf) = 0b100u;
	CHECK(bits<G::f6_9>(buf) == 0b100u);
	
	bits<G::f7_8>(buf) = false;
	CHECK(bits<G::f7_8>(buf) == false);

	bits<G::f5_15>(buf) = 0b0100011010u;
	CHECK(bits<G::f5_15>(buf) == 0b0100011010u);
	
	bits<G::f1_17>(buf) = 0b1001010001101011u;
	CHECK(bits<G::f1_17>(buf) == 0b1001010001101011u);
	
	bits<G::f0_32>(buf) = 0b01001010001101011110110000101101u;
	CHECK(bits<G::f0_32>(buf) == 0b01001010001101011110110000101101u);
	
	bits<G::f35_40>(buf) = 0b01010u;
	CHECK(bits<G::f35_40>(buf) == 0b01010u);
	
	bits<G::f32_64>(buf) = 0b01001010001101011110110000101101u;
	CHECK(bits<G::f32_64>(buf) == 0b01001010001101011110110000101101u);
	
	bits<G::f64_88>(buf) = 0b010010100011010111101100u;
	CHECK(bits<G::f64_88>(buf) == 0b010010100011010111101100u);
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
