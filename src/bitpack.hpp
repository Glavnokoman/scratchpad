#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace detail {
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

	template<uint16_t begin, uint16_t end, class Res=Uint_t<end-begin>>
	struct BitsProxy{ 
		uint8_t* const buf;
		
		explicit BitsProxy(uint8_t* buf): buf{buf}{}
		
		operator Res() const {
			return Res{};
		}
	};
	
	template<uint16_t begin, uint16_t end, class Res=Uint_t<end-begin>>
	struct ConstBitsProxy{
		const uint8_t* const buf;
		
		explicit ConstBitsProxy(const uint8_t* buf): buf{buf}{}
		
		operator Res() const { return Res{}; }
	};
}

template<uint16_t begin, uint16_t end> struct BitRange{ enum{Begin=begin, End=end};};

template<class T>
auto bits(const uint8_t* buf) { 
	return detail::ConstBitsProxy<T::Begin, T::End>{buf}; 
}

template<class T>
auto bits(uint8_t* buf) {
	return detail::BitsProxy<T::Begin, T::End>{buf};
}


/// big-endian bitpack
template<uint16_t begin, uint16_t end=begin+1u, class Res = detail::Uint_t<end-begin>> 
struct bitpack{
	uint8_t* buf;
	const uint8_t* cbuf;

	explicit bitpack(uint8_t* buf): buf(buf), cbuf(buf) {}
	explicit bitpack(const uint8_t* cbuf): buf(nullptr), cbuf(cbuf) {}

	operator Res() const {
		auto b0 = cbuf + begin/8;
		
		if constexpr (samebyte()){ // begin and end are in the same byte
			constexpr auto mask = (1u << (end - begin)) - 1u;
			return (*b0 >> (8 - end%8)) & mask;
		}

		Res r = (begin%8 == 0 ? *b0 : (*b0) & (255u >> begin%8));
		
		constexpr auto bytebegin = (begin + 8u) & ~7u;     // begin of the next read aligned at byte boundary
		for(auto i = (end - bytebegin)/8u ; i != 0u; --i){ // read up complete bytes
			r = (r << 8);
			r |= *(++b0);
		}

		if constexpr (end%8 != 0){ // end is not aligned at byte boundary
			r = (r << end%8);
			r |= *(++b0) >> (8 - end%8);
		}
		return r;
	}
	
//	template<class T>
	auto operator=(Res x)-> void {
		assert(x <= (Res(-1) >> (8*sizeof(Res) - (end - begin)))); // number fits into designated bits
		
		if constexpr (samebyte()){ // begin and end are in the same byte
			constexpr auto mask = (255u >> begin%8);
			buf[begin/8] |= (uint8_t(x) << (8 - end%8)) & mask;
			return;
		}
		
		auto b0 = buf + (end + 7)/8; // end of the next byte to write
		if constexpr (end%8 != 0){ // write the last bits not making a complete byte
			*(--b0) |= (uint8_t(x) << (8 - end%8));
			x >>= end%8;
		}

		constexpr auto byteend = end & ~7u;              // end of the next write aligned at byte boundary
		if constexpr ((byteend - begin) > 8u){
			for(auto i = (byteend - begin)/8u; i != 0; --i){ // write complete bytes
				*(--b0) = uint8_t(x);
				x >>= 8;
			}
		}
		
		if constexpr (begin%8 != 0){ *(--b0) |= uint8_t(x); } // begin is not aligned at byte boundary
	}
	
	template<class T>
	auto friend operator == (bitpack b, T val)-> bool { return Res(b) == val; }
	
private: // helpers
	/// @return true if bitpack values are within one byte boundaries
	constexpr static auto samebyte() { return begin/8u == end/8u; }
};
