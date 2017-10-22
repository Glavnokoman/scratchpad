#include <bitpack.hpp>

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

namespace {

	// test bitpack
	struct H {
		enum {size = 64};
		
		using f0_8  = bitpack<0, 8>;
		using f0_1  = bitpack<0, 1>;
		using f6_7  = bitpack<6>;
		using f5_8  = bitpack<5, 8>;
		using f6_9  = bitpack<6, 9>;
		using f7_8  = bitpack<7, 8> ;
		using f5_15 = bitpack<5, 15>;
		using f1_17 = bitpack<1, 17>;
		using f0_32 = bitpack<0, 32>;
		using f35_40 = bitpack<35, 40>;
		using f32_64 = bitpack<32, 64>;
		using f64_88 = bitpack<64, 88>;
	};
	
	uint8_t cbuf[12] = { 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                   , 0b01001010, 0b00110101, 0b11101100, 0b00101101
	                   , 0b01001010, 0b00110101, 0b11101100, 0b00101101 };
	uint8_t buf[12] = {0u};
} // namespace name

TEST_CASE("read bitfields", "[read_bitfields]"){
	CHECK(H::f0_8(cbuf) == cbuf[0]);
	CHECK(H::f0_1(cbuf) == false);
	CHECK(H::f6_7(cbuf) == true);
	CHECK(H::f5_8(cbuf) == 0b010u);
	CHECK(H::f6_9(cbuf) == 0b100u);
	CHECK(H::f7_8(cbuf) == false);
	CHECK(H::f5_15(cbuf) == 0b0100011010u);
	CHECK(H::f1_17(cbuf) == 0b1001010001101011u);
	CHECK(H::f0_32(cbuf) == 0b01001010001101011110110000101101u);
	CHECK(H::f35_40(cbuf) == 0b01010u);
	CHECK(H::f32_64(cbuf) == 0b01001010001101011110110000101101u);
	CHECK(H::f64_88(cbuf) == 0b010010100011010111101100u);
}

TEST_CASE("write bitfields", "[write_bitfields]"){
	H::f0_8{buf} = cbuf[0];
	CHECK(H::f0_8{buf} == cbuf[0]);
	
	H::f0_1{buf} = false;
	CHECK(H::f0_1(buf) == false);

	H::f6_7{buf} = true;
	CHECK(H::f6_7(buf) == true);
	
	H::f5_8{buf} = 0b010u;
	CHECK(H::f5_8(buf) == 0b010u);
	
	H::f6_9{buf} = 0b100u;
	CHECK(H::f6_9(buf) == 0b100u);
	
	H::f7_8{buf} = false;
	CHECK(H::f7_8(buf) == false);

	H::f5_15{buf} = 0b0100011010u;
	CHECK(H::f5_15(buf) == 0b0100011010u);
	
	H::f1_17{buf} = 0b1001010001101011u;
	CHECK(H::f1_17(buf) == 0b1001010001101011u);
	
	H::f0_32{buf} = 0b01001010001101011110110000101101u;
	CHECK(H::f0_32(buf) == 0b01001010001101011110110000101101u);
	
	H::f35_40{buf} = 0b01010u;
	CHECK(H::f35_40(buf) == 0b01010u);
	
	H::f32_64{buf} = 0b01001010001101011110110000101101u;
	CHECK(H::f32_64(buf) == 0b01001010001101011110110000101101u);
	
	H::f64_88{buf} = 0b010010100011010111101100u;
	CHECK(H::f64_88(buf) == 0b010010100011010111101100u);
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
