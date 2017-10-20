#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <vector>

/// Thread-safe multiple writers single reader circular pop-the-last buffer.
template <class T> class BufBuf;

/// Thread-safe multiple writers single reader circular pop-the-last buffer. Specialization for array types.
template <class T>
class BufBuf<T[]> {
public:
	/// Construct buffer with given number of writing slots. Actual number of slots is that + 2.
	BufBuf(size_t nslots      ///< number of writing slots
	       , size_t slotsize  ///< number of elements of type T in one slot
	): slotsize(slotsize), buf(2*slotsize)
	{
		buf.reserve((nslots + 2)*slotsize);
		rd_next = rd_cur = buf.data();
		wr_next = buf.data() + slotsize;
		assert(check());
	}
	
	/// @return readable slot if buffer non-empty, nullptr otherwise.
	/// Nonblocking. Slot data remains valid till next call to read() functions. 
	/// Does not invalidate previous data if nullptr is returned.
	auto try_read()-> T* {
		std::lock_guard<std::mutex> lock(m);
		assert(check());
		if(rd_cur == rd_next){
			return nullptr;
		}
		wr_next = rd_cur;
		rd_cur = rd_next;
		assert(check());
		return rd_cur;
	}
	
	/// @return readable slot.
	/// Blocks till readable data becomes available. 
	/// Slot data remains valid till next call to read() functions.
	auto read()-> T* {
		std::unique_lock<std::mutex> lock(m);
		assert(check());
		while(rd_cur == rd_next){
			cvpub.wait(lock);
		}
		wr_next = rd_cur;
		rd_cur = rd_next;
		assert(check());
		return rd_cur;
	}
	
	/// @return pointer to the next slot open for writing
	/// Publishes slot for reading.
	auto publish(T* publish_ptr)-> T* {
		assert(buf.data() <= publish_ptr 
		       && publish_ptr < buf.data() + buf.size());   // publish pointer is within cuurent buffer
		assert((publish_ptr - buf.data()) % slotsize == 0); // publish pointer points to head of some slot
		
		T* ret;
		{ std::lock_guard<std::mutex> lock(m);
			assert(publish_ptr != rd_cur); // publish event to the slot currently being read
			assert(check());
			if(rd_next != rd_cur){
				ret = rd_next;
				rd_next = publish_ptr;
				assert(check());
				return ret;
			}
			rd_next = publish_ptr;
			ret = wr_next;
			assert(check());
		}
		cvpub.notify_one();
		
		return ret;
	}

	/// @return pointer to a next writing slot. The slot is ready to be written to.
	/// @throw std::runtime_error if no more writing slots are available.
	auto getWriteSlot()-> T* {
		std::lock_guard<std::mutex> lock(m);
		assert(check());
		if(buf.size() == buf.capacity()){
			throw std::runtime_error("writers exhausted");
		}
		auto r = buf.data() + buf.size();
		buf.resize(buf.size() + slotsize);
		assert(check());
		return r;
	}

	/// @return pointer to a next writing slot on success, nullptr if no more writing slots are available.
	/// Returned slot is ready to be written to.
	auto getWriteSlot(std::nothrow_t)-> T* {
		std::lock_guard<std::mutex> lock(m);
		assert(check());
		if(buf.size() == buf.capacity()){
			return nullptr;
		}
		auto r = buf.data() + buf.size();
		buf.resize(buf.size() + slotsize);
		assert(check());
		return r;
	}
private:
	/// Class invariant. if the buf is empty wr_next should point to a 'free' slot. 
	/// Must hold on enter and exit of critical sections.
	auto check()-> bool {
		return rd_next != rd_cur || wr_next != rd_cur;
	}
private: // data
	const size_t slotsize; ///< size of the slot (in units of sizeof(T))
	std::vector<T> buf;    ///< buffer memory
	std::mutex m;
	std::condition_variable cvpub; ///< triggered on publish() event if the buffer is empty
	T* rd_cur;  ///< slot currently being read. rd_cur == rd_next signifies empty buffer.
	T* rd_next; ///< slot next to read, the last published slot.
	T* wr_next; ///< for empty buffer slot free to write. otherwise points to random slot
}; // class BufBuf<T*>
