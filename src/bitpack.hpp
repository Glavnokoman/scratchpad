#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

#include <iostream> // debug

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

	/// @return true if begin and end of the bit range values are within same byte
	template<uint8_t begin, uint8_t end>
	constexpr static auto samebyte() { return begin/8u == end/8u; }
	
	///
	template<class Buf, uint16_t begin, uint16_t end, class Res=Uint_t<end-begin>>
	struct BitsProxy_{ 
		const Buf buf;
		
		explicit BitsProxy_(Buf buf): buf{buf}{}
		
		///
		operator Res() const {
			auto b0 = buf + begin/8;
			
			if constexpr(samebyte<begin, end>()){ // begin and end are in the same byte
				constexpr auto mask = (1u << (end - begin)) - 1u;
				return (*b0 >> (8 - end%8)) & mask;
			}
	
			Res r = (begin%8 == 0 ? *b0 : (*b0) & (255u >> begin%8));
			
			constexpr auto bytebegin = (begin + 8u) & ~7u;     // begin of the next read aligned at byte boundary
			for(auto i = (end - bytebegin)/8u; i != 0u; --i){ // read up complete bytes
				r = (r << 8);
				r |= *(++b0);
			}
	
			if constexpr(end%8 != 0){ // end is not aligned at byte boundary
				r = (r << end%8);
				r |= *(++b0) >> (8 - end%8);
			}
			return r;
		}
		
		///
		auto operator= (Res x)-> void {
			assert(x <= (Res(-1) >> (8*sizeof(Res) - (end - begin)))); // number fits into designated bits
			
			if constexpr(samebyte<begin, end>()){ // begin and end are in the same byte
				buf[begin/8] |= (uint8_t(x) << (8 - end%8)) & 255u;
				return;
			}
			
			if constexpr(end%8){ // end is not aligned at byte boundary
				buf[end/8] |= (uint8_t(x) << (8 - end%8)) & 255u;
				x >> end%8;
			}
			
			constexpr auto bytebegin = (begin + 7u) & ~7u;
			auto b0 = buf + end/8 - 1;
			for(auto i = (end - bytebegin)/8u; i != 0u; --i){
				*b0-- = uint8_t(x);
				x >>= 8;
			}
			
			if constexpr(begin%8u){
				*b0 |= uint8_t(x) & 255u;
			}
		}
		
//		/// write value to the bitfield
//		auto operator=(Res x) -> void {
//			assert(x <= (Res(-1) >> (8 * sizeof(Res) - (end - begin)))); // number fits into designated bits
//			if (samebyte()) {                                            // begin and end are in the same byte
//				constexpr auto mask = (255u >> begin % 8);
//				buf[begin / 8] |= (uint8_t(x) << (8 - end % 8)) & mask;
//				return;
//			}
	
//			auto b0 = buf + (end + 7) / 8; // end of the next byte to write
//			if (end % 8) {                 // write the last bits not making a complete byte
//				*(--b0) |= (uint8_t(x) << (8 - end % 8));
//				x >>= end % 8;
//			}
	
//			constexpr auto byteend = end & ~7u;                  // end of the next write aligned at byte boundary
//			for (auto i = (byteend - begin) / 8u; i != 0; --i) { // write complete bytes
//				*(--b0) = uint8_t(x);
//				x >>= 8;
//			}
	
//			if (begin % 8) {
//				*(--b0) |= uint8_t(x);
//			} // begin is not aligned at byte boundary
//		}
		
		///
		template<class T>
		friend auto operator== (BitsProxy_ x, T val)-> bool { return Res(x) == val; }
	}; // struct BitsProxy_
	
	template<uint16_t begin, uint16_t end>
	using BitsProxy = BitsProxy_<uint8_t*, begin, end, Uint_t<end-begin>>;
	
	template<uint16_t begin, uint16_t end>
	using ConstBitsProxy = BitsProxy_<uint8_t const*, begin, end, Uint_t<end-begin>>;
}

template<uint16_t begin, uint16_t end=begin+1> struct BitRange{ enum{Begin=begin, End=end};};

template<class T>
auto bits(const uint8_t* buf) {
//	std::cout << __PRETTY_FUNCTION__ << "\n";
	return detail::ConstBitsProxy<T::Begin, T::End>{buf}; 
}

template<class T>
auto bits(uint8_t* buf) {
//	std::cout << __PRETTY_FUNCTION__ << "\n";
	return detail::BitsProxy<T::Begin, T::End>{buf};
}
