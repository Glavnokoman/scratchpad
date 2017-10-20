#pragma once

#include <cstdint>

template<uint64_t val>
struct RequiredBits {
	enum {value = val < 2 ? 1 
	            : val <= 1u << 8  ? 8
	            : val <= 1u << 16 ? 16
	            : val <= 1ul << 32 ? 32
	            :                    64 };
};

template<uint8_t bits> struct Uint;
template<> struct Uint<1> { using type = bool; };
template<> struct Uint<8> { using type = uint8_t; };
template<> struct Uint<16> { using type = uint16_t; };
template<> struct Uint<32> { using type = uint32_t; };
template<> struct Uint<64> { using type = uint64_t; };


template<uint16_t begin, uint16_t end, class Res = typename Uint<end - begin>::type>
struct Bitpack {
	// proxy
	struct V {
		operator Res();
		auto operator=(Res)-> Res;
		
		uint8_t* const buf;
	};
	
	auto operator ()(uint8_t* buf){ return V{buf}; }
};

template <uint16_t begin, uint16_t end=begin+1>
auto bitpack(uint8_t* buf){
	return BitPack<begin, end, Uint<8>>(buf);
}

auto usage()-> void {
	struct Head {
		
		BitPack<3, 44> f2;
	};
	
	uint8_t buf[15];
	auto f2 = Head::f2(buf);
}
