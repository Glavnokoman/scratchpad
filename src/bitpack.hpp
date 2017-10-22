#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

template<uint8_t val>
struct TypeBits {
	enum { value = val <= 8  ? 8
	             : val <= 16 ? 16
	             : val <= 32 ? 32
	             :             64 };
};

template<uint8_t bits> struct Uint;
template<> struct Uint<8>  { using type = uint8_t; };
template<> struct Uint<16> { using type = uint16_t; };
template<> struct Uint<32> { using type = uint32_t; };
template<> struct Uint<64> { using type = uint64_t; };
template<uint8_t bits> using Uint_t = typename Uint<TypeBits<bits>::value>::type;

/// big-endian bitpack
template<uint16_t begin, uint16_t end=begin+1u, class Res = Uint_t<end-begin>> 
struct bitpack{
	union {
		uint8_t* buf;
		const uint8_t* cbuf;
	};
	
	bitpack(uint8_t* buf): buf(buf) {}
	bitpack(const uint8_t* cbuf): cbuf(cbuf) {}

	operator Res() const {
		auto b0 = cbuf + begin/8;
		
		if(samebyte()){ // begin and end are in the same byte
			constexpr auto mask = (1u << (end - begin)%8u) - 1u;
			return (*b0 >> (8 - end%8)) & mask;
		}

		Res r = (begin%8 == 0 ? *b0 : (*b0) & (255u >> begin%8));
		
		constexpr auto bytebegin = (begin + 8u) & ~7u;     // begin of the next read aligned at byte boundary
		for(auto i = (end - bytebegin)/8u ; i != 0u; --i){ // read up complete bytes
			r = (r << 8);
			r |= *(++b0);
		}

		if(end%8){ // end is not aligned at byte boundary
			r = (r << end%8);
			r |= *(++b0) >> (8 - end%8);
		}
		return r;
	}
	
	auto operator=(Res x)-> void {
		assert(x <= (~Res(0) >> (sizeof(Res) - (end - begin)))); // number fits into designated bits
		if(samebyte()){ // begin and end are in the same byte
			constexpr auto mask = (255u >> begin%8);
			auto xb = uint8_t(x);
			buf[begin/8] |= (xb << (8 - end%8)) & mask;
		}
		
		auto b0 = buf + (end + 7)/8; // end of the next byte to write
		if(end%8){ // write the last bits not making a complete byte
			auto xb = uint8_t(x);
			*(--b0) |= (xb << (8 - end%8));
			x >>= end%8;
		}

		constexpr auto byteend = end & ~7u;                // end of the next write aligned at byte boundary
		for(auto i = (byteend - begin)/8u; i != 0; --i){ // write complete bytes
			*(--b0) = uint8_t(x);
			x >>= 8;
		}
		
		if(begin%8){ *(--b0) |= uint8_t(x); } // begin is not aligned at byte boundary
	}
	
	template<class T>
	auto friend operator == (bitpack b, T val)-> bool { return Res(b) == val; }
	
private: // helpers
	/// @return true if bitpack values are within one byte boundaries
	constexpr static auto samebyte() { return begin/8u == end/8u; }
};
