#pragma once

#include <algorithm>
#include <cstdint>

template<uint64_t val>
struct RequiredBits {
	enum {value = val <= 1u       ? 1 
	            : val <= 1u << 8  ? 8
	            : val <= 1u << 16 ? 16
	            : val <= 1u << 32 ? 32
	            :                   64 };
};

template<uint8_t val>
struct TypeBits {
	enum { value = val <= 1  ? 1
	             : val <= 8  ? 8
	             : val <= 16 ? 16
	             : val <= 32 ? 32
	             :             64 };
};

template<uint8_t bits> struct Uint;
template<> struct Uint<1>  { using type = bool; };
template<> struct Uint<8>  { using type = uint8_t; };
template<> struct Uint<16> { using type = uint16_t; };
template<> struct Uint<32> { using type = uint32_t; };
template<> struct Uint<64> { using type = uint64_t; };

template<uint8_t bits> using Uint_t = typename Uint<TypeBits<bits>::value>::type;

/// big-endian bitpack
template<uint16_t begin, uint16_t end=begin+1, class Res = Uint_t<end-begin>> 
struct Bitpack {
	// proxy
	struct V {
		/// read value from bitpack
		operator Res() const {
			auto b0 = buf + begin/8;
			
			if(end/8 == begin/8){ // begin and end are in the same byte
				constexpr auto mask = (1u << (end - begin)) - 1u;
				return *b0 >> (8 - end%8) & mask;
			}

			Res r = (begin%8 == 0 ? *b0 : (*b0) & (255u >> begin%8));
			
			constexpr auto bytebegin = (begin + 8u) & ~7u;      // begin of the next read aligned at byte boundary
			for(uint8_t i = (end - bytebegin)/8 ; i != 0; --i){ // read up complete bytes
				r <<= 8;
				r |= *(++b0);
			}

			if(end%8){ // end is not aligned at byte boundary
				r <<= end%8;
				r |= (*(++b0) >> (8 - end%8));
			}
			return r;
		}
		
		/// assign value to bitpack
		auto operator=(Res x)-> void {
			auto b0 = buf + begin/8;
			
			if(begin/8 == end/8){ // begin and end are in the same byte
				constexpr auto mask = (255u >> begin%8) & (255u >> end%8);
				b0 |= (x << (8 - end%8)) & mask;
			}
			
		}
		
		
		uint8_t* const buf;
	};
	
	auto operator ()(uint8_t* buf){ return V{buf}; }
};


struct Head {
	static Bitpack<3, 44> f2;
};


auto usage()-> void {
	
	uint8_t buf[15];
	auto f2 = Head::f2(buf);
}
