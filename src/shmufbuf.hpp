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
		auto is_empty() const-> bool { return rd_cur == rd_next; }
		auto set_empty(){rd_cur = rd_next;}

		uint8_t wr_next;
		uint8_t rd_cur;
		uint8_t rd_next;
	};

	// basic wrapper around flock, to give it with Lockable interface
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
		File_handle(int fd): _fd(fd){}
		File_handle(const File_handle&) = delete;
		File_handle(File_handle&& other) noexcept : _fd(other._fd) { other._fd = -1; }
		~File_handle() noexcept { if(_fd != -1) close(_fd); }

		auto fd() const {return _fd;}
	private:
		int _fd;
	}; // struct File_handle
} // namespace detail


//template<class... Ts> struct ShmufBuf {};

/// 'Postbox' buffer for interprocess data transfer.
/// Memory-mapped IPC SPSC pop-the-last FIFO buffer. Overload for value types
template<class T>
struct ShmufBuf {
	// TODO: static assert that T is bitwise copyable

	static auto type_size() { return sizeof(T); }

	/// Tries to pop the value from the buffer. Nonblocking
	/// \return Pointer to the value in the buffer. The pointer remains valid (and underlying data const) until the next call to one of pop() function. if the buffer is empty the nullptr is returned.
	auto try_pop()-> T* {
		auto flock = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck(flock);

		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		if(cb.is_empty()){
			return nullptr;
		}
		cb.set_empty();
		auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size();
		return reinterpret_cast<T*>(p);
	}

	/// Returns the value from the buffer. Blocking. If the buffer is empty waits till smth is pushed there and then returns valid reference.
	/// Returned reference remains valid (and underlying data const) untill the next call to one of pop() functions.
	auto pop()-> T& {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());

		auto id_wr = uint8_t{};
		{
			auto flock = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
			std::lock_guard<detail::Flock> lck(flock);
			if(!cb.is_empty()){
				auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size();
				cb.set_empty();
				return reinterpret_cast<T&>(*p);
			}

			id_wr = cb.wr_next;
		}
		auto flock = detail::Flock(fd.fd(), F_RDLCK, sizeof(T), sizeof(detail::ControlBlock) + id_wr*type_size());
		std::lock_guard<detail::Flock> lock_blk(flock); // lazy-waiting

		auto flock_ctl = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);

		cb.set_empty();
		auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size();
		return reinterpret_cast<T&>(*p);
	}

	///
	auto next_wrid(const detail::ControlBlock& cb)-> uint8_t {
		auto r = (cb.wr_next + 1) % nslots;
		while(r == cb.rd_cur){
			r = (r + 1) % nslots;
		}
		return r;
	}

	///
	auto push(const T& frame)-> void {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());

		auto p = reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.wr_next*sizeof(T));
		std::copy_n(&frame, 1, p);

		detail::Flock(fd.fd(), F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.wr_next*sizeof(T)).unlock();

		auto flock_ctl = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);

		cb.rd_next = cb.wr_next;
		cb.wr_next = next_wrid(cb);

		detail::Flock(fd.fd(), F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.wr_next*sizeof(T)).try_lock();
	}

	///
	auto empty()-> bool {
		auto flock = detail::Flock(fd.fd(), F_RDLCK);
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
		auto flock = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck(flock);

		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		if(cb.is_empty()){
			return nullptr;
		}
		cb.set_empty();
		return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size());
	}

	/// Returns the value from the buffer. Blocking. If the buffer is empty waits till smth is pushed there and then returns valid reference.
	/// Returned reference remains valid (and underlying data const) untill the next call to one of pop() functions.
	auto pop()-> T* {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());

		auto wr_next = uint8_t{};
		{
			auto flock = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
			std::lock_guard<detail::Flock> lck(flock);
			if(!cb.is_empty()){
				cb.set_empty();
				auto p = ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size();
				return reinterpret_cast<T*>(p);
			}

			wr_next = cb.wr_next;
		}
		auto flock = detail::Flock(fd.fd(), F_RDLCK, sizeof(T), sizeof(detail::ControlBlock) + wr_next*type_size());
		std::lock_guard<detail::Flock> lock_blk(flock); // lazy-waiting

		auto flock_ctl = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);

		cb.set_empty();
		return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.rd_cur*type_size());
	}

	///
	auto next_wrid(const detail::ControlBlock& cb)-> uint8_t {
		auto r = (cb.wr_next + 1) % nslots;
		while(r == cb.rd_cur){
			r = (r + 1) % nslots;
		}
		return r;
	}

	///
	auto push(const T frame[])-> void {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());

		auto p = reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.wr_next*type_size());
		std::copy(frame, frame+slot_size, p);

		detail::Flock(fd.fd(), F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.wr_next*type_size()).unlock();

		auto flock_ctl = detail::Flock(fd.fd(), F_WRLCK, sizeof(detail::ControlBlock));
		std::lock_guard<detail::Flock> lck_ctl(flock_ctl);

		cb.rd_next = cb.wr_next;
		cb.wr_next = next_wrid(cb);

		detail::Flock(fd.fd(), F_WRLCK, sizeof(T), sizeof(detail::ControlBlock) + cb.wr_next*type_size()).try_lock();
	}

	///
	auto empty()-> bool {
		auto flock = detail::Flock(fd.fd(), F_RDLCK);
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
