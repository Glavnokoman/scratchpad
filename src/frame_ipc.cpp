#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>

// posix stuff
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include <unistd.h>

using namespace std;

namespace{
	enum MODE {CLIENT, SERVER};
	const size_t FRAME_SIZE = 1024;
	
	/// Handle command line parameters
	struct Params{
		Params(int argc, char* argv[]): _pname(argv[0]){
			if(argc != 3){
				usage();
				exit(1);
			}
			
			if(strcmp(argv[1], "CLIENT") == 0){
				_mode = MODE::CLIENT;
			} else if(strcmp(argv[1], "SERVER") == 0){
				_mode = MODE::SERVER;
			} else {
				usage();
				exit(1);
			}
			
			_filepath = argv[2];
		}
		
		auto usage() const-> void {
			std::cout << "create client and server exchanging random data via socket" << "\n";
			std::cout << "usage: " << _pname << " {CLIENT|SERVER} path/to/socket" << "\n";
		}
		
		auto mode() const { return _mode; }
		auto filepath() const { return _filepath; }
		
		const char* _pname;    ///< program name
		const char* _filepath; ///< file path used as an interface to socket
		MODE _mode;      ///< execution mode
	}; // struct Params
	
	/// run the client
	__attribute__ ((noreturn))
	auto client(const char* path){
		auto buf = std::vector<uint8_t>(FRAME_SIZE);
		
		auto s = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if(s < 0){
			throw std::runtime_error("failed to open socket");
		}
		
		auto remote = sockaddr_un{};
		remote.sun_family = AF_LOCAL;
		strncpy(remote.sun_path, path, sizeof(remote.sun_path));
		if(connect(s, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) < 0){
			throw std::runtime_error(std::string("client failed to connect to ") + path);
		}
		
		for(;;){
			if(recv(s, buf.data(), buf.size()*sizeof(buf[0]), 0) <= 0){
				throw std::runtime_error("network error");
			}
			std::cout << buf[0] << std::endl;
			std::this_thread::sleep_for(1s);
		}
	}
	
	/// run the server
	__attribute__ ((noreturn))
	auto server(const char* path){
		auto s = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if(s < 0){
			throw std::runtime_error("failed to open server socket");
		}
		
		auto local = sockaddr_un{};
		local.sun_family = AF_LOCAL;
		strncpy(local.sun_path, path, sizeof(local.sun_path));
		unlink(local.sun_path);
		
		if(bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local))){
			throw std::runtime_error("failed to bind server socket");
		}
		
		if(listen(s, 4) < 0){
			throw std::runtime_error("listen...");
		}
		
		auto len = socklen_t(sizeof(sockaddr_un));
		sockaddr_un remote;
		auto s2 = accept(s, reinterpret_cast<sockaddr*>(&remote), &len); // blocking???
		if(s2 < 0){
			throw std::runtime_error("failed to accept connection");
		}
		
		auto i = uint8_t(0);
		for(;;){
			fd_set wfds;
			FD_ZERO(&wfds);
			FD_SET(s2, &wfds);
	
			sigset_t empty_mask;
			sigemptyset(&empty_mask);
			
			auto buf = std::vector<uint8_t>(FRAME_SIZE, uint8_t(i++)); // dgs if i++ wraps around
			if (pselect(s2 + 1, nullptr, &wfds, nullptr, nullptr, &empty_mask)) {
				send(s2, buf.data(), buf.size()*sizeof(buf[0]), 0);
			}
		}
	}
} // namespace


auto main(int argc, char* argv[]) -> int {
	ios_base::sync_with_stdio(false);

	const auto p = Params(argc, argv);
	switch(p.mode()){
		case MODE::CLIENT:
			client(p.filepath());
			break;
		case MODE::SERVER:
			server(p.filepath());
			break;
	}

	return 0;
}
