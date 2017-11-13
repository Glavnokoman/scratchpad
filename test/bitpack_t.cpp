#include <bitpack.hpp>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

namespace {
	struct G {
		enum {size = 64};
		
		using f0_8  = Bits<0, 8>;
		using f0_1  = Bits<0, 1>;
		using f6_7  = Bits<6>;
		using f5_8  = Bits<5, 8>;
		using f6_9  = Bits<6, 9>;
		using f7_8  = Bits<7, 8> ;
		using f5_15 = Bits<5, 15>;
		using f1_17 = Bits<1, 17>;
		using f0_32 = Bits<0, 32>;
		using f35_40 = Bits<35, 40>;
		using f32_64 = Bits<32, 64>;
		using f64_88 = Bits<64, 88>;
	};
	
	const uint8_t cbuf[12] = { 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                         , 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                         , 0b01001010, 0b00110101, 0b11101100, 0b00101101 };
//	uint8_t buf[12] = {0u};
} // namespace name

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

TEST_CASE("or bitfields", "[or_bitfields]"){
	uint8_t buf[12] = {0u};

	SECTION("write 1 complete byte in the head"){
		bits<G::f0_8>(buf) |= cbuf[0];
		CHECK(bits<G::f0_8>(buf) == cbuf[0]);
	}
	SECTION("write 1 bit in the head"){
		bits<G::f0_1>(buf) |= false;
		CHECK(bits<G::f0_1>(buf) == false);
		bits<G::f0_1>(buf) |= true;
		CHECK(bits<G::f0_1>(buf) == true);
	}
	SECTION("write one bit in the middle of first byte"){
		bits<G::f6_7>(buf) |= false;
		CHECK(bits<G::f6_7>(buf) == false);
		bits<G::f6_7>(buf) |= true;
		CHECK(bits<G::f6_7>(buf) == true);
	}
	SECTION("several bits at the end of a byte"){
		bits<G::f5_8>(buf) |= 0b010u;
		CHECK(bits<G::f5_8>(buf) == 0b010u);
	}
	SECTION("several bits in the beginning of a byte"){
		bits<G::f6_9>(buf) |= 0b100u;
		CHECK(bits<G::f6_9>(buf) == 0b100u);
	}
	SECTION("write last bit of a byte"){
		bits<G::f7_8>(buf) |= false;
		CHECK(bits<G::f7_8>(buf) == false);
		bits<G::f7_8>(buf) |= true;
		CHECK(bits<G::f7_8>(buf) == true);
	}
	SECTION("span write over a byte border (incomplete bytes)"){
		bits<G::f5_15>(buf) |= 0b0100011010u;
		CHECK(bits<G::f5_15>(buf) == 0b0100011010u);
	}
	SECTION("span write over a byte border (complete bytes)"){
		bits<G::f1_17>(buf) |= 0b1001010001101011u;
		CHECK(bits<G::f1_17>(buf) == 0b1001010001101011u);
	}
	SECTION("write 4 bytes in the beginning"){
		bits<G::f0_32>(buf) |= 0b01001010001101011110110000101101u;
		CHECK(bits<G::f0_32>(buf) == 0b01001010001101011110110000101101u);
	}
	SECTION("span write over a byte border (incomplete byte)"){
		bits<G::f35_40>(buf) |= 0b01010u;
		CHECK(bits<G::f35_40>(buf) == 0b01010u);
	}
	SECTION("write 4 bytes in the middle of buf"){
		bits<G::f32_64>(buf) |= 0b01001010001101011110110000101101u;
		CHECK(bits<G::f32_64>(buf) == 0b01001010001101011110110000101101u);
	}
	SECTION("write 3 bytes after the 64 bit offset"){
		bits<G::f64_88>(buf) |= 0b010010100011010111101100u;
		CHECK(bits<G::f64_88>(buf) == 0b010010100011010111101100u);
	}
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
