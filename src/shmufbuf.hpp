#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <tuple>

namespace detail{
	///
	struct ControlBlock {
		enum STATE:uint8_t {EMPTY=0, FULL=1};
		
		uint8_t id_wr;
		uint8_t id_rd;
		STATE state;
	};
	
	///
	struct FileBlockLock {
		FileBlockLock(const FileBlockLock&) = delete;
		auto operator= (const FileBlockLock&) = delete;
		
		FileBlockLock(FileBlockLock&& other) noexcept
		   : _fl(other._fl), _fd(other._fd)
		{
			other.detach();
		}
		
		FileBlockLock(int fd, short l_type=F_WRLCK, short int l_whence=SEEK_SET
		                , long l_start=0, long l_len=sizeof(ControlBlock))
		: _fl{l_type, l_whence, l_start, l_len, getpid()}
		, _fd(fd)
		{
			fcntl(fd, F_SETLKW, &_fl);
		}
		
		~FileBlockLock() noexcept {
			release();
		}
		
		auto release()-> void {
			if(!detached()){
				_fl.l_type = F_UNLCK;
				fcntl(_fd, F_SETLK, &_fl);
				detach();
			}
		}
		
		auto detach()-> void { _fd = -1; }
		auto detached() const-> bool { return _fd == -1;}
		
	private: // data
		flock _fl;
		int _fd;
	}; // struct FileBlockLock
	
	struct Unmapper {
		const int fd;
		auto operator()(char* ptr) const-> void {
			struct stat stat_buf;
			if(ptr != nullptr && fstat(fd, &stat_buf) != -1){
				munmap(ptr, size_t(stat_buf.st_size));
			}
		}
	}; // struct Unmapper
	
	/// raii wrapper of a file handle
	struct File_handle {
		const int fd;
		~File_handle() noexcept {
			close(fd);
		}
	}; // struct File_handle
} // namespace detail

/// memory-mapped IPC SPSC FIFO buffer
template<class T>
struct ShmufBuf {
	
	/// Tries to pop the value from the buffer. Nonblocking
	/// \return Pointer to the value in the buffer. The pointer remains valid (and underlying data const) until the next call to one of pop() function. if the buffer is empty the nullptr is returned.
	auto try_pop()-> T* { 
		auto lock = detail::FileBlockLock(fd.fd);
		
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		if(cb.state == detail::ControlBlock::EMPTY){
			return nullptr;
		}
		auto rdid_n = (cb.id_wr + nslots - 1) % nslots;
		while(rdid_n == cb.id_rd){
			rdid_n = (rdid_n + nslots - 1) % nslots;
		}
		cb.id_rd = rdid_n;
		cb.state = detail::ControlBlock::EMPTY;
	
		return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + rdid_n*sizeof(T));
	}
	
	/// Returns the value from the buffer. Blocking. If the buffer is empty waits till smth is pushed there and then returns valid reference. 
	/// Returned reference remains valid (and underlying data const) untill the next call to one of pop() functions.
	auto pop()-> T& {
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto id_wr = uint8_t{};
		{  auto lock_ctl = detail::FileBlockLock(fd.fd);
			if(cb.state == detail::ControlBlock::EMPTY){
				auto rdid_n = (cb.id_wr + nslots - 1) % nslots;
				while(rdid_n == cb.id_rd){
					rdid_n = (rdid_n + nslots - 1) % nslots;
				}
				cb.id_rd = rdid_n;
				cb.state = detail::ControlBlock::EMPTY;
				return reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + rdid_n*sizeof(T));
			}
			
			id_wr = cb.id_wr;
		}
		// TODO: potential problem. cb.id_wr may be overriden by another process here
		auto lock_blk = detail::FileBlockLock(fd.fd, F_RDLCK, SEEK_SET
		                                     , sizeof(detail::ControlBlock) + id_wr*sizeof(T), sizeof(T));
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
	auto push(const T& frame)-> void { 
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		auto p = reinterpret_cast<T*>(ptr.get() + sizeof(detail::ControlBlock) + cb.id_wr*sizeof(T));
		std::copy_n(p, 1, &frame);
		
		auto lck_ctl = detail::FileBlockLock(fd.fd);

		cb.state = detail::ControlBlock::FULL;
		
		auto wrid_n = next_wrid(cb);
		detail::FileBlockLock(fd.fd, F_WRLCK, SEEK_SET, sizeof(detail::ControlBlock) + cb.id_wr*sizeof(T), sizeof(T)); // clears the lock (probably)
		detail::FileBlockLock(fd.fd, F_WRLCK, SEEK_SET, sizeof(detail::ControlBlock) + wrid_n*sizeof(T), sizeof(T)).detach();
		cb.id_wr = wrid_n;
	}
	
	///
	auto empty()-> bool {
		auto lck = detail::FileBlockLock(fd.fd, F_RDLCK);
		auto& cb = reinterpret_cast<detail::ControlBlock&>(*ptr.get());
		
		return cb.state == detail::ControlBlock::EMPTY;
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
	using ptr_t = std::unique_ptr<char, detail::Unmapper>;
	
	detail::File_handle fd;  ///< file descriptor for a memory-mapped obj
	ptr_t ptr;               ///< pointer to head of a shared memory buffer. \internal TODO: make it const for c++17
	const uint8_t nslots;    ///< number of slots in buffer
}; // struct ShmufBuf
