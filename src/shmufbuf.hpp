#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <tuple>

namespace detail{
	///
	struct ControlBlock {
		auto is_empty() const-> bool { return id_rd == id_wr; }
		
		auto set_empty(){id_rd = id_wr;}
		
		uint8_t id_wr;
		uint8_t id_rd;
	};
	
	// basic wrapper aroud flock, to give it with Lockable interface
	class Flock {
	public:
		Flock(int fd, short l_type, long l_len, long l_start=0, short l_whence=SEEK_SET)
		   : _fl{l_type, l_whence, l_start, l_len, getpid()}, _fd{fd}, _type{l_type}
		{}
		
		auto lock()-> void {
			_fl.l_type=_type;
			fcntl(_fd, F_SETLKW, &_fl);
		}
		
		auto unlock() noexcept-> void {
			_fl.l_type=F_UNLCK;
			fcntl(_fd, F_SETLK, &_fl);
		}

		auto try_lock()-> bool {
			_fl.l_type=_type;
			return fcntl(_fd, F_SETLK, &_fl) != -1;
		}
	private: // data
		flock _fl;
		int   _fd; 
		short _type;
	};

	///
	struct Unmapper {
		const int fd;
		auto operator()(char* ptr) const-> void {
			struct stat stat_buf;
			if(ptr != nullptr && fstat(fd, &stat_buf) != -1){
				munmap(ptr, size_t(stat_buf.st_size));
			}
		}
	}; // struct Unmapper
	
	using unique_mpt = std::unique_ptr<char, Unmapper>;
	
	/// raii wrapper of a file handle
	struct File_handle {
		const int fd;
		~File_handle() noexcept { close(fd); }
	}; // struct File_handle
} // namespace detail


//template<class... Ts> struct ShmufBuf {}; 
	
/// memory-mapped IPC SPSC pop-the-last FIFO buffer. Overload for value types
template<class T>
struct ShmufBuf {
	// TODO: static assert that T is bitwise copyable
	
	static auto type_size() { return sizeof(T); }
	
	/// Tries to pop the value from the buffer. Nonblocking
	/// \return Pointer to the value in the buffer. The pointer remains valid (and underlying data const) until the next call to one of pop() function. if the buffer is empty the nullptr is returned.
	auto try_pop()-> T* { 
		auto flock = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck(flock);
		
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		if(cb.is_empty()){
			return nullptr;
		}
		cb.set_empty();
		return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.id_rd*type_size());
	}
	
	/// Returns the value from the buffer. Blocking. If the buffer is empty waits till smth is pushed there and then returns valid reference. 
	/// Returned reference remains valid (and underlying data const) untill the next call to one of pop() functions.
	auto pop()-> T& {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto id_wr = uint8_t{};
		{  
			auto flock = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
			std::lock_guard<detail::Flock> lck(flock);
			if(!cb.is_empty()){
				cb.set_empty();
				auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.id_rd*type_size();
				return reinterpret_cast<T&>(*p);
			}
			
			id_wr = cb.id_wr;
		}
		auto flock = detail::Flock(fd.fd, F_RDLCK, sizeof(T), sizeof(detail::ControlBlock) + id_wr*type_size());
		std::lock_guard<detail::Flock> lock_blk(flock); // lazy-waiting
		
		auto r = try_pop();
		assert(r != nullptr);
		return *r;
	}
	
	///
	auto next_wrid(const detail::ControlBlock& cb)-> uint8_t {
		auto r = (cb.id_wr + 1) % nslots;
		while(r == cb.id_rd){
			r = (r + 1) % nslots;
		}
		return r;
	}
	
	///
	auto push(const T& frame)-> void { 
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto p = reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.id_wr*sizeof(T));
		std::copy_n(&frame, 1, p);
		
		detail::Flock(fd.fd, F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.id_wr*sizeof(T)).unlock();

		auto flock_ctl = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);
		
		const auto id_wr_new = next_wrid(cb); // do not try to 'optimize' this one out
		cb.id_rd = cb.id_wr;
		cb.id_wr = id_wr_new;
		
		detail::Flock(fd.fd, F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.id_wr*sizeof(T)).try_lock();
	}
	
	///
	auto empty()-> bool {
		auto flock = detail::Flock(fd.fd, F_RDLCK);
		std::lock_guard<detail::Flock> lck_ctl(flock);
		
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		return cb.is_empty();
	}
	
	///
	static auto create(const char* path, uint8_t nslots=3)-> ShmufBuf {
		shm_unlink(path);
		auto flags = int{O_RDWR | O_CREAT};
		auto fd = shm_open(path, flags, S_IRWXU);
		if(fd == -1){
			throw std::runtime_error(std::strerror(errno));
		}
		
		const auto len = sizeof(detail::ControlBlock) + nslots*sizeof(T);
		if(ftruncate(fd, len) == -1){
			close(fd);
			throw std::runtime_error(std::strerror(errno));
		}
		
		auto ptr = reinterpret_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		return {fd, {ptr, detail::Unmapper{fd}}, nslots};
	}
	
	///
	static auto connect(const char* path)-> ShmufBuf {
		auto flags = int{O_RDWR};
		auto fd = shm_open(path, flags, 0);
		if(fd == -1){
			throw std::runtime_error(std::strerror(errno));
		}

		struct stat stat_buf;
		int err = fstat(fd, &stat_buf);
		if(err == -1){
			close(fd);
			throw  std::runtime_error(std::strerror(errno));
		}
		
		const auto len = size_t(stat_buf.st_size);
		auto ptr = reinterpret_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		const auto nslots = (len - sizeof(detail::ControlBlock))/sizeof(T);
		return {fd, {ptr, detail::Unmapper{fd}}, nslots};
	}
	
	// data	
	detail::File_handle fd;  ///< file descriptor for a memory-mapped obj
	detail::unique_mpt ptr;  ///< pointer to head of a shared memory buffer
	const uint8_t nslots;    ///< number of slots in buffer
}; // struct ShmufBuf


/// memory-mapped IPC SPSC pop-the-last FIFO buffer. Overload for non-static array types
template<class T>
struct ShmufBuf<T[]> {
	// TODO: get rid of duplicated code
	/// type size in bytes
	auto type_size() const {return slot_size*sizeof(T);}
	
	/// Tries to pop the value from the buffer. Nonblocking
	/// \return Pointer to the value in the buffer. The pointer remains valid (and underlying data const) until the next call to one of pop() function. if the buffer is empty the nullptr is returned.
	auto try_pop()-> T* { 
		auto flock = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck(flock);
		
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		if(cb.is_empty()){
			return nullptr;
		}
		cb.set_empty();
		return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.id_rd*type_size());
	}
	
	/// Returns the value from the buffer. Blocking. If the buffer is empty waits till smth is pushed there and then returns valid reference. 
	/// Returned reference remains valid (and underlying data const) untill the next call to one of pop() functions.
	auto pop()-> T* {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto id_wr = uint8_t{};
		{
			auto flock = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
			std::lock_guard<detail::Flock> lck(flock);
			if(!cb.is_empty()){
				cb.set_empty();
				auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.id_rd*type_size();
				return reinterpret_cast<T*>(p);
			}
			
			id_wr = cb.id_wr;
		}
		auto flock = detail::Flock(fd.fd, F_RDLCK, sizeof(T), sizeof(detail::ControlBlock) + id_wr*type_size());
		std::lock_guard<detail::Flock> lock_blk(flock); // lazy-waiting
		
		auto r = try_pop();
		assert(r != nullptr);
		return r;
	}
	
	///
	auto next_wrid(const detail::ControlBlock& cb)-> uint8_t {
		auto r = (cb.id_wr + 1) % nslots;
		while(r == cb.id_rd){
			r = (r + 1) % nslots;
		}
		return r;
	}
	
	///
	auto push(const T frame[])-> void { 
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto p = reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.id_wr*type_size());
		std::copy(frame, frame+slot_size, p);
		
		detail::Flock(fd.fd, F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.id_wr*type_size()).unlock();

		auto flock_ctl = detail::Flock(fd.fd, F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);
		
		const auto id_wr_new = next_wrid(cb); // do not try to 'optimize' this one out
		cb.id_rd = cb.id_wr;
		cb.id_wr = id_wr_new;
		
		detail::Flock(fd.fd, F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.id_wr*type_size()).try_lock();
	}
	
	///
	auto empty()-> bool {
		auto flock = detail::Flock(fd.fd, F_RDLCK);
		std::lock_guard<detail::Flock> lck_ctl(flock);
		
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		return cb.is_empty();
	}
	
	///
	static auto create(const char* path, size_t buf_size, uint8_t nslots=3)-> ShmufBuf {
		shm_unlink(path);
		auto flags = int{O_RDWR | O_CREAT};
		auto fd = shm_open(path, flags, S_IRWXU);
		if(fd == -1){
			throw std::runtime_error(std::strerror(errno));
		}
		
		const auto len = sizeof(detail::ControlBlock) + nslots*sizeof(T)*buf_size;
		if(ftruncate(fd, len) == -1){
			close(fd);
			throw std::runtime_error(std::strerror(errno));
		}
		
		auto ptr = reinterpret_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		std::fill_n(ptr, len, 0);
		return {fd, {ptr, detail::Unmapper{fd}}, buf_size, nslots};
	}
	
	///
	static auto connect(const char* path, size_t buf_size)-> ShmufBuf {
		auto flags = int{O_RDWR};
		auto fd = shm_open(path, flags, 0);
		if(fd == -1){
			throw std::runtime_error(std::strerror(errno));
		}

		struct stat stat_buf;
		int err = fstat(fd, &stat_buf);
		if(err == -1){
			close(fd);
			throw  std::runtime_error(std::strerror(errno));
		}
		
		const auto len = size_t(stat_buf.st_size);
		auto ptr = reinterpret_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		const auto nslots = (len - sizeof(detail::ControlBlock))/(buf_size*sizeof(T));
		if(nslots >= std::numeric_limits<uint8_t>::max()){
			throw std::runtime_error("can not create buffer with " + std::to_string(nslots) + " number of slots");
		}
		return {fd, {ptr, detail::Unmapper{fd}}, buf_size, uint8_t(nslots)};
	}
	
// private: // data
	detail::File_handle fd;  ///< file descriptor for a memory-mapped obj
	detail::unique_mpt ptr;  ///< pointer to head of a shared memory buffer
	const size_t  slot_size; ///< slot size in sizeof(T) units
	const uint8_t nslots;    ///< number of slots in buffer
}; // struct ShmufBuf
