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

	template<class T, uint16_t begin, uint16_t end, bool samebyte> struct bitpack_reader;
	
	namespace samebyte {
		template<uint16_t begin, uint16_t end>
		struct bitpack_reader{
			auto operator()(const uint8_t* buf)-> uint8_t {
				constexpr auto mask = (1u << (end - begin)) - 1u;
				return (*(buf + begin) >> (8 - end%8)) & mask;
			}
		}; // bitpack<samebyte=true>
	} // namespace samebyte

	namespace multibyte {
		template<class T, uint16_t begin, uint16_t headoffs>
		auto read_head(const uint8_t* buf)-> T {
			static_assert(begin%8 != 0, "begin of read must not be aligned at byte boundary");
			static_assert(headoffs < 8, "head chunk must be within the byte boundaries");
			return (*buf) & (255u >> headoffs);
		}
	
		template<class T, uint16_t begin, uint16_t bulkbytes>
		auto attach_bulk(T r, const uint8_t* buf)-> T {
			for(auto i = bulkbytes; i != 0u; --i, ++buf){
				r <<= 8;
				r |= *(buf);
			}
			return r;
		}
		
		template<class T, uint16_t begin, uint16_t bulkbytes>
		auto read_bulk(const uint8_t* buf)-> T {
			static_assert(bulkbytes != 0, "requires non-zero num of bytes to read");
			return attach_bulk<T, begin + 8, bulkbytes - 1>(*buf, buf);		
		}
		
		template<class T, uint16_t begin, uint16_t tailbits>
		auto attach_tail(T r, const uint8_t* buf)-> T {
			r <<= tailbits;
			r |= *(buf + begin/8) >> (8 - tailbits);
			return r;
		}
		
		template<class T, uint16_t begin, uint16_t tailbits>
		auto read_tail(const uint8_t* buf)-> T {
			return *(buf + begin/8) >> (8 - tailbits);
		}
		
		template<class T, uint16_t begin, uint16_t end, uint16_t headoffs, uint16_t bulkbytes, uint16_t tailbits>
		struct bitpack_reader_multibyte {
			auto operator()(const uint8_t* buf)-> T {
				auto val = read_head<T, begin, headoffs>(buf);
				val = attach_bulk<T, begin + 8u - headoffs, bulkbytes>(val, buf);
				val = attach_tail<T, end, tailbits>(val, buf);
				return val;
			}
		};
		
		template<class T, uint16_t begin, uint16_t end, uint16_t tailbits>
		struct bitpack_reader_multibyte<T, begin, end, 0, 0, tailbits> {
			auto operator()(const uint8_t* buf)-> T { return read_tail<T, end, tailbits>(buf); }
		};
		
		template<class T, uint16_t begin, uint16_t end, uint16_t bulkbytes>
		struct bitpack_reader_multibyte<T, begin, end, 0, bulkbytes, 0> {
			auto operator()(const uint8_t* buf)-> T { return read_bulk<T, begin, bulkbytes>(buf); }
		};
		
		template<class T, uint16_t begin, uint16_t end, uint16_t bulkbytes, uint16_t tailbits>
		struct bitpack_reader_multibyte<T, begin, end, 0, bulkbytes, tailbits>{
			auto operator()(const uint8_t* buf)-> T {
				return attach_tail<T, end, tailbits>( read_bulk<T, begin, bulkbytes>(buf), buf); 
			}
		};
		
		template<class T, uint16_t begin, uint16_t end, uint16_t headoffs>
		struct bitpack_reader_multibyte<T, begin, end, headoffs, 0, 0>{
			auto operator()(const uint8_t* buf)-> T { return read_head<T, begin, headoffs>(buf); }
		};
		
		template<class T, uint16_t begin, uint16_t end, uint16_t headoffs, uint16_t tailbits>
		struct bitpack_reader_multibyte<T, begin, end, headoffs, 0, tailbits>{
			auto operator()(const uint8_t* buf)-> T {
				return attach_tail<T, end, tailbits>(read_head<T, begin, headoffs>(buf), buf);
			}
		};
	
		template<class T, uint16_t begin, uint16_t end, uint16_t headoffs, uint16_t bulkbytes>
		struct bitpack_reader_multibyte<T, begin, end, headoffs, bulkbytes, 0>{
			auto operator()(const uint8_t* buf)-> T {
				return attach_bulk<T, begin + 8u - headoffs, bulkbytes>(
				         read_head<T, begin, headoffs>(buf), buf);
			}
		};
		
		template<class T, uint16_t begin, uint16_t end>
		struct bitpack_reader{
			auto operator()(const uint8_t* buf)-> T {
				return bitpack_reader_multibyte<T, begin, end, begin%8, end/8 - begin/8, end%8>{}(buf);
			}
		}; // bitpack<samebyte=false>
		
	} // namespace multibyte
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
	
//	template<class T>
	auto operator=(Res x)-> void {
		assert(x <= (Res(-1) >> (8*sizeof(Res) - (end - begin)))); // number fits into designated bits
		if(samebyte()){ // begin and end are in the same byte
			constexpr auto mask = (255u >> begin%8);
			buf[begin/8] |= (uint8_t(x) << (8 - end%8)) & mask;
			return;
		}
		
		auto b0 = buf + (end + 7)/8; // end of the next byte to write
		if(end%8){ // write the last bits not making a complete byte
			*(--b0) |= (uint8_t(x) << (8 - end%8));
			x >>= end%8;
		}

		constexpr auto byteend = end & ~7u;              // end of the next write aligned at byte boundary
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
