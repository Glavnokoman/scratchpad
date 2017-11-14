#pragma once

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

	/// @return true if begin and end of the bit range values are within same byte
	template<uint8_t begin, uint8_t end>
	constexpr auto samebyte()-> bool { return begin/8u == end/8u; }
	
	// write complete bytes to byte array
	template<class Buf, class T, uint8_t times, bool shift_last>
	auto write_complete_bytes(Buf& buf, T& x){
		if constexpr(times > 0){
			*(--buf) = uint8_t(x);
			if constexpr(times > 1){
				write_complete_bytes<Buf, T, times - 1, shift_last>(buf, x >>= 8);
			} else if constexpr(shift_last){
				x >>= 8;
			}
		} 
	}
	
	// |= complete bytes to byte array
	template<class Buf, class T, uint8_t times, bool shift_last>
	auto or_complete_bytes(Buf& buf, T& x){
		if constexpr(times > 0){
			*(--buf) |= uint8_t(x);
			if constexpr(times > 1){
				or_complete_bytes<Buf, T, times - 1, shift_last>(buf, x >>= 8);
			} else if constexpr(shift_last){
				x >>= 8;
			}
		} 
	}

	///
	template<class Buf, uint16_t begin, uint16_t end, class Res=Uint_t<end-begin>>
	struct BitsProxy_{ 
		const Buf buf;
		
		explicit BitsProxy_(Buf buf): buf{buf}{}
		
		/// read the value of the bits range
		operator Res() const {
			auto b0 = buf + begin/8;
			if constexpr(samebyte<begin, end>()){ // begin and end are in the same byte
				constexpr auto mask = (1u << (end - begin)) - 1u;
				return (*b0 >> (8 - end%8)) & mask;
			} else {
				Res r = (begin%8 == 0 ? *b0 : (*b0) & (255u >> begin%8));
				
				constexpr auto bytebegin = (begin + 8u) & ~7u;    // begin of the next read aligned at byte boundary
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
		}
		
		/// or the value to the bits range
		auto operator|= (Res x)-> void {
			assert(x <= (Res(-1) >> (8*sizeof(Res) - (end - begin)))); // number fits into designated bits
			if constexpr(samebyte<begin, end>()){ // begin and end are in the same byte
				buf[begin/8] |= uint8_t(x) << (8 - end%8);
			} else {
				auto b0 = buf + (end + 7)/8; // end of the next byte to write
				if constexpr(end%8 != 0){    // write the last bits not making a complete byte
					*(--b0) |= (uint8_t(x) << (8 - end%8));
					x >>= end%8;
				}
		
				constexpr auto completebytes = ((end & ~7u) - begin)/8u;
				or_complete_bytes<Buf, Res, completebytes, begin%8 != 0>(b0, x);
		
				if constexpr(begin%8 != 0){ // begin is not aligned at byte boundary
					*(--b0) |= uint8_t(x);
				}
			}
		}
		
		/// Write value to the bits range. 
		/// For incomplete byte write is equivalent to |= after zeroing the target bits first.
		auto operator= (Res x)-> void {
			assert(x <= (Res(-1) >> (8*sizeof(Res) - (end - begin)))); // number fits into designated bits
			if constexpr(samebyte<begin, end>()){ // begin and end are in the same byte
				buf[begin/8] &= (255u << (8 - begin%8)) | (255u >> end%8);
				buf[begin/8] |= uint8_t(x) << (8 - end%8);
			} else {
				auto b0 = buf + (end + 7)/8; // end of the next byte to write
				if constexpr(end%8 != 0){    // write the last bits not making a complete byte
					*(--b0) &= 255u >> end%8;
					*b0 |= uint8_t(x) << (8 - end%8);
					x >>= end%8;
				}
		
				constexpr auto completebytes = ((end & ~7u) - begin)/8u;
				write_complete_bytes<Buf, Res, completebytes, begin%8 != 0>(b0, x);
				
				if constexpr(begin%8 != 0){ // begin is not aligned at byte boundary
					*(--b0) &= 255u << (8 - begin%8);
					*b0 |= uint8_t(x);
				}
			}
		}

		///
		template<class T>
		friend auto operator== (BitsProxy_ x, T val)-> bool { return Res(x) == val; }
	}; // struct BitsProxy_
	
	template<uint16_t begin, uint16_t end>
	using BitsProxy = BitsProxy_<uint8_t*, begin, end, Uint_t<end-begin>>;
	
	template<uint16_t begin, uint16_t end>
	using ConstBitsProxy = BitsProxy_<uint8_t const*, begin, end, Uint_t<end-begin>>;
}

template<uint16_t begin, uint16_t end=begin+1> struct Bits{ enum{Begin=begin, End=end};};

template<class T>
auto bits(const uint8_t* buf) {
	constexpr auto cboff = T::Begin/8;
	return detail::ConstBitsProxy<T::Begin - 8*cboff, T::End - 8*cboff>{buf + cboff}; 
}

template<class T>
auto bits(uint8_t* buf) {
	constexpr auto cboff = T::Begin/8;
	return detail::BitsProxy<T::Begin - 8*cboff, T::End - 8*cboff>{buf + cboff};
}
